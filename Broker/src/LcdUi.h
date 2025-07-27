#ifndef LCD_UI_H
#define LCD_UI_H

// Định nghĩa các trạng thái của giao diện
enum UiMode {
    UI_MODE_STATUS,
    UI_MODE_MAIN_MENU,
    UI_MODE_EDIT_MODE,
    UI_MODE_EDIT_MANUAL,
    UI_MODE_EDIT_THRESHOLDS,
    UI_MODE_EDIT_VALUE
};

// Định nghĩa các mode hoạt động để dễ quản lý
enum SystemMode { AUTO, ON, OFF, HALF, TIMER, MANUAL };

// >> KHAI BÁO BIẾN DÙNG CHUNG VỚI "extern" <<
// Báo cho các file khác biết về sự tồn tại của các biến này
extern SystemMode g_currentMode;
extern bool g_relayStatus[5];
extern float g_tempMin, g_tempMax;
extern int g_humMin, g_humMax;
extern int g_gasLimit, g_relayDelay;
extern float g_currentTemp, g_currentHum;
extern int g_currentGas, g_currentSoil;
extern int NUM_RELAYS;


void setupLcd();
void loopLcd();

// Các hàm cập nhật dữ liệu
void updateLcdSensorData(float temp, float hum, int gas, int soil);
void updateLcdRelayStatus(int relayNum, bool status);
void updateLcdNetworkStatus(bool isOnline);
void setMode(SystemMode newMode);

// Các hàm xử lý tương tác từ encoder
void handleEncoderRotation(long newPosition);
void handleEncoderClick();

// Hàm tiện ích
const char* modeToString(SystemMode mode);

#endif