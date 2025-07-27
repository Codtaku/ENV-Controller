#ifndef CONFIG_H
#define CONFIG_H

// --- CẤU HÌNH MẠNG WIFI ---
extern const char* WIFI_SSID;
extern const char* WIFI_PASSWORD;

const int MQTT_PORT = 1883;

// --- CẤU HÌNH PHẦN CỨNG ---
const int LCD_COLUMNS = 20;
const int LCD_ROWS = 4;
const int LCD_ADDRESS = 0x27;

// >> THÊM CẤU HÌNH CHO ENCODER <<
const int ENCODER_CLK_PIN = 16; // Chân CLK của encoder
const int ENCODER_DT_PIN = 17;  // Chân DT của encoder
const int ENCODER_SW_PIN = 18;  // Chân nút nhấn SW của encoder

#endif