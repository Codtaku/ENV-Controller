#include <Arduino.h>
#include <WiFi.h>
#include "config.h"
#include "MqttHandler.h"
#include "LcdUi.h"
#include "SettingsManager.h"
#include "WebServerHandler.h"
#include <ESP32Encoder.h>
#include <ArduinoJson.h>
#include <DNSServer.h>
#include <ESPAsyncWebServer.h>
#include "SPIFFS.h"
#include <LiquidCrystal_I2C.h>

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

DNSServer dnsServer;
bool apMode = false; // Cờ để biết đang ở chế độ AP hay không
AsyncWebServer ap_server(80); // Server riêng cho chế độ AP

extern LiquidCrystal_I2C lcd;

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

void handleWifiSave(AsyncWebServerRequest *request) {
    g_wifi_ssid = request->arg("ssid");
    g_wifi_pass = request->arg("pass");
    saveSettings();
    request->send(200, "text/html", "<h1>Da luu Wi-Fi</h1><p>Thiet bi se khoi dong lai trong 5 giay.</p>");
    delay(5000);
    ESP.restart();
}

void startAPMode() {
    const char* ap_ssid = "Broker-Setup-Portal";
    apMode = true;
    
    WiFi.softAP(ap_ssid);
    IPAddress apIP = WiFi.softAPIP();
    Serial.printf("AP IP address: %s\n", apIP.toString().c_str());
    
    dnsServer.start(53, "*", apIP);
    
    // Đăng ký các route cho server AP
    ap_server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(SPIFFS, "/wifisetup.html", "text/html");
    });
    ap_server.on("/savewifi", HTTP_GET, handleWifiSave);
    ap_server.begin();
    
    lcd.clear();
    lcd.setCursor(0,0); lcd.print("Che do Cai dat WiFi");
    lcd.setCursor(0,1); lcd.print("Mang: "); lcd.print(ap_ssid);
    lcd.setCursor(0,2); lcd.print("IP: "); lcd.print(apIP);
    lcd.setCursor(0,3); lcd.print("Mo trinh duyet");
    // >> KHÔNG CÒN VÒNG LẶP BLOCKING Ở ĐÂY NỮA <<
}

void setup() {
    Serial.begin(115200);
    setupSettings(); 
    setupLcd(); 
    
    WiFi.mode(WIFI_STA);
    if (g_wifi_ssid.length() > 0) {
        WiFi.begin(g_wifi_ssid.c_str(), g_wifi_pass.c_str());
        Serial.print("Dang ket noi vao mang da luu");
        int attempts = 40; 
        while (WiFi.status() != WL_CONNECTED && attempts > 0) {
            delay(500);
            Serial.print(".");
            attempts--;
        }
    }

    if (WiFi.status() == WL_CONNECTED) {
        isOnline = true;
        Serial.println("\nKet noi thanh cong!");
        Serial.print("IP Address: ");
        Serial.println(WiFi.localIP());
    } else {
        isOnline = false;
        Serial.println("\nKhong the ket noi. Chuyen sang che do AP.");
        startAPMode(); 
    }

    updateLcdNetworkStatus(isOnline);
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
    // >> SỬA ĐỔI LOGIC LOOP <<
    if (apMode) {
        // Nếu đang ở chế độ AP, chỉ xử lý DNS
        dnsServer.processNextRequest();
    } else {
        // Nếu ở chế độ bình thường, chạy logic chính
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
}