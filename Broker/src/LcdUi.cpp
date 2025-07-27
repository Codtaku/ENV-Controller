#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include "config.h"
#include "LcdUi.h"
#include "MqttHandler.h"
#include "SettingsManager.h"

LiquidCrystal_I2C lcd(LCD_ADDRESS, LCD_COLUMNS, LCD_ROWS);

UiMode currentUiMode = UI_MODE_STATUS;
int statusPage = 0;
const int MAX_STATUS_PAGES = 4;
int mainMenu_index = 0;
const int MAIN_MENU_ITEMS = 6;
int editMenu_index = 0;
int editing_param_index = 0;

float g_currentTemp = 0.0, g_currentHum = 0.0;
int g_currentGas = 0, g_currentSoil = 0;
bool g_isOnline = false;
SystemMode g_currentMode = AUTO;
bool g_relayStatus[5] = {false};

float g_tempMin = 25.0, g_tempMax = 32.0;
int g_humMin = 60, g_humMax = 85;
int g_gasLimit = 700;
int g_relayDelay = 10;
float g_edit_value;
int NUM_RELAYS = 5;
const char *relayNames[] = {"Den 1", "Den 2", "Quat 1", "Quat 2", "Quat 3"};
const char *relayCmdTopics[] = {"chuong/dieu_khien/den_1/SET", "chuong/dieu_khien/den_2/SET", "chuong/dieu_khien/quat_1/SET", "chuong/dieu_khien/quat_2/SET", "chuong/dieu_khien/quat_3/SET"};


const char *paramNames[] = {"Nhiet do Min", "Nhiet do Max", "Do am Min", "Do am Max", "Gioi han Gas", "Do tre Relay"};
const int NUM_PARAMS = 6;

unsigned long lastLcdUpdate = 0;
const int LCD_UPDATE_INTERVAL = 2000;

// Khai báo các hàm cục bộ
void drawStatusScreen();
void drawMainMenu();
void drawModeMenu();
void drawManualControlMenu();
void drawThresholdsMenu();
void drawEditValueScreen();

const char *modeToString(SystemMode mode)
{
    switch (mode)
    {
    case AUTO:
        return "AUTO";
    case ON:
        return "ON";
    case OFF:
        return "OFF";
    case HALF:
        return "HALF";
    case TIMER:
        return "TIMER";
    case MANUAL:
        return "MANUAL";
    default:
        return "N/A";
    }
}

void setMode(SystemMode newMode)
{
    if (g_currentMode == newMode)
        return;
    if (g_currentMode == MANUAL && newMode != MANUAL)
    {
        Serial.println("Exiting MANUAL mode. Resetting all relays to OFF.");
        for (int i = 0; i < NUM_RELAYS; i++)
        {
            if (g_relayStatus[i])
            {
                g_relayStatus[i] = false; // Cập nhật trạng thái cục bộ
                publishCommand(relayCmdTopics[i], "OFF");
            }
        }
    }
    g_currentMode = newMode;
    publishCommand("chuong/broker/mode", modeToString(g_currentMode));
    Serial.printf("Mode changed to: %s\n", modeToString(g_currentMode));
}

void setupLcd()
{
    lcd.init();
    lcd.backlight();
    lcd.setCursor(0, 0);
    lcd.print("He thong Broker");
    lcd.setCursor(0, 1);
    lcd.print("Dang khoi dong...");
    delay(2000);
}

void loopLcd()
{
    if (currentUiMode == UI_MODE_STATUS)
    {
        if (millis() - lastLcdUpdate > LCD_UPDATE_INTERVAL)
        {
            lastLcdUpdate = millis();
            drawStatusScreen();
        }
    }
}

