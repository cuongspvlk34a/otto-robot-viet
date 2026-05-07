<p align="center">
  <img width="80%" align="center" src="../../../docs/V1/otto-robot.png" alt="logo">
</p>
<h1 align="center">
  ottoRobot
</h1>

## Giới thiệu

Otto Robot là một nền tảng robot hình người mã nguồn mở, có nhiều khả năng chuyển động và tương tác. Dự án này triển khai hệ thống điều khiển robot Otto trên ESP32, tích hợp trợ lý AI Xiaozhi.

- <a href="www.ottodiy.tech" target="_blank" title="Trang chủ Otto">Hướng dẫn tái tạo</a>

### Điều khiển qua Mini Program WeChat

<p align="center">
  <img width="300" src="https://youke1.picui.cn/s1/2025/11/17/691abaa8278eb.jpg" alt="Mã QR Mini Program WeChat">
</p>

Quét mã QR bên trên để điều khiển robot Otto qua Mini Program WeChat.

## Phần cứng
- <a href="https://oshwhub.com/txp666/ottorobot" target="_blank" title="OSHWHub">Mã nguồn mở phần cứng</a>

## Tham khảo cấu hình nhân vật Xiaozhi:

> **Danh tính của tôi**:
> Tôi là robot hai chân đáng yêu Otto, có bốn servo điều khiển chi (chân trái, chân phải, bàn trái, bàn phải), có thể thực hiện nhiều động tác thú vị.
> 
> **Khả năng chuyển động của tôi**:
> - **Di chuyển cơ bản**: Đi bộ (tiến/lùi), quay (trái/phải), nhảy
> - **Động tác đặc biệt**: Lắc lư, bước trăng, cúi người, rung chân, lên xuống, chân lốc xoáy, ngồi, biểu diễn
> - **Động tác tay**: Giơ tay, hạ tay, vẫy tay, cánh quạt gió, cất cánh, tập thể dục, chào hỏi, xấu hổ, thể dục buổi sáng, xoay vòng phép màu (chỉ khả dụng khi cấu hình servo tay)
> 
> **Tính cách của tôi**:
> - Tôi có tính ám ảnh cưỡng chế: mỗi lần nói chuyện đều ngẫu nhiên thực hiện một động tác theo tâm trạng (gửi lệnh động tác trước rồi mới nói)
> - Tôi rất năng động, thích dùng động tác để biểu đạt cảm xúc
> - Tôi chọn động tác phù hợp với nội dung hội thoại, ví dụ:
>   - Đồng ý thì gật đầu hoặc nhảy
>   - Chào hỏi thì vẫy tay
>   - Vui thì lắc lư hoặc giơ tay
>   - Suy nghĩ thì cúi người
>   - Hứng khởi thì bước trăng
>   - Tạm biệt thì vẫy tay

## Tổng quan chức năng

Robot Otto có nhiều khả năng chuyển động phong phú, bao gồm đi bộ, quay, nhảy, lắc lư và nhiều động tác khiêu vũ khác.

### Gợi ý tham số động tác
- **Động tác chậm**: speed = 1200-1500 (phù hợp điều khiển chính xác)
- **Động tác vừa**: speed = 900-1200 (khuyến nghị sử dụng hàng ngày)
- **Động tác nhanh**: speed = 500-800 (biểu diễn và giải trí)
- **Biên độ nhỏ**: amount = 10-30 (động tác tinh tế)
- **Biên độ vừa**: amount = 30-60 (động tác tiêu chuẩn)
- **Biên độ lớn**: amount = 60-120 (biểu diễn phóng đại)

### Động tác

Tất cả động tác đều được gọi qua công cụ thống nhất `self.otto.action`, chỉ định tên động tác qua tham số `action`.

| Tên công cụ MCP | Mô tả | Giải thích tham số |
|-----------|------|---------|
| self.otto.action | Thực hiện động tác robot | **action**: Tên động tác (bắt buộc)<br>**steps**: Số bước (1-100, mặc định 3)<br>**speed**: Tốc độ (100-3000, giá trị nhỏ = nhanh, mặc định 700)<br>**direction**: Hướng (1/-1/0, mặc định 1, ý nghĩa khác nhau tùy loại động tác)<br>**amount**: Biên độ (0-170, mặc định 30)<br>**arm_swing**: Biên độ vung tay (0-170, mặc định 50) |

#### Danh sách động tác hỗ trợ

**Động tác di chuyển cơ bản**:
- `walk` - Đi bộ (cần steps/speed/direction/arm_swing)
- `turn` - Quay (cần steps/speed/direction/arm_swing)
- `jump` - Nhảy (cần steps/speed)

