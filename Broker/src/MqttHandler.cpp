#include <Arduino.h>
#include <WiFi.h>
#include <PicoMQTT.h>
#include <ArduinoJson.h>
#include "config.h"
#include "MqttHandler.h"
#include "SettingsManager.h"
#include "LcdUi.h"

PicoMQTT::Server mqtt_server;

void subscribeToTopics() {
    mqtt_server.subscribe("#", [](const char *topic, const char *payload) {
        String topic_str = String(topic);
        String payload_str = String(payload);
        
        Serial.printf("Received message on topic: %s\n", topic_str.c_str());
        Serial.printf("Payload: %s\n", payload_str.c_str());

        if (topic_str == "chuong/cam_bien") {
            StaticJsonDocument<256> doc;
            deserializeJson(doc, payload_str);
            updateLcdSensorData(doc["nhietdo"], doc["doam_kk"], doc["khi_gas"], doc["doam_dat"]);
        } 
        else if (topic_str == "chuong/node/settings") {
            StaticJsonDocument<256> doc;
            deserializeJson(doc, payload_str);
            g_tempMin = doc["temp_min"];
            g_tempMax = doc["temp_max"];
            g_humMin = doc["hum_min"];
            g_humMax = doc["hum_max"];
            g_gasLimit = doc["gas_limit"];
            g_relayDelay = doc["relay_delay"];
            saveSettings();
        }
        else if (topic_str == "chuong/node/request") {
            StaticJsonDocument<128> doc;
            deserializeJson(doc, payload_str);

            // >> PHẦN SỬA ĐỔI BẮT ĐẦU TỪ ĐÂY <<
            // Chuyển giá trị JSON thành const char* trước khi so sánh
            const char* request_val = doc["request"]; 
            if (strcmp(request_val, "GET_SETTINGS") == 0) {
            // >> PHẦN SỬA ĐỔI KẾT THÚC TẠI ĐÂY <<
                Serial.println("Node requested settings. Publishing...");
                publishSettingsUpdate();
            }
        }
        else if (topic_str == "chuong/node/event") {
            StaticJsonDocument<128> doc;
            deserializeJson(doc, payload_str);
            const char* event_type = doc["event_type"];

            if (strcmp(event_type, "SENSOR_ERROR") == 0) {
                Serial.println("CRITICAL: Sensor error reported from Node!");
                setMode(OFF);
            } 
            // >> PHẦN SỬA ĐỔI BẮT ĐẦU TỪ ĐÂY <<
            else if (strcmp(event_type, "BUTTON_PRESS") == 0) {
                const char* value = doc["value"];
                if (strcmp(value, "REQUEST_AUTO") == 0) {
                    Serial.println("Node requested AUTO mode.");
                    setMode(AUTO);
                } else if (strcmp(value, "REQUEST_ON") == 0) {
                    Serial.println("Node requested ON mode.");
                    setMode(ON);
                } else if (strcmp(value, "REQUEST_OFF") == 0) {
                    Serial.println("Node requested OFF mode.");
                    setMode(OFF);
                } else if (strcmp(value, "REQUEST_HALF") == 0) {
                    Serial.println("Node requested HALF mode.");
                    setMode(HALF);
                } else if (strcmp(value, "REQUEST_TIMER") == 0) {
                    Serial.println("Node requested TIMER mode.");
                    setMode(TIMER);
                } else if (strcmp(value, "REQUEST_MANUAL") == 0) {
                    Serial.println("Node requested MANUAL mode with relay states.");
                    
                    // >> PHẦN SỬA ĐỔI BẮT ĐẦU TỪ ĐÂY <<
                    // 1. Kiểm tra xem key "relay_status" có tồn tại và có phải là một mảng không
                    if (doc.containsKey("relay_status") && doc["relay_status"].is<JsonArray>()) {
                        JsonArray relayStates = doc["relay_status"];
                        // 2. Cập nhật trạng thái relay từ payload
                        for (int i = 0; i < relayStates.size() && i < 5; i++) {
                            g_relayStatus[i] = relayStates[i].as<bool>();
                        }
                        setMode(MANUAL);
                    } else {
                        Serial.println("ERROR: REQUEST_MANUAL is missing or has invalid relay_status array.");
                    }
                    // >> PHẦN SỬA ĐỔI KẾT THÚC TẠI ĐÂY <<
                }
            }
            // >> PHẦN SỬA ĐỔI KẾT THÚC TẠI ĐÂY <<
            else if (strcmp(event_type, "BOOT") == 0) {
                Serial.println("Node has rebooted.");
            }
        }
        else if (topic_str.startsWith("chuong/node/relay/")) {
            // Tách tên relay từ topic, ví dụ: "den_1" từ "chuong/node/relay/den_1/STATE"
            topic_str.remove(0, String("chuong/node/relay/").length());
            String relay_name = topic_str.substring(0, topic_str.indexOf('/'));
            
            bool new_status = (payload_str == "ON");

            // Tìm relay tương ứng và cập nhật
            if (relay_name == "den_1") updateLcdRelayStatus(0, new_status);
            else if (relay_name == "den_2") updateLcdRelayStatus(1, new_status);
            else if (relay_name == "quat_1") updateLcdRelayStatus(2, new_status);
            else if (relay_name == "quat_2") updateLcdRelayStatus(3, new_status);
            else if (relay_name == "quat_3") updateLcdRelayStatus(4, new_status);
        }
    });
}

void setupMqttBroker() {
    subscribeToTopics();
    mqtt_server.begin();
    Serial.println("PicoMQTT Broker is running!");
}

void loopMqttBroker() {
    mqtt_server.loop();
}

void publishCommand(const char* topic, const char* payload) {
    Serial.printf("Publishing to %s: %s\n", topic, payload);
    mqtt_server.publish(topic, payload);
}

void publishSettingsUpdate() {
    StaticJsonDocument<256> doc;
    
    doc["temp_min"] = g_tempMin;
    doc["temp_max"] = g_tempMax;
    doc["hum_min"] = g_humMin;
    doc["hum_max"] = g_humMax;
    doc["gas_limit"] = g_gasLimit;
    doc["relay_delay"] = g_relayDelay;

    char json_output[256];
    serializeJson(doc, json_output);

    publishCommand("chuong/broker/settings", json_output);
}