void drawStatusScreen()
{
    char buffer[21];
    lcd.clear();
    switch (statusPage)
    {
    case 0:
        snprintf(buffer, sizeof(buffer), "HH:MM%s %s", g_isOnline ? "[ONLINE]" : "[OFFLINE]", modeToString(g_currentMode));
        lcd.setCursor(0, 0);
        lcd.print(buffer);
        snprintf(buffer, sizeof(buffer), "Nhiet:%.1f*C Am:%.1f%%", g_currentTemp, g_currentHum);
        lcd.setCursor(0, 1);
        lcd.print(buffer);
        snprintf(buffer, sizeof(buffer), "Gas:%dppm Dat:%d%%", g_currentGas, g_currentSoil);
        lcd.setCursor(0, 2);
        lcd.print(buffer);
        lcd.setCursor(0, 3);
        lcd.print("<<   Trang 1/4    >>");
        break;
    case 1:
        lcd.setCursor(0, 0);
        lcd.print("TRANG THAI RELAY 1/2");
        lcd.setCursor(0, 1);
        lcd.print("Den 1:    ");
        lcd.print(g_relayStatus[0] ? "ON" : "OFF");
        lcd.setCursor(0, 2);
        lcd.print("Den 2:    ");
        lcd.print(g_relayStatus[1] ? "ON" : "OFF");
        lcd.setCursor(0, 3);
        lcd.print("Quat 1:   ");
        lcd.print(g_relayStatus[2] ? "ON" : "OFF");
        break;
    case 2:
        lcd.setCursor(0, 0);
        lcd.print("TRANG THAI RELAY 2/2");
        lcd.setCursor(0, 1);
        lcd.print("Quat 2:   ");
        lcd.print(g_relayStatus[3] ? "ON" : "OFF");
        lcd.setCursor(0, 2);
        lcd.print("Quat 3:   ");
        lcd.print(g_relayStatus[4] ? "ON" : "OFF");
        lcd.setCursor(0, 3);
        lcd.print("<<   Trang 3/4    >>");
        break;
    case 3:
        lcd.setCursor(0, 0);
        lcd.print("THONG TIN MANG");
        lcd.setCursor(0, 1);
        lcd.print("IP: " + WiFi.localIP().toString());
        lcd.setCursor(0, 2);
        lcd.print("Wi-Fi: " + String(WiFi.RSSI()) + "dBm");
        lcd.setCursor(0, 3);
        lcd.print("<<   Trang 4/4    >>");
        break;
    }
}

void drawMainMenu()
{
    lcd.clear();
    const char *menuItems[] = {"Che do Hoat dong", "Dieu khien Tay", "Cai dat Nguong", "Ket noi lai Mang", "Reset WiFi", "Quay lai"};
    lcd.setCursor(0, 0);
    lcd.print("MENU CHINH");
    int startItem = 0;
    if (mainMenu_index >= 3)
        startItem = mainMenu_index - 2;
    for (int i = 0; i < 3; i++)
    {
        int itemIndex = startItem + i;
        lcd.setCursor(0, i + 1);
        if (itemIndex < MAIN_MENU_ITEMS)
        {
            lcd.print((itemIndex == mainMenu_index) ? ">" : " ");
            lcd.print(menuItems[itemIndex]);
        }
        else
        {
            lcd.print("                    ");
        }
    }
}

void drawModeMenu()
{
    lcd.clear();
    const char *modeItems[] = {"AUTO", "ON", "OFF", "HALF", "TIMER"};
    const int numSelectableModes = 5;
    lcd.setCursor(0, 0);
    lcd.print("Chon Che do:");
    int startItem = 0;
    if (editMenu_index >= 3)
        startItem = editMenu_index - 2;
    for (int i = 0; i < 3; i++)
    {
        int itemIndex = startItem + i;
        lcd.setCursor(0, i + 1);
        if (itemIndex < numSelectableModes)
        {
            lcd.print((itemIndex == editMenu_index) ? ">" : " ");
            lcd.print(modeItems[itemIndex]);
        }
        else
        {
            lcd.print("                    ");
        }
    }
}

void drawManualControlMenu()
{
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Dieu khien Tay:");
    int startItem = 0;
    if (editMenu_index >= 3)
        startItem = editMenu_index - 2;
    const int manualMenuItems = NUM_RELAYS + 1;
    for (int i = 0; i < 3; i++)
    {
        int itemIndex = startItem + i;
        lcd.setCursor(0, i + 1);
        if (itemIndex < manualMenuItems)
        {
            lcd.print((itemIndex == editMenu_index) ? ">" : " ");
            if (itemIndex < NUM_RELAYS)
            {
                char buffer[21];
                snprintf(buffer, sizeof(buffer), "%-8s: %s", relayNames[itemIndex], g_relayStatus[itemIndex] ? "ON" : "OFF");
                lcd.print(buffer);
            }
            else
            {
                lcd.print("Quay lai");
            }
        }
        else
        {
            lcd.print("                    ");
        }
    }
}

