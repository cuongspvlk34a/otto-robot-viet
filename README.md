# 🤖 Otto Robot Việt — AI Chatbot trên ESP32-S3

[![ESP-IDF](https://img.shields.io/badge/ESP--IDF-5.5.3-blue?logo=espressif)](https://github.com/espressif/esp-idf)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![Board](https://img.shields.io/badge/Board-ESP32--S3-green)](https://www.espressif.com/en/products/socs/esp32-s3)
[![Language](https://img.shields.io/badge/Ngôn%20ngữ-Tiếng%20Việt-red)](main/assets/locales/vi-VN)

Dự án Việt hóa firmware **[xiaozhi-esp32](https://github.com/78/xiaozhi-esp32)** để chạy trên robot Otto với ESP32-S3.  
Robot có thể nói chuyện bằng **tiếng Việt** thông qua AI của [xiaozhi.me](https://xiaozhi.me).

> ⚠️ **Lưu ý:** Backend vẫn là server `xiaozhi.me`. Bạn cần tạo tài khoản tại [xiaozhi.me](https://xiaozhi.me) để sử dụng. Dự án này chỉ Việt hóa giao diện, comment và ngôn ngữ mặc định — không thay đổi backend.

---

## ✨ Tính năng

- 🗣️ Giao tiếp bằng tiếng Việt (locale `vi-VN` mặc định)
- 🤖 Chuyển động robot Otto điều khiển qua servo SG90 (4 servo)
- 📡 Kết nối WiFi, cấu hình qua BLE (BluFi)
- 😊 Hiển thị biểu cảm emoji trên màn hình OLED
- 🔊 Nhận lệnh giọng nói, phản hồi âm thanh
- 🏠 Hỗ trợ MCP cho smart home automation

---

## 🛒 Danh sách phần cứng

| Linh kiện | Số lượng | Ghi chú |
|-----------|----------|---------|
| ESP32-S3 module (N16R8 khuyến nghị) | 1 | Hoặc dev board có mic INMP441 |
| Servo SG90 | 4 | 2 chân + 2 chân robot Otto |
| Khung robot Otto | 1 | In 3D hoặc mua kit, tìm "Otto Robot 3D print" |
| Màn hình OLED SSD1306 128×64 | 1 | Giao tiếp I2C |
| Micro INMP441 | 1 | I2S, có sẵn trên một số board |
| Loa nhỏ + khuếch đại | 1 | MAX98357A hoặc tương đương |
| Pin LiPo 3.7V 1000mAh | 1 | Tùy chọn nếu dùng pin |
| Dây jumper, PCB | — | Kết nối linh kiện |

> Tham khảo wiring trong `main/boards/otto-robot/config.h`

---

## ⚙️ Cài đặt môi trường

### Yêu cầu
- Windows 10/11 (64-bit)
- ESP-IDF **5.5.3** (bắt buộc đúng phiên bản)
- Python 3.8+
- Git

### Bước 1 — Cài ESP-IDF 5.5.3

Tải ESP-IDF Windows Installer từ trang chính thức:  
👉 https://github.com/espressif/esp-idf/releases/tag/v5.5.3

Chạy file `esp-idf-tools-setup-offline-5.5.3.exe`, chọn ESP32-S3 target.  
Sau cài đặt, mở **ESP-IDF Command Prompt** (shortcut trong Start Menu).

### Bước 2 — Clone repo này

```bash
git clone https://github.com/YOUR_USERNAME/otto-robot-viet.git
cd otto-robot-viet
```

### Bước 3 — Cài dependencies

```bash
idf.py --version          # Kiểm tra: phải ra 5.5.3
pip install idf-component-manager --break-system-packages
```

---

## 🔨 Build và Flash

### Chọn board Otto Robot

```bash
idf.py set-target esp32s3
idf.py menuconfig
```

Trong menuconfig: **Xiaozhi Configuration → Board Type → otto-robot**

### Build

```bash
idf.py build
```

Lần đầu build sẽ mất 5–15 phút (tải component từ idf-component-registry).

### Flash

```bash
idf.py -p COM_PORT flash monitor
```

Thay `COM_PORT` bằng cổng thực tế (xem trong Device Manager, ví dụ `COM3`).

> Nếu lỗi permission: giữ nút BOOT trên board khi bắt đầu flash.

---

## 📶 Cấu hình WiFi và kết nối xiaozhi.me

### Bước 1 — Tạo tài khoản
Đăng ký tại: https://xiaozhi.me  
Tạo device mới, lấy **Device ID** và **Token**.

### Bước 2 — Cấu hình WiFi qua BLE (BluFi)
1. Cài app **EspBluFi** (Android/iOS)
2. Bật robot, đèn LED nhấp nháy = chờ kết nối BLE
3. Mở app → Scan → Chọn `otto-robot-XXXX`
4. Nhập SSID + Password WiFi → Send

### Bước 3 — Nhập Device Credentials
Sau khi có WiFi, robot tự kết nối `xiaozhi.me`.  
Nếu cần nhập Device ID thủ công, nói lệnh: **"Cài đặt"** rồi làm theo hướng dẫn âm thanh.

---

## 📁 Cấu trúc dự án

```
otto-robot-viet/
├── main/
│   ├── boards/otto-robot/     # Board config, servo, movement, display
│   ├── assets/locales/vi-VN/  # Chuỗi tiếng Việt
│   └── ...
├── docs/                      # Tài liệu kỹ thuật
├── scripts/                   # Công cụ build, debug
├── CMakeLists.txt
├── sdkconfig.defaults.esp32s3
└── README.md
```

---

## 🐛 Báo lỗi / Đặt câu hỏi

Dùng **Issues** trên GitHub, theo template có sẵn.  
Vui lòng đính kèm: phiên bản firmware, log Serial, mô tả phần cứng.

---

## 🙏 Credits & License

Dự án này là **derivative work** từ:

- **[xiaozhi-esp32](https://github.com/78/xiaozhi-esp32)** — Copyright (c) 2025 Shenzhen Xinzhi Future Technology Co., Ltd.
- Giấy phép gốc: **MIT License** (giữ nguyên, xem file [LICENSE](LICENSE))

Những thay đổi trong repo này:
- Việt hóa toàn bộ comment, log, README
- Thêm locale `vi-VN` làm mặc định
- Target board `otto-robot` (ESP32-S3)

Phần Việt hóa: Copyright (c) 2025 [Tên của bạn]

---

<sub>Made with ❤️ for the Vietnamese maker community</sub>