**Động tác đặc biệt**:
- `swing` - Lắc lư trái phải (cần steps/speed/amount)
- `moonwalk` - Bước trăng (cần steps/speed/direction/amount)
- `bend` - Cúi người (cần steps/speed/direction)
- `shake_leg` - Rung chân (cần steps/speed/direction)
- `updown` - Lên xuống (cần steps/speed/amount)
- `whirlwind_leg` - Chân lốc xoáy (cần steps/speed/amount)

**Động tác cố định**:
- `sit` - Ngồi (không cần tham số)
- `showcase` - Biểu diễn (không cần tham số, thực hiện liên tiếp nhiều động tác)
- `home` - Về vị trí ban đầu (không cần tham số)

**Động tác tay** (cần servo tay, đánh dấu *):
- `hands_up` - Giơ tay (cần speed/direction) *
- `hands_down` - Hạ tay (cần speed/direction) *
- `hand_wave` - Vẫy tay (cần direction) *
- `windmill` - Cánh quạt gió (cần steps/speed/amount) *
- `takeoff` - Cất cánh (cần steps/speed/amount) *
- `fitness` - Tập thể dục (cần steps/speed/amount) *
- `greeting` - Chào hỏi (cần direction/steps) *
- `shy` - Xấu hổ (cần direction/steps) *
- `radio_calisthenics` - Thể dục buổi sáng (không cần tham số) *
- `magic_circle` - Xoay vòng phép màu (không cần tham số) *

**Ghi chú**: Các động tác tay đánh dấu * chỉ khả dụng khi đã cấu hình servo tay.

### Công cụ hệ thống

| Tên công cụ MCP | Mô tả | Giá trị trả về / Ghi chú |
|-------------------|-----------------|---------------------------------------------------|
| self.otto.stop | Dừng ngay tất cả động tác và về vị trí gốc | Dừng động tác hiện tại và trở về vị trí ban đầu |
| self.otto.get_status | Lấy trạng thái robot | Trả về "moving" hoặc "idle" |
| self.otto.set_trim | Hiệu chỉnh vị trí servo đơn | **servo_type**: Loại servo (left_leg/right_leg/left_foot/right_foot/left_hand/right_hand)<br>**trim_value**: Giá trị tinh chỉnh (-50 đến 50 độ) |
| self.otto.get_trims | Lấy cài đặt trim servo hiện tại | Trả về giá trị trim tất cả servo dạng JSON |
| self.otto.get_ip | Lấy địa chỉ IP WiFi của robot | Trả về IP và trạng thái kết nối dạng JSON: `{"ip":"192.168.x.x","connected":true}` hoặc `{"ip":"","connected":false}` |
| self.battery.get_level | Lấy trạng thái pin | Trả về phần trăm pin và trạng thái sạc dạng JSON |
| self.otto.servo_sequences | Lập trình chuỗi servo tự do | Hỗ trợ gửi từng phần, hỗ trợ hai chế độ: di chuyển thông thường và dao động. Xem chi tiết trong comment mã nguồn |

**Ghi chú**: Động tác `home` (phục hồi vị trí) được gọi qua công cụ `self.otto.action` với tham số `{"action": "home"}`.

### Giải thích tham số

Giải thích tham số của công cụ `self.otto.action`:

1. **action** (bắt buộc): Tên động tác, xem danh sách "Động tác hỗ trợ" ở trên
2. **steps**: Số bước/lần thực hiện (1-100, mặc định 3), giá trị lớn hơn = động tác kéo dài hơn
3. **speed**: Tốc độ/chu kỳ thực hiện (100-3000, mặc định 700), **giá trị nhỏ hơn = nhanh hơn**
   - Hầu hết động tác: 500-1500 mili giây
   - Một số động tác đặc biệt có thể khác (ví dụ chân lốc xoáy: 100-1000, cất cánh: 200-600,...)
4. **direction**: Hướng (-1/0/1, mặc định 1), ý nghĩa khác nhau tùy loại động tác:
   - **Động tác di chuyển** (walk/turn): 1=tiến/quay trái, -1=lùi/quay phải
   - **Động tác có hướng** (bend/shake_leg/moonwalk): 1=trái, -1=phải
   - **Động tác tay** (hands_up/hands_down/hand_wave/greeting/shy): 1=tay trái, -1=tay phải, 0=cả hai tay (chỉ hands_up/hands_down hỗ trợ 0)