void drawThresholdsMenu()
{
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Cai dat Nguong:");
    int startItem = 0;
    if (editMenu_index >= 3)
        startItem = editMenu_index - 2;
    const int thresholdMenuItems = NUM_PARAMS + 1;
    for (int i = 0; i < 3; i++)
    {
        int itemIndex = startItem + i;
        lcd.setCursor(0, i + 1);
        if (itemIndex < thresholdMenuItems)
        {
            lcd.print((itemIndex == editMenu_index) ? ">" : " ");
            if (itemIndex < NUM_PARAMS)
            {
                lcd.print(paramNames[itemIndex]);
            }
            else
            {
                lcd.print("Quay lai");
            }
        }
        else
        {
            lcd.print("                    ");
        }
    }
}

void drawEditValueScreen()
{
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Dang chinh sua:");
    lcd.setCursor(0, 1);
    lcd.print(paramNames[editing_param_index]);
    lcd.setCursor(0, 3);
    char buffer[21];
    if (editing_param_index < 2)
    { // Temp Min/Max là float
        snprintf(buffer, sizeof(buffer), "<      %.1f      >", g_edit_value);
    }
    else
    { // Các thông số còn lại là int
        snprintf(buffer, sizeof(buffer), "<      %d      >", (int)g_edit_value);
    }
    lcd.print(buffer);
}

void handleEncoderRotation(long newPosition)
{
    static long oldPosition = 0;
    int direction = (newPosition > oldPosition) ? 1 : -1;
    switch (currentUiMode)
    {
    case UI_MODE_STATUS:
        statusPage = (statusPage + direction + MAX_STATUS_PAGES) % MAX_STATUS_PAGES;
        drawStatusScreen();
        break;
    case UI_MODE_MAIN_MENU:
        mainMenu_index += direction;
        if (mainMenu_index < 0)
            mainMenu_index = 0;
        if (mainMenu_index >= MAIN_MENU_ITEMS)
            mainMenu_index = MAIN_MENU_ITEMS - 1;
        drawMainMenu();
        break;
    case UI_MODE_EDIT_MODE:
        editMenu_index += direction;
        if (editMenu_index < 0)
            editMenu_index = 0;
        if (editMenu_index >= 5)
            editMenu_index = 4;
        drawModeMenu();
        break;
    case UI_MODE_EDIT_MANUAL:
        editMenu_index += direction;
        if (editMenu_index < 0)
            editMenu_index = 0;
        if (editMenu_index >= NUM_RELAYS + 1)
            editMenu_index = NUM_RELAYS;
        drawManualControlMenu();
        break;
    case UI_MODE_EDIT_THRESHOLDS:
        editMenu_index += direction;
        if (editMenu_index < 0)
            editMenu_index = 0;
        if (editMenu_index >= NUM_PARAMS + 1)
            editMenu_index = NUM_PARAMS;
        drawThresholdsMenu();
        break;
    case UI_MODE_EDIT_VALUE:
        float step = (editing_param_index < 2) ? 0.5 : 1.0;
        g_edit_value += (direction * step);
        if (editing_param_index == 0 && g_edit_value > g_tempMax)
            g_edit_value = g_tempMax;
        if (editing_param_index == 1 && g_edit_value < g_tempMin)
            g_edit_value = g_tempMin;
        if (editing_param_index == 2 && g_edit_value > g_humMax)
            g_edit_value = g_humMax;
        if (editing_param_index == 3 && g_edit_value < g_humMin)
            g_edit_value = g_humMin;
        drawEditValueScreen();
        break;
    }
    oldPosition = newPosition;
}

