#include <Arduino.h>
#include "SPIFFS.h" // << THAY ĐỔI
#include <ArduinoJson.h>
#include "SettingsManager.h"
#include "LcdUi.h" 

const char* CONFIG_FILE = "/config.json";

void loadSettings() {
    File configFile = SPIFFS.open(CONFIG_FILE, "r"); // << THAY ĐỔI
    if (!configFile) {
        Serial.println("Failed to open config file for reading. Using default values.");
        return;
    }

    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, configFile);
    if (error) {
        Serial.println("Failed to parse config file. Using default values.");
        return;
    }

    g_tempMin = doc["temp_min"] | 25.0;
    g_tempMax = doc["temp_max"] | 32.0;
    g_humMin = doc["hum_min"] | 60;
    g_humMax = doc["hum_max"] | 85;
    g_gasLimit = doc["gas_limit"] | 700;
    g_relayDelay = doc["relay_delay"] | 10;

    configFile.close();
    Serial.println("Settings loaded successfully.");
}

void saveSettings() {
    File configFile = SPIFFS.open(CONFIG_FILE, "w"); // << THAY ĐỔI
    if (!configFile) {
        Serial.println("Failed to open config file for writing.");
        return;
    }

    StaticJsonDocument<512> doc;
    doc["temp_min"] = g_tempMin;
    doc["temp_max"] = g_tempMax;
    doc["hum_min"] = g_humMin;
    doc["hum_max"] = g_humMax;
    doc["gas_limit"] = g_gasLimit;
    doc["relay_delay"] = g_relayDelay;

    if (serializeJson(doc, configFile) == 0) {
        Serial.println("Failed to write to config file.");
    } else {
        Serial.println("Settings saved successfully.");
    }
    configFile.close();
}


void setupSettings() {
    if (!SPIFFS.begin(true)) { // << THAY ĐỔI (true để format nếu không mount được)
        Serial.println("An Error has occurred while mounting SPIFFS");
        return;
    }
    Serial.println("SPIFFS mounted successfully.");
    loadSettings();
}