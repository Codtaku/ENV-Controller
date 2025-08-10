#ifndef MQTT_HANDLER_H
#define MQTT_HANDLER_H

void setupMqttBroker();
void loopMqttBroker();

void publishCommand(const char* topic, const char* payload);

// >> HÀM MỚI: Gửi đi toàn bộ các thông số cài đặt <<
void publishSettingsUpdate();

#endif