void handleEncoderClick()
{
    switch (currentUiMode)
    {
    case UI_MODE_STATUS:
        currentUiMode = UI_MODE_MAIN_MENU;
        mainMenu_index = 0;
        drawMainMenu();
        break;
    case UI_MODE_MAIN_MENU:
        if (mainMenu_index == 0)
        {
            currentUiMode = UI_MODE_EDIT_MODE;
            editMenu_index = (g_currentMode == MANUAL) ? AUTO : g_currentMode;
            drawModeMenu();
        }
        else if (mainMenu_index == 1)
        {
            currentUiMode = UI_MODE_EDIT_MANUAL;
            editMenu_index = 0;
            drawManualControlMenu();
        }
        else if (mainMenu_index == 2)
        {
            currentUiMode = UI_MODE_EDIT_THRESHOLDS;
            editMenu_index = 0;
            drawThresholdsMenu();
        }
        else if (mainMenu_index == 4)
        {
            lcd.clear();
            lcd.print("Dang xoa WiFi...");
            WiFiManager wm;
            wm.resetSettings();
            delay(1000);
            lcd.print("Khoi dong lai!");
            delay(1000);
            ESP.restart();
        }
        else if (mainMenu_index == MAIN_MENU_ITEMS - 1)
        {
            currentUiMode = UI_MODE_STATUS;
            drawStatusScreen();
        }
        else
        {
            lcd.clear();
            lcd.print("Chuc nang Phat trien");
            delay(1500);
            drawMainMenu();
        }
        break;
    case UI_MODE_EDIT_MODE:
        setMode((SystemMode)editMenu_index);
        currentUiMode = UI_MODE_MAIN_MENU;
        drawMainMenu();
        break;
    case UI_MODE_EDIT_MANUAL:
        if (editMenu_index < NUM_RELAYS)
        {
            g_relayStatus[editMenu_index] = !g_relayStatus[editMenu_index];
            publishCommand(relayCmdTopics[editMenu_index], g_relayStatus[editMenu_index] ? "ON" : "OFF");
            setMode(MANUAL);
            drawManualControlMenu();
        }
        else
        {
            currentUiMode = UI_MODE_MAIN_MENU;
            drawMainMenu();
        }
        break;
    case UI_MODE_EDIT_THRESHOLDS:
        if (editMenu_index < NUM_PARAMS)
        {
            editing_param_index = editMenu_index;
            switch (editing_param_index)
            {
            case 0:
                g_edit_value = g_tempMin;
                break;
            case 1:
                g_edit_value = g_tempMax;
                break;
            case 2:
                g_edit_value = g_humMin;
                break;
            case 3:
                g_edit_value = g_humMax;
                break;
            case 4:
                g_edit_value = g_gasLimit;
                break;
            case 5:
                g_edit_value = g_relayDelay;
                break;
            }
            currentUiMode = UI_MODE_EDIT_VALUE;
            drawEditValueScreen();
        }
        else
        {
            currentUiMode = UI_MODE_MAIN_MENU;
            drawMainMenu();
        }
        break;
    case UI_MODE_EDIT_VALUE:
        switch (editing_param_index)
        {
        case 0:
            g_tempMin = g_edit_value;
            break;
        case 1:
            g_tempMax = g_edit_value;
            break;
        case 2:
            g_humMin = (int)g_edit_value;
            break;
        case 3:
            g_humMax = (int)g_edit_value;
            break;
        case 4:
            g_gasLimit = (int)g_edit_value;
            break;
        case 5:
            g_relayDelay = (int)g_edit_value;
            break;
        }
        saveSettings();
        publishSettingsUpdate();
        currentUiMode = UI_MODE_EDIT_THRESHOLDS;
        drawThresholdsMenu();
        break;
    }
}

void updateLcdSensorData(float temp, float hum, int gas, int soil)
{
    g_currentTemp = temp;
    g_currentHum = hum;
    g_currentGas = gas;
    g_currentSoil = soil;
}
void updateLcdRelayStatus(int relayNum, bool status)
{
    if (relayNum >= 0 && relayNum < 5)
        g_relayStatus[relayNum] = status;
}
void updateLcdNetworkStatus(bool isOnline) { g_isOnline = isOnline; }