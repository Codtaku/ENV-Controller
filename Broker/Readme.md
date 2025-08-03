# 🤖 Project: Broker Controller (Bộ điều khiển Trung tâm)

Đây là firmware cho thiết bị Broker, đóng vai trò là "bộ não" trung tâm của hệ thống điều khiển môi trường IoT. Nó chịu trách nhiệm điều phối toàn bộ hoạt động, giao tiếp với các thiết bị Node, và cung cấp giao diện tương tác cho người dùng.

## ✨ Tính năng chính

- **MQTT Broker Tích hợp:** Chạy một MQTT broker (PicoMQTT) ngay trên ESP32 để quản lý toàn bộ giao tiếp trong mạng nội bộ.
- **Giao diện Điều khiển Kép:**
    - **Cục bộ:** Màn hình LCD 2004 và núm xoay encoder cho phép giám sát và cấu hình toàn bộ hệ thống mà không cần các thiết bị khác.
    - **Từ xa:** Giao diện Web (Web UI) nhẹ, cho phép giám sát và cài đặt qua trình duyệt trên cùng mạng Wi-Fi.
- **Quản lý Cấu hình Tập trung:** Lưu trữ, quản lý và đồng bộ hóa các thông số cài đặt (ngưỡng nhiệt độ, độ ẩm...) cho toàn hệ thống.
- **Lưu trữ Bền bỉ:** Mọi cài đặt được lưu vào file `config.json` trên hệ thống file SPIFFS, không bị mất khi mất điện.
- **Cơ chế Kết nối Mạng Thông minh:** Tự động chuyển sang chế độ Access Point (AP) để cài đặt Wi-Fi khi không thể kết nối vào mạng đã lưu.

## 🔌 Phần cứng
- **Vi điều khiển:** ESP32 Dev Kit
- **Hiển thị:** Màn hình LCD 2004 (I2C)
- **Đầu vào:** Núm xoay Encoder có nút nhấn

## 📁 Cấu trúc Phần mềm
Code được chia thành các module độc lập để dễ quản lý và bảo trì:
- `main.cpp`: Luồng chính, điều phối hoạt động của các module.
- `LcdUi.cpp/.h`: Quản lý toàn bộ giao diện và logic của màn hình LCD và encoder.
- `MqttHandler.cpp/.h`: Xử lý tất cả các hoạt động của MQTT Broker và giao tiếp MQTT.
- `WebServerHandler.cpp/.h`: Chạy Web Server và xử lý các yêu cầu API từ giao diện web.
- `SettingsManager.cpp/.h`: Quản lý việc đọc/ghi file cấu hình `config.json` trên SPIFFS.
- `config.h`: Chứa các định nghĩa về chân cắm và các hằng số.

## 📡 Giao diện Giao tiếp MQTT
Broker vừa là Server, vừa là Client. Nó vừa lắng nghe tin nhắn từ các Node, vừa gửi đi các lệnh và thông báo trạng thái.

### Tin nhắn Nhận về (Subscribed Topics)
| Topic | Ví dụ Payload | Mô tả |
| :--- | :--- | :--- |
| `chuong/cam_bien` | `{"nhietdo":28.5, ...}` | Nhận dữ liệu cảm biến từ Node để hiển thị. |
| `chuong/node/relay/+/STATE` | `"ON"` | Nhận trạng thái **thực tế đã được xác nhận** của một relay từ Node. Dấu `+` là wildcard cho tên relay. |
| `chuong/node/settings` | `{"temp_min":26.0, ...}` | Nhận bộ cài đặt mới do người dùng thay đổi cục bộ trên Node và lưu lại. |
| `chuong/node/event` | `{"event_type":"SENSOR_ERROR",...}` | Nhận các sự kiện đặc biệt (lỗi, yêu cầu đổi mode...) từ Node và ra quyết định xử lý. |
| `chuong/node/request` | `{"request":"GET_SETTINGS"}` | Nhận yêu cầu gửi lại bộ cài đặt từ một Node vừa khởi động. |

### Tin nhắn Gửi đi (Published Topics)
| Topic | Ví dụ Payload | Mô tả |
| :--- | :--- | :--- |
| `chuong/dieu_khien/{thiet_bi}/SET` | `"ON"` | Gửi **lệnh** yêu cầu Node bật/tắt một thiết bị cụ thể. |
| `chuong/broker/settings` | `{"temp_min":25.0, ...}` | Gửi đi **toàn bộ** bộ cài đặt mới nhất mỗi khi có thay đổi (từ LCD, Web, hoặc từ Node khác). |
| `chuong/broker/mode` | `"MANUAL"` | Gửi đi **chế độ hoạt động** mới của toàn hệ thống. |
| `chuong/broker/status` | `{"mode":"AUTO", "relays":[...], ...}` | Gửi báo cáo trạng thái tổng hợp một cách định kỳ để các client luôn được đồng bộ. |