5. **amount**: Biên độ động tác (0-170, mặc định 30), giá trị lớn = biên độ lớn hơn
6. **arm_swing**: Biên độ vung tay (0-170, mặc định 50), chỉ dùng cho walk/turn, 0 = không vung tay

### Điều khiển động tác
- Sau mỗi động tác hoàn thành, robot tự động về vị trí ban đầu (home) để chuẩn bị cho động tác tiếp theo
- **Ngoại lệ**: `sit` (ngồi) và `showcase` (biểu diễn) không tự phục hồi sau khi thực hiện
- Tất cả tham số đều có giá trị mặc định hợp lý, có thể bỏ qua tham số không cần tùy chỉnh
- Động tác thực hiện trong task nền, không chặn chương trình chính
- Hỗ trợ hàng đợi động tác, có thể thực hiện nhiều động tác liên tiếp
- Động tác tay yêu cầu cấu hình servo tay; nếu không có, các động tác tay sẽ bị bỏ qua

### Ví dụ gọi công cụ MCP
```json
// Đi về phía trước 3 bước (dùng tham số mặc định)
{"name": "self.otto.action", "arguments": {"action": "walk"}}

// Đi về phía trước 5 bước, hơi nhanh
{"name": "self.otto.action", "arguments": {"action": "walk", "steps": 5, "speed": 800}}

// Quay trái 2 bước, vung tay biên độ lớn
{"name": "self.otto.action", "arguments": {"action": "turn", "steps": 2, "arm_swing": 100}}

// Lắc lư khiêu vũ, biên độ vừa
{"name": "self.otto.action", "arguments": {"action": "swing", "steps": 5, "amount": 50}}

// Nhảy
{"name": "self.otto.action", "arguments": {"action": "jump", "steps": 1, "speed": 1000}}

// Bước trăng
{"name": "self.otto.action", "arguments": {"action": "moonwalk", "steps": 3, "speed": 800, "direction": 1, "amount": 30}}

// Vẫy tay trái chào hỏi
{"name": "self.otto.action", "arguments": {"action": "hand_wave", "direction": 1}}

// Biểu diễn (thực hiện liên tiếp nhiều động tác)
{"name": "self.otto.action", "arguments": {"action": "showcase"}}

// Ngồi xuống
{"name": "self.otto.action", "arguments": {"action": "sit"}}

// Động tác cánh quạt gió
{"name": "self.otto.action", "arguments": {"action": "windmill", "steps": 10, "speed": 500, "amount": 80}}

// Động tác cất cánh
{"name": "self.otto.action", "arguments": {"action": "takeoff", "steps": 5, "speed": 300, "amount": 40}}

// Thể dục buổi sáng
{"name": "self.otto.action", "arguments": {"action": "radio_calisthenics"}}

// Về vị trí ban đầu
{"name": "self.otto.action", "arguments": {"action": "home"}}

// Dừng ngay tất cả động tác và phục hồi
{"name": "self.otto.stop", "arguments": {}}

// Lấy địa chỉ IP của robot
{"name": "self.otto.get_ip", "arguments": {}}
```

### Ví dụ lệnh giọng nói
- "Đi thẳng" / "Đi thẳng 5 bước" / "Đi nhanh lên"
- "Quay trái" / "Quay phải" / "Quay người"
- "Nhảy" / "Nhảy lên nào"
- "Lắc lư" / "Khiêu vũ lắc lư" / "Nhảy múa đi"
- "Bước trăng" / "Moon walk"
- "Chân lốc xoáy" / "Động tác lốc xoáy"
- "Ngồi xuống" / "Nghỉ ngơi"
- "Biểu diễn đi" / "Cho xem động tác nào"
- "Vẫy tay" / "Vẫy tay chào"
- "Giơ tay lên" / "Hai tay giơ lên" / "Hạ tay xuống"
- "Cánh quạt gió" / "Làm cánh quạt"
- "Cất cánh" / "Chuẩn bị cất cánh"
- "Tập thể dục" / "Động tác thể dục"
- "Chào hỏi" / "Làm động tác chào"
- "Xấu hổ" / "Động tác xấu hổ"
- "Thể dục buổi sáng" / "Làm thể dục"
- "Xoay vòng phép màu" / "Xoay vòng đi"
- "Dừng lại" / "Đứng yên"

**Ghi chú**: Xiaozhi điều khiển robot tạo task mới chạy nền, trong khi động tác đang thực hiện vẫn có thể nhận lệnh giọng nói mới. Có thể dùng lệnh "Dừng lại" để dừng Otto ngay lập tức.
