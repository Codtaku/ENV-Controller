#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include "SPIFFS.h"
#include "LcdUi.h" 
#include "SettingsManager.h"
#include "MqttHandler.h"

AsyncWebServer server(80);

void handleStatusRequest(AsyncWebServerRequest *request) {
    DynamicJsonDocument doc(1024);
    
    doc["ip"] = WiFi.localIP().toString();
    doc["temp"] = g_currentTemp;
    doc["hum"] = g_currentHum;
    doc["gas"] = g_currentGas;
    doc["soil"] = g_currentSoil;
    doc["mode"] = modeToString(g_currentMode);
    doc["mode_val"] = g_currentMode;

    JsonArray relays = doc.createNestedArray("relays");
    for(int i=0; i<5; i++) {
        relays.add(g_relayStatus[i]);
    }

    JsonObject settings = doc.createNestedObject("settings");
    settings["temp_min"] = g_tempMin;
    settings["temp_max"] = g_tempMax;
    settings["hum_min"] = g_humMin;
    settings["hum_max"] = g_humMax;
    settings["gas_limit"] = g_gasLimit;
    settings["relay_delay"] = g_relayDelay;

    String jsonResponse;
    serializeJson(doc, jsonResponse);
    request->send(200, "application/json", jsonResponse);
}

void handleSetRequest(AsyncWebServerRequest *request) {
    if(request->hasParam("temp_min")) g_tempMin = request->getParam("temp_min")->value().toFloat();
    if(request->hasParam("temp_max")) g_tempMax = request->getParam("temp_max")->value().toFloat();
    if(request->hasParam("hum_min")) g_humMin = request->getParam("hum_min")->value().toInt();
    if(request->hasParam("hum_max")) g_humMax = request->getParam("hum_max")->value().toInt();
    if(request->hasParam("gas_limit")) g_gasLimit = request->getParam("gas_limit")->value().toInt();
    if(request->hasParam("relay_delay")) g_relayDelay = request->getParam("relay_delay")->value().toInt();
    
    if(request->hasParam("mode")) {
        int modeVal = request->getParam("mode")->value().toInt();
        setMode((SystemMode)modeVal);

        // Nếu mode được set là MANUAL, xử lý trạng thái relay gửi kèm
        if (modeVal == MANUAL) {
            const char* relayCmdTopics[] = {
                "chuong/dieu_khien/den_1/SET", "chuong/dieu_khien/den_2/SET", 
                "chuong/dieu_khien/quat_1/SET", "chuong/dieu_khien/quat_2/SET", "chuong/dieu_khien/quat_3/SET"
            };
            for (int i = 0; i < 5; i++) {
                String paramName = "r" + String(i);
                if (request->hasParam(paramName)) {
                    bool newState = (request->getParam(paramName)->value() == "true");
                    // Chỉ gửi lệnh nếu trạng thái thay đổi
                    if (g_relayStatus[i] != newState) {
                         publishCommand(relayCmdTopics[i], newState ? "ON" : "OFF");
                    }
                }
            }
        }
    }

    saveSettings(); 
    publishSettingsUpdate();
    
    request->send(200, "text/plain", "OK");
}

void handleControlRequest(AsyncWebServerRequest *request) {
    if (request->hasParam("relay")) {
        int relayIndex = request->getParam("relay")->value().toInt();
        if (relayIndex >= 0 && relayIndex < NUM_RELAYS) {
            // Lấy trạng thái hiện tại và đảo ngược nó
            bool currentState = g_relayStatus[relayIndex];
            bool newState = !currentState;

            // >> SỬA LỖI: CẬP NHẬT TRẠNG THÁI CỤC BỘ NGAY LẬP TỨC <<
            g_relayStatus[relayIndex] = newState;
            
            // Gửi lệnh MQTT đến Node
            const char* relayCmdTopics[] = {
                "chuong/dieu_khien/den_1/SET", 
                "chuong/dieu_khien/den_2/SET", 
                "chuong/dieu_khien/quat_1/SET", 
                "chuong/dieu_khien/quat_2/SET", 
                "chuong/dieu_khien/quat_3/SET"
            };
            publishCommand(relayCmdTopics[relayIndex], newState ? "ON" : "OFF");

            // Tự động chuyển sang mode MANUAL
            setMode(MANUAL);
            
            request->send(200, "text/plain", "Command Sent");
            return;
        }
    }
    request->send(400, "text/plain", "Bad Request");
}


void initWebServer() {
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(SPIFFS, "/index.html", "text/html");
    });
    server.serveStatic("/", SPIFFS, "/");

    // API endpoints
    server.on("/status", HTTP_GET, handleStatusRequest);
    server.on("/set", HTTP_GET, handleSetRequest);
    // >> XÓA ENDPOINT /control <<
    // server.on("/control", HTTP_GET, handleControlRequest); 
    server.on("/reboot", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(200, "text/plain", "Rebooting...");
        delay(1000);
        ESP.restart();
    });

    server.begin();
    Serial.println("Web Server started.");
}