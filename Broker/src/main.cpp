#include <Arduino.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include "config.h"
#include "MqttHandler.h"
#include "LcdUi.h"
#include "SettingsManager.h"
#include "WebServerHandler.h"
#include <ESP32Encoder.h>
#include <ArduinoJson.h>

// --- BIẾN TOÀN CỤC ---
bool isOnline = false;
unsigned long lastReconnectAttempt = 0;
const long RECONNECT_INTERVAL = 30000;
unsigned long lastStatusPublish = 0;
const long STATUS_PUBLISH_INTERVAL = 20000;

ESP32Encoder encoder;
long oldEncoderPosition = 0;

int buttonState;
int lastButtonState = HIGH;
unsigned long lastPressTime = 0;
unsigned long lastRepeatTime = 0;
const long LONG_PRESS_THRESHOLD = 500;
const long REPEAT_INTERVAL = 150;

// --- CÁC HÀM ---
void publishBrokerStatus() {
    StaticJsonDocument<512> doc;
    doc["mode"] = modeToString(g_currentMode);
    JsonArray relayStateArray = doc.createNestedArray("relay_status");
    for (int i = 0; i < NUM_RELAYS; i++) {
        relayStateArray.add(g_relayStatus[i]);
    }
    JsonObject settingsObj = doc.createNestedObject("settings");
    settingsObj["temp_min"] = g_tempMin;
    settingsObj["temp_max"] = g_tempMax;
    settingsObj["hum_min"] = g_humMin;
    settingsObj["hum_max"] = g_humMax;
    settingsObj["gas_limit"] = g_gasLimit;
    settingsObj["relay_delay"] = g_relayDelay;
    char json_output[512];
    serializeJson(doc, json_output);
    publishCommand("chuong/broker/status", json_output);
}

void setup() {
    Serial.begin(115200);
    setupLcd(); 
    
    WiFi.mode(WIFI_STA);
    WiFiManager wm;
    wm.setConfigPortalTimeout(180);

    if (wm.autoConnect("Broker-Setup-Portal")) {
        isOnline = true;
        Serial.println("\nKet noi thanh cong!");
        Serial.print("IP Address: ");
        Serial.println(WiFi.localIP());
    } else {
        isOnline = false;
        Serial.println("Khong the ket noi Wi-Fi. Chay o che do offline.");
    }

    updateLcdNetworkStatus(isOnline);
    
    setupSettings(); 
    setMode(AUTO);

    ESP32Encoder::useInternalWeakPullResistors = UP;
    encoder.attachHalfQuad(ENCODER_DT_PIN, ENCODER_CLK_PIN);
    encoder.clearCount();
    pinMode(ENCODER_SW_PIN, INPUT_PULLUP);

    if (isOnline) {
        setupMqttBroker(); 
        initWebServer();
    }
}

void handleButton() {
    buttonState = digitalRead(ENCODER_SW_PIN);
    if (buttonState == LOW && lastButtonState == HIGH) {
        lastPressTime = millis();
    }
    if (buttonState == HIGH && lastButtonState == LOW) {
        if (millis() - lastPressTime < LONG_PRESS_THRESHOLD) {
            handleEncoderClick();
        }
    }
    if (buttonState == LOW) {
        if (millis() - lastPressTime > LONG_PRESS_THRESHOLD) {
            if (millis() - lastRepeatTime > REPEAT_INTERVAL) {
                lastRepeatTime = millis();
                handleEncoderClick();
            }
        }
    }
    lastButtonState = buttonState;
}

void loop() {
    if (WiFi.status() != WL_CONNECTED) {
        if (isOnline) {
            isOnline = false;
            updateLcdNetworkStatus(false);
            Serial.println("Mat ket noi Wi-Fi!");
        }
        if (millis() - lastReconnectAttempt > RECONNECT_INTERVAL) {
            lastReconnectAttempt = millis();
            Serial.println("Dang co gang ket noi lai Wi-Fi...");
            WiFi.reconnect();
        }
    } else {
        if (!isOnline) {
            isOnline = true;
            updateLcdNetworkStatus(true);
            Serial.println("Da ket noi lai Wi-Fi!");
        }
    }

    if (isOnline) {
        loopMqttBroker();
        if (millis() - lastStatusPublish > STATUS_PUBLISH_INTERVAL) {
            lastStatusPublish = millis();
            publishBrokerStatus();
        }
    }

    loopLcd();

    long newEncoderPosition = encoder.getCount();
    if (newEncoderPosition != oldEncoderPosition) {
        handleEncoderRotation(newEncoderPosition);
        oldEncoderPosition = newEncoderPosition;
    }

    handleButton();
}