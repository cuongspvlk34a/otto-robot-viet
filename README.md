# Chatbot AI dựa trên MCP

([Tiếng Việt](README.md) | [中文](README_zh.md) | [日本語](README_ja.md) | )

## Giới thiệu

👉 [Con người: Cho AI xem camera vs AI: Phát hiện ngay chủ nhà chưa gội đầu 3 ngày【bilibili】](https://www.bilibili.com/video/BV1bpjgzKEhd/)

👉 [Tự tay làm bạn gái AI, hướng dẫn cho người mới bắt đầu【bilibili】](https://www.bilibili.com/video/BV1XnmFYLEJN/)

XiaoZhi AI Chatbot là một thiết bị tương tác bằng giọng nói, tận dụng khả năng AI của các mô hình ngôn ngữ lớn như Qwen / DeepSeek, và thực hiện điều khiển đa thiết bị thông qua giao thức MCP.

<img src="docs/mcp-based-graph.jpg" alt="Điều khiển mọi thứ qua MCP" width="320">

## Ghi chú phiên bản

Phiên bản v2 hiện tại không tương thích với bảng phân vùng của v1, do đó không thể nâng cấp từ v1 lên v2 qua OTA. Xem chi tiết bảng phân vùng tại <partitions/v2/README.md>.

Tất cả phần cứng đang chạy v1 có thể nâng cấp lên v2 bằng cách nạp firmware thủ công.

Phiên bản ổn định của v1 là 1.9.2. Bạn có thể chuyển sang v1 bằng lệnh `git checkout v1`. Nhánh v1 sẽ được duy trì đến tháng 2 năm 2026.

### Các tính năng đã triển khai

- Wi-Fi / ML307 Cat.1 4G
- Kích hoạt giọng nói ngoại tuyến [ESP-SR](https://github.com/espressif/esp-sr)
- Hỗ trợ hai giao thức truyền thông ([Websocket](docs/websocket.md) hoặc MQTT+UDP)
- Sử dụng codec âm thanh OPUS
- Tương tác giọng nói dựa trên kiến trúc streaming ASR + LLM + TTS
- Nhận dạng giọng nói, xác định người nói hiện tại [3D Speaker](https://github.com/modelscope/3D-Speaker)
- Màn hình OLED / LCD, hỗ trợ hiển thị emoji
- Hiển thị pin và quản lý nguồn điện
- Hỗ trợ đa ngôn ngữ (Tiếng Trung, Tiếng Anh, Tiếng Nhật)
- Hỗ trợ các nền tảng chip ESP32-C3, ESP32-S3, ESP32-P4
- MCP phía thiết bị để điều khiển phần cứng (Loa, LED, Servo, GPIO, v.v.)
- MCP phía đám mây để mở rộng khả năng mô hình lớn (điều khiển nhà thông minh, thao tác màn hình PC, tìm kiếm tri thức, email, v.v.)
- Tùy chỉnh từ kích hoạt, phông chữ, emoji và hình nền qua công cụ chỉnh sửa trực tuyến ([Custom Assets Generator](https://github.com/78/xiaozhi-assets-generator))

## Phần cứng

### Thực hành DIY với Breadboard

Xem hướng dẫn tài liệu Feishu:

👉 [“Bách khoa toàn thư XiaoZhi AI Chatbot”](https://ccnphfhqs21z.feishu.cn/wiki/F5krwD16viZoF0kKkvDcrZNYnhb?from=from_copylink)

Demo breadboard:

![Demo Breadboard](docs/v1/wiring2.jpg)

### Hỗ trợ hơn 70 phần cứng mã nguồn mở (danh sách một phần)

- <a href="https://oshwhub.com/li-chuang-kai-fa-ban/li-chuang-shi-zhan-pai-esp32-s3-kai-fa-ban" target="_blank" title="LiChuang ESP32-S3 Development Board">Bo mạch phát triển LiChuang ESP32-S3</a>
- <a href="https://github.com/espressif/esp-box" target="_blank" title="Espressif ESP32-S3-BOX3">Espressif ESP32-S3-BOX3</a>
- <a href="https://docs.m5stack.com/zh_CN/core/CoreS3" target="_blank" title="M5Stack CoreS3">M5Stack CoreS3</a>
- <a href="https://docs.m5stack.com/en/atom/Atomic%20Echo%20Base" target="_blank" title="AtomS3R + Echo Base">M5Stack AtomS3R + Echo Base</a>
- <a href="https://gf.bilibili.com/item/detail/1108782064" target="_blank" title="Magic Button 2.4">Magic Button 2.4</a>
- <a href="https://www.waveshare.net/shop/ESP32-S3-Touch-AMOLED-1.8.htm" target="_blank" title="Waveshare ESP32-S3-Touch-AMOLED-1.8">Waveshare ESP32-S3-Touch-AMOLED-1.8</a>
- <a href="https://github.com/Xinyuan-LilyGO/T-Circle-S3" target="_blank" title="LILYGO T-Circle-S3">LILYGO T-Circle-S3</a>
- <a href="https://oshwhub.com/tenclass01/xmini_c3" target="_blank" title="XiaGe Mini C3">XiaGe Mini C3</a>
- <a href="https://oshwhub.com/movecall/cuican-ai-pendant-lights-up-y" target="_blank" title="Movecall CuiCan ESP32S3">Mặt dây chuyền AI CuiCan</a>
- <a href="https://github.com/WMnologo/xingzhi-ai" target="_blank" title="WMnologo-Xingzhi-1.54">WMnologo-Xingzhi-1.54TFT</a>
- <a href="https://www.seeedstudio.com/SenseCAP-Watcher-W1-A-p-5979.html" target="_blank" title="SenseCAP Watcher">SenseCAP Watcher</a>
- <a href="https://www.bilibili.com/video/BV1BHJtz6E2S/" target="_blank" title="ESP-HI Low Cost Robot Dog">Robot chó chi phí thấp ESP-HI</a>

<div style="display: flex; justify-content: space-between;">
  <a href="docs/v1/lichuang-s3.jpg" target="_blank" title="LiChuang ESP32-S3 Development Board">
    <img src="docs/v1/lichuang-s3.jpg" width="240" />
  </a>
  <a href="docs/v1/espbox3.jpg" target="_blank" title="Espressif ESP32-S3-BOX3">
    <img src="docs/v1/espbox3.jpg" width="240" />
  </a>
  <a href="docs/v1/m5cores3.jpg" target="_blank" title="M5Stack CoreS3">
    <img src="docs/v1/m5cores3.jpg" width="240" />
  </a>
  <a href="docs/v1/atoms3r.jpg" target="_blank" title="AtomS3R + Echo Base">
    <img src="docs/v1/atoms3r.jpg" width="240" />
  </a>
  <a href="docs/v1/magiclick.jpg" target="_blank" title="Magic Button 2.4">
    <img src="docs/v1/magiclick.jpg" width="240" />
  </a>
  <a href="docs/v1/waveshare.jpg" target="_blank" title="Waveshare ESP32-S3-Touch-AMOLED-1.8">
    <img src="docs/v1/waveshare.jpg" width="240" />
  </a>
  <a href="docs/v1/lilygo-t-circle-s3.jpg" target="_blank" title="LILYGO T-Circle-S3">
    <img src="docs/v1/lilygo-t-circle-s3.jpg" width="240" />
  </a>
  <a href="docs/v1/xmini-c3.jpg" target="_blank" title="XiaGe Mini C3">
    <img src="docs/v1/xmini-c3.jpg" width="240" />
  </a>
  <a href="docs/v1/movecall-cuican-esp32s3.jpg" target="_blank" title="CuiCan">
    <img src="docs/v1/movecall-cuican-esp32s3.jpg" width="240" />
  </a>
  <a href="docs/v1/wmnologo_xingzhi_1.54.jpg" target="_blank" title="WMnologo-Xingzhi-1.54">
    <img src="docs/v1/wmnologo_xingzhi_1.54.jpg" width="240" />
  </a>
  <a href="docs/v1/sensecap_watcher.jpg" target="_blank" title="SenseCAP Watcher">
    <img src="docs/v1/sensecap_watcher.jpg" width="240" />
  </a>
  <a href="docs/v1/esp-hi.jpg" target="_blank" title="ESP-HI Low Cost Robot Dog">
    <img src="docs/v1/esp-hi.jpg" width="240" />
  </a>
</div>

## Phần mềm

### Nạp Firmware

Người mới bắt đầu nên sử dụng firmware có thể nạp mà không cần thiết lập môi trường phát triển.

Firmware mặc định kết nối đến máy chủ chính thức [xiaozhi.me](https://xiaozhi.me). Người dùng cá nhân có thể đăng ký tài khoản để sử dụng mô hình Qwen thời gian thực miễn phí.

👉 [Hướng dẫn nạp firmware cho người mới bắt đầu](https://ccnphfhqs21z.feishu.cn/wiki/Zpz4wXBtdimBrLk25WdcXzxcnNS)

### Môi trường phát triển

- Cursor hoặc VSCode
- Cài đặt plugin ESP-IDF, chọn phiên bản SDK 5.4 trở lên
- Linux tốt hơn Windows về tốc độ biên dịch và ít vấn đề driver hơn
- Dự án này sử dụng phong cách code C++ của Google, vui lòng đảm bảo tuân thủ khi nộp code

### Tài liệu dành cho nhà phát triển

- [Hướng dẫn tạo bo mạch tùy chỉnh](docs/custom-board.md) - Tìm hiểu cách tạo bo mạch tùy chỉnh cho XiaoZhi AI
- [Hướng dẫn sử dụng điều khiển IoT qua giao thức MCP](docs/mcp-usage.md) - Tìm hiểu cách điều khiển thiết bị IoT qua giao thức MCP
- [Luồng tương tác giao thức MCP](docs/mcp-protocol.md) - Triển khai giao thức MCP phía thiết bị
- [Tài liệu giao thức truyền thông lai MQTT + UDP](docs/mqtt-udp.md)
- [Tài liệu chi tiết giao thức truyền thông WebSocket](docs/websocket.md)

## Cấu hình mô hình ngôn ngữ lớn

Nếu bạn đã có thiết bị XiaoZhi AI Chatbot và đã kết nối với máy chủ chính thức, bạn có thể đăng nhập vào bảng điều khiển [xiaozhi.me](https://xiaozhi.me) để cấu hình.

👉 [Video hướng dẫn vận hành backend (Giao diện cũ)](https://www.bilibili.com/video/BV1jUCUY2EKM/)

## Các dự án mã nguồn mở liên quan

Để triển khai máy chủ trên máy tính cá nhân, tham khảo các dự án mã nguồn mở sau:

- [xinnan-tech/xiaozhi-esp32-server](https://github.com/xinnan-tech/xiaozhi-esp32-server) Máy chủ Python
- [joey-zhou/xiaozhi-esp32-server-java](https://github.com/joey-zhou/xiaozhi-esp32-server-java) Máy chủ Java
- [AnimeAIChat/xiaozhi-server-go](https://github.com/AnimeAIChat/xiaozhi-server-go) Máy chủ Golang
- [hackers365/xiaozhi-esp32-server-golang](https://github.com/hackers365/xiaozhi-esp32-server-golang) Máy chủ Golang

Các dự án client khác sử dụng giao thức truyền thông XiaoZhi:

- [huangjunsen0406/py-xiaozhi](https://github.com/huangjunsen0406/py-xiaozhi) Client Python
- [TOM88812/xiaozhi-android-client](https://github.com/TOM88812/xiaozhi-android-client) Client Android
- [100askTeam/xiaozhi-linux](http://github.com/100askTeam/xiaozhi-linux) Client Linux bởi 100ask
- [78/xiaozhi-sf32](https://github.com/78/xiaozhi-sf32) Firmware chip Bluetooth của Tứ Xuyên
- [QuecPython/solution-xiaozhiAI](https://github.com/QuecPython/solution-xiaozhiAI) Firmware QuecPython bởi Quectel

Công cụ tùy chỉnh nội dung:

- [78/xiaozhi-assets-generator](https://github.com/78/xiaozhi-assets-generator) Trình tạo nội dung tùy chỉnh (Từ kích hoạt, phông chữ, emoji, hình nền)

## Về dự án

Đây là dự án ESP32 mã nguồn mở, phát hành theo giấy phép MIT, cho phép bất kỳ ai sử dụng miễn phí, kể cả cho mục đích thương mại.

Chúng tôi hy vọng dự án này giúp mọi người hiểu về phát triển phần cứng AI và ứng dụng các mô hình ngôn ngữ lớn đang phát triển nhanh chóng vào các thiết bị phần cứng thực tế.

Nếu bạn có bất kỳ ý tưởng hoặc đề xuất nào, hãy thoải mái đặt Issues hoặc tham gia [Discord](https://discord.gg/C759fGMBcZ) hoặc nhóm QQ: 994694848

## Lịch sử Star

<a href="https://star-history.com/#78/xiaozhi-esp32&Date">
 <picture>
   <source media="(prefers-color-scheme: dark)" srcset="https://api.star-history.com/svg?repos=78/xiaozhi-esp32&type=Date&theme=dark" />
   <source media="(prefers-color-scheme: light)" srcset="https://api.star-history.com/svg?repos=78/xiaozhi-esp32&type=Date" />
   <img alt="Biểu đồ lịch sử Star" src="https://api.star-history.com/svg?repos=78/xiaozhi-esp32&type=Date" />
 </picture>
</a>
