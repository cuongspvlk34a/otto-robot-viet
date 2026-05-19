/*
    Bộ điều khiển Robot Otto - Phiên bản giao thức MCP
*/

#include <cJSON.h>
#include <esp_log.h>

#include <cstdlib> 
#include <cstring>

#include "application.h"
#include "board.h"
#include "config.h"
#include "mcp_server.h"
#include "otto_movements.h"
#include "power_manager.h"
#include "sdkconfig.h"
#include "settings.h"
#include <wifi_manager.h>

#define TAG "OttoController"

class OttoController {
private:
    Otto otto_;
    TaskHandle_t action_task_handle_ = nullptr;
    QueueHandle_t action_queue_;
    bool has_hands_ = false;
    bool is_action_in_progress_ = false;

    struct OttoActionParams {
        int action_type;
        int steps;
        int speed;
        int direction;
        int amount;
        char servo_sequence_json[512];  // Lưu chuỗi JSON của chuỗi servo
    };

    enum ActionType {
        ACTION_WALK = 1,
        ACTION_TURN = 2,
        ACTION_JUMP = 3,
        ACTION_SWING = 4,
        ACTION_MOONWALK = 5,
        ACTION_BEND = 6,
        ACTION_SHAKE_LEG = 7,
        ACTION_SIT = 25,  // Ngồi xuống
        ACTION_RADIO_CALISTHENICS = 26,  // Thể dục buổi sáng
        ACTION_MAGIC_CIRCLE = 27,  // Xoay vòng phép màu
        ACTION_UPDOWN = 8,
        ACTION_TIPTOE_SWING = 9,
        ACTION_JITTER = 10,
        ACTION_ASCENDING_TURN = 11,
        ACTION_CRUSAITO = 12,
        ACTION_FLAPPING = 13,
        ACTION_HANDS_UP = 14,
        ACTION_HANDS_DOWN = 15,
        ACTION_HAND_WAVE = 16,
        ACTION_WINDMILL = 20,  // Cánh quạt gió
        ACTION_TAKEOFF = 21,   // Cất cánh
        ACTION_FITNESS = 22,   // Tập thể dục
        ACTION_GREETING = 23,  // Chào hỏi
        ACTION_SHY = 24,        // Xấu hổ
        ACTION_SHOWCASE = 28,   // Biểu diễn động tác
        ACTION_HOME = 17,
        ACTION_SERVO_SEQUENCE = 18,  // Chuỗi servo (tự lập trình)
        ACTION_WHIRLWIND_LEG = 19,   // Chân lốc xoáy
        // -- Điệu nhảy mở rộng
        ACTION_SAMBA = 29,        // Điệu Samba
        ACTION_BALLET = 30,       // Điệu Ballet
        ACTION_HULA = 31,         // Điệu Hula Hawaii
        ACTION_TWIST = 32,        // Điệu Twist
        ACTION_ROBOT_DANCE = 33   // Điệu Robot
    };

    static void ActionTask(void* arg) {
        OttoController* controller = static_cast<OttoController*>(arg);
        OttoActionParams params;
        controller->otto_.AttachServos();

        while (true) {
            if (xQueueReceive(controller->action_queue_, &params, pdMS_TO_TICKS(1000)) == pdTRUE) {
                ESP_LOGI(TAG, "Thực hiện động tác: %d", params.action_type);
                PowerManager::PauseBatteryUpdate();  // Tạm dừng cập nhật pin khi bắt đầu động tác
                controller->is_action_in_progress_ = true;
                if (params.action_type == ACTION_SERVO_SEQUENCE) {
                    // Thực hiện chuỗi servo (tự lập trình) - chỉ hỗ trợ định dạng tên khóa ngắn
                    cJSON* json = cJSON_Parse(params.servo_sequence_json);
                    if (json != nullptr) {
                        ESP_LOGD(TAG, "Phân tích JSON thành công, độ dài=%d", strlen(params.servo_sequence_json));
                        // Dùng tên khóa ngắn "a" để chỉ mảng động tác
                        cJSON* actions = cJSON_GetObjectItem(json, "a");
                        if (cJSON_IsArray(actions)) {
                            int array_size = cJSON_GetArraySize(actions);
                            ESP_LOGI(TAG, "Thực hiện chuỗi servo, tổng %d động tác", array_size);
                            
                            // Lấy độ trễ sau khi thực hiện xong chuỗi (tên khóa ngắn "d", tham số cấp cao nhất)
                            int sequence_delay = 0;
                            cJSON* delay_item = cJSON_GetObjectItem(json, "d");
                            if (cJSON_IsNumber(delay_item)) {
                                sequence_delay = delay_item->valueint;
                                if (sequence_delay < 0) sequence_delay = 0;
                            }
                            
                            // Khởi tạo vị trí servo hiện tại (giữ nguyên vị trí servo không được chỉ định)
                            int current_positions[SERVO_COUNT];
                            for (int j = 0; j < SERVO_COUNT; j++) {
                                current_positions[j] = 90;  // Vị trí mặc định ở giữa
                            }
                            // Vị trí mặc định của servo tay
                            current_positions[LEFT_HAND] = 45;
                            current_positions[RIGHT_HAND] = 180 - 45;
                            
                            for (int i = 0; i < array_size; i++) {
                                cJSON* action_item = cJSON_GetArrayItem(actions, i);
                                if (cJSON_IsObject(action_item)) {
                                    // Kiểm tra chế độ dao động (tên khóa ngắn "osc")
                                    cJSON* osc_item = cJSON_GetObjectItem(action_item, "osc");
                                    if (cJSON_IsObject(osc_item)) {
                                        // Chế độ dao động - dùng Execute2, dao động quanh góc tuyệt đối làm tâm
                                        int amplitude[SERVO_COUNT] = {0};
                                        int center_angle[SERVO_COUNT] = {0};
                                        double phase_diff[SERVO_COUNT] = {0};
                                        int period = 300;  // Chu kỳ mặc định 300 mili giây
                                        float steps = 8.0;  // Số bước mặc định 8.0
                                        
                                        const char* servo_names[] = {"ll", "rl", "lf", "rf", "lh", "rh"};
                                        
                                        // Đọc biên độ (tên khóa ngắn "a"), mặc định 0 độ
                                        for (int j = 0; j < SERVO_COUNT; j++) {
                                            amplitude[j] = 0;  // Biên độ mặc định 0 độ
                                        }
                                        cJSON* amp_item = cJSON_GetObjectItem(osc_item, "a");
                                        if (cJSON_IsObject(amp_item)) {
                                            for (int j = 0; j < SERVO_COUNT; j++) {
                                                cJSON* amp_value = cJSON_GetObjectItem(amp_item, servo_names[j]);
                                                if (cJSON_IsNumber(amp_value)) {
                                                    int amp = amp_value->valueint;
                                                    if (amp >= 10 && amp <= 90) {
                                                        amplitude[j] = amp;
                                                    }
                                                }
                                            }
                                        }
                                        
                                        // Đọc góc tâm (tên khóa ngắn "o"), mặc định 90 độ (góc tuyệt đối 0-180 độ)
                                        for (int j = 0; j < SERVO_COUNT; j++) {
                                            center_angle[j] = 90;  // Góc tâm mặc định 90 độ (vị trí giữa)
                                        }
                                        cJSON* center_item = cJSON_GetObjectItem(osc_item, "o");
                                        if (cJSON_IsObject(center_item)) {
                                            for (int j = 0; j < SERVO_COUNT; j++) {
                                                cJSON* center_value = cJSON_GetObjectItem(center_item, servo_names[j]);
                                                if (cJSON_IsNumber(center_value)) {
                                                    int center = center_value->valueint;
                                                    if (center >= 0 && center <= 180) {
                                                        center_angle[j] = center;
                                                    }
                                                }
                                            }
                                        }
                                        
                                        // Kiểm tra an toàn: ngăn chân trái và chân phải dao động biên độ lớn đồng thời
                                        const int LARGE_AMPLITUDE_THRESHOLD = 40;  // Ngưỡng biên độ lớn: 40 độ
                                        bool left_leg_large = amplitude[LEFT_LEG] >= LARGE_AMPLITUDE_THRESHOLD;
                                        bool right_leg_large = amplitude[RIGHT_LEG] >= LARGE_AMPLITUDE_THRESHOLD;
                                        bool left_foot_large = amplitude[LEFT_FOOT] >= LARGE_AMPLITUDE_THRESHOLD;
                                        bool right_foot_large = amplitude[RIGHT_FOOT] >= LARGE_AMPLITUDE_THRESHOLD;
                                        
                                        if (left_leg_large && right_leg_large) {
                                            ESP_LOGW(TAG, "Phát hiện chân trái và phải dao động biên độ lớn đồng thời, giới hạn biên độ chân phải");
                                            amplitude[RIGHT_LEG] = 0;  // Tắt dao động chân phải
                                        }
                                        if (left_foot_large && right_foot_large) {
                                            ESP_LOGW(TAG, "Phát hiện bàn trái và phải dao động biên độ lớn đồng thời, giới hạn biên độ bàn phải");
                                            amplitude[RIGHT_FOOT] = 0;  // Tắt dao động bàn phải
                                        }
                                        
                                        // Đọc độ lệch pha (tên khóa ngắn "ph", đơn vị độ, chuyển sang radian)
                                        cJSON* phase_item = cJSON_GetObjectItem(osc_item, "ph");
                                        if (cJSON_IsObject(phase_item)) {
                                            for (int j = 0; j < SERVO_COUNT; j++) {
                                                cJSON* phase_value = cJSON_GetObjectItem(phase_item, servo_names[j]);
                                                if (cJSON_IsNumber(phase_value)) {
                                                    // Chuyển đổi từ độ sang radian
                                                    phase_diff[j] = phase_value->valuedouble * 3.141592653589793 / 180.0;
                                                }
                                            }
                                        }
                                        
                                        // Đọc chu kỳ (tên khóa ngắn "p"), phạm vi 100-3000 mili giây
                                        cJSON* period_item = cJSON_GetObjectItem(osc_item, "p");
                                        if (cJSON_IsNumber(period_item)) {
                                            period = period_item->valueint;
                                            if (period < 100) period = 100;
                                            if (period > 3000) period = 3000;  // Giới hạn 3000ms theo mô tả
                                        }
                                        
                                        // Đọc số chu kỳ (tên khóa ngắn "c"), phạm vi 0.1-20.0
                                        cJSON* steps_item = cJSON_GetObjectItem(osc_item, "c");
                                        if (cJSON_IsNumber(steps_item)) {
                                            steps = (float)steps_item->valuedouble;
                                            if (steps < 0.1) steps = 0.1;
                                            if (steps > 20.0) steps = 20.0;  // Giới hạn 20.0 theo mô tả
                                        }
                                        
                                        // Thực hiện dao động - dùng Execute2, tâm là góc tuyệt đối
                                        ESP_LOGI(TAG, "Thực hiện dao động %d: period=%d, steps=%.1f", i, period, steps);
                                        controller->otto_.Execute2(amplitude, center_angle, period, phase_diff, steps);
                                        
                                        // Cập nhật vị trí sau dao động (dùng center_angle làm vị trí cuối)
                                        for (int j = 0; j < SERVO_COUNT; j++) {
                                            current_positions[j] = center_angle[j];
                                        }
                                    } else {
                                        // Chế độ di chuyển thông thường
                                        // Sao chép từ mảng vị trí hiện tại, giữ nguyên servo không được chỉ định
                                        int servo_target[SERVO_COUNT];
                                        for (int j = 0; j < SERVO_COUNT; j++) {
                                            servo_target[j] = current_positions[j];
                                        }
                                        
                                        // Đọc vị trí servo từ JSON (tên khóa ngắn "s")
                                        cJSON* servos_item = cJSON_GetObjectItem(action_item, "s");
                                        if (cJSON_IsObject(servos_item)) {
                                            // Tên khóa ngắn: ll/rl/lf/rf/lh/rh
                                            const char* servo_names[] = {"ll", "rl", "lf", "rf", "lh", "rh"};
                                            
                                            for (int j = 0; j < SERVO_COUNT; j++) {
                                                cJSON* servo_value = cJSON_GetObjectItem(servos_item, servo_names[j]);
                                                if (cJSON_IsNumber(servo_value)) {
                                                    int position = servo_value->valueint;
                                                    // Giới hạn vị trí trong phạm vi 0-180 độ
                                                    if (position >= 0 && position <= 180) {
                                                        servo_target[j] = position;
                                                    }
                                                }
                                            }
                                        }
                                                                                                                    
                                        // Lấy tốc độ di chuyển (tên khóa ngắn "v", mặc định 1000 mili giây)
                                        int speed = 1000;
                                        cJSON* speed_item = cJSON_GetObjectItem(action_item, "v");
                                        if (cJSON_IsNumber(speed_item)) {
                                            speed = speed_item->valueint;
                                            if (speed < 100) speed = 100;  // Tối thiểu 100ms
                                            if (speed > 3000) speed = 3000;  // Tối đa 3000ms
                                        }
                                        
                                        // Thực hiện di chuyển servo
                                        ESP_LOGI(TAG, "Thực hiện động tác %d: ll=%d, rl=%d, lf=%d, rf=%d, v=%d",
                                                 i, servo_target[LEFT_LEG], servo_target[RIGHT_LEG],
                                                 servo_target[LEFT_FOOT], servo_target[RIGHT_FOOT], speed);
                                        controller->otto_.MoveServos(speed, servo_target);
                                        
                                        // Cập nhật mảng vị trí hiện tại cho động tác tiếp theo
                                        for (int j = 0; j < SERVO_COUNT; j++) {
                                            current_positions[j] = servo_target[j];
                                        }
                                    }
                                    
                                    // Lấy thời gian trễ sau động tác (tên khóa ngắn "d")
                                    int delay_after = 0;
                                    cJSON* delay_item = cJSON_GetObjectItem(action_item, "d");
                                    if (cJSON_IsNumber(delay_item)) {
                                        delay_after = delay_item->valueint;
                                        if (delay_after < 0) delay_after = 0;
                                    }
                                    
                                    // Trễ sau động tác (không trễ sau động tác cuối cùng)
                                    if (delay_after > 0 && i < array_size - 1) {
                                        ESP_LOGI(TAG, "Động tác %d hoàn thành, trễ %d ms", i, delay_after);
                                        vTaskDelay(pdMS_TO_TICKS(delay_after));
                                    }
                                }
                            }
                            
                            // Trễ sau khi hoàn thành chuỗi (dùng để nghỉ giữa các chuỗi)
                            if (sequence_delay > 0) {
                                // Kiểm tra xem còn chuỗi nào đang chờ trong queue không
                                UBaseType_t queue_count = uxQueueMessagesWaiting(controller->action_queue_);
                                if (queue_count > 0) {
                                    ESP_LOGI(TAG, "Chuỗi hoàn thành, trễ %d ms trước chuỗi tiếp theo (còn %d chuỗi trong queue)", 
                                             sequence_delay, queue_count);
                                    vTaskDelay(pdMS_TO_TICKS(sequence_delay));
                                }
                            }
                            // Giải phóng bộ nhớ JSON
                            cJSON_Delete(json);
                        } else {
                            ESP_LOGE(TAG, "Định dạng chuỗi servo sai: 'a' không phải mảng");
                            cJSON_Delete(json);
                        }
                    } else {
                        // Lấy thông tin lỗi từ cJSON
                        const char* error_ptr = cJSON_GetErrorPtr();
                        int json_len = strlen(params.servo_sequence_json);
                        ESP_LOGE(TAG, "Phân tích JSON chuỗi servo thất bại, độ dài=%d, vị trí lỗi: %s", json_len, 
                                 error_ptr ? error_ptr : "không xác định");
                        ESP_LOGE(TAG, "Nội dung JSON: %s", params.servo_sequence_json);
                    }
                } else {
                    // Thực hiện động tác định sẵn
                    switch (params.action_type) {
                        case ACTION_WALK:
                            controller->otto_.Walk(params.steps, params.speed, params.direction,
                                                   params.amount);
                            break;
                        case ACTION_TURN:
                            controller->otto_.Turn(params.steps, params.speed, params.direction,
                                                   params.amount);
                            break;
                        case ACTION_JUMP:
                            controller->otto_.Jump(params.steps, params.speed);
                            break;
                        case ACTION_SWING:
                            controller->otto_.Swing(params.steps, params.speed, params.amount);
                            break;
                        case ACTION_MOONWALK:
                            controller->otto_.Moonwalker(params.steps, params.speed, params.amount,
                                                         params.direction);
                            break;
                        case ACTION_BEND:
                            controller->otto_.Bend(params.steps, params.speed, params.direction);
                            break;
                        case ACTION_SHAKE_LEG:
                            controller->otto_.ShakeLeg(params.steps, params.speed, params.direction);
                            break;
                        case ACTION_SIT:
                            controller->otto_.Sit();
                            break;
                        case ACTION_RADIO_CALISTHENICS:
                            if (controller->has_hands_) {
                                controller->otto_.RadioCalisthenics();
                            }
                            break;
                        case ACTION_MAGIC_CIRCLE:
                            if (controller->has_hands_) {
                                controller->otto_.MagicCircle();
                            }
                            break;
                        case ACTION_SHOWCASE:
                            controller->otto_.Showcase();
                            break;
                        case ACTION_UPDOWN:
                            controller->otto_.UpDown(params.steps, params.speed, params.amount);
                            break;
                        case ACTION_TIPTOE_SWING:
                            controller->otto_.TiptoeSwing(params.steps, params.speed, params.amount);
                            break;
                        case ACTION_JITTER:
                            controller->otto_.Jitter(params.steps, params.speed, params.amount);
                            break;
                        case ACTION_ASCENDING_TURN:
                            controller->otto_.AscendingTurn(params.steps, params.speed, params.amount);
                            break;
                        case ACTION_CRUSAITO:
                            controller->otto_.Crusaito(params.steps, params.speed, params.amount,
                                                       params.direction);
                            break;
                        case ACTION_FLAPPING:
                            controller->otto_.Flapping(params.steps, params.speed, params.amount,
                                                       params.direction);
                            break;
                        case ACTION_WHIRLWIND_LEG:
                            controller->otto_.WhirlwindLeg(params.steps, params.speed, params.amount);
                            break;
                        case ACTION_HANDS_UP:
                            if (controller->has_hands_) {
                                controller->otto_.HandsUp(params.speed, params.direction);
                            }
                            break;
                        case ACTION_HANDS_DOWN:
                            if (controller->has_hands_) {
                                controller->otto_.HandsDown(params.speed, params.direction);
                            }
                            break;
                        case ACTION_HAND_WAVE:
                            if (controller->has_hands_) {
                                controller->otto_.HandWave( params.direction);
                            }
                            break;
                        case ACTION_WINDMILL:
                            if (controller->has_hands_) {
                                controller->otto_.Windmill(params.steps, params.speed, params.amount);
                            }
                            break;
                        case ACTION_TAKEOFF:
                            if (controller->has_hands_) {
                                controller->otto_.Takeoff(params.steps, params.speed, params.amount);
                            }
                            break;
                        case ACTION_FITNESS:
                            if (controller->has_hands_) {
                                controller->otto_.Fitness(params.steps, params.speed, params.amount);
                            }
                            break;
                        case ACTION_GREETING:
                            if (controller->has_hands_) {
                                controller->otto_.Greeting(params.direction, params.steps);
                            }
                            break;
                        case ACTION_SHY:
                            if (controller->has_hands_) {
                                controller->otto_.Shy(params.direction, params.steps);
                            }
                            break;
                        case ACTION_HOME:
                            controller->otto_.Home(true);
                            break;
                        case ACTION_SAMBA:
                            controller->otto_.Samba(params.steps, params.speed, params.amount);
                            break;
                        case ACTION_BALLET:
                            controller->otto_.Ballet(params.steps, params.speed, params.amount);
                            break;
                        case ACTION_HULA:
                            controller->otto_.Hula(params.steps, params.speed, params.amount);
                            break;
                        case ACTION_TWIST:
                            controller->otto_.Twist(params.steps, params.speed, params.amount);
                            break;
                        case ACTION_ROBOT_DANCE:
                            controller->otto_.RobotDance(params.steps, params.speed, params.amount);
                            break;
                    }
                    if(params.action_type != ACTION_SIT){
                        if (params.action_type != ACTION_HOME && params.action_type != ACTION_SERVO_SEQUENCE) {
                            controller->otto_.Home(params.action_type != ACTION_HANDS_UP);
                        }
                    }
                }
                controller->is_action_in_progress_ = false;
                PowerManager::ResumeBatteryUpdate();  // Tiếp tục cập nhật pin khi kết thúc động tác
                vTaskDelay(pdMS_TO_TICKS(20));
            }
        }
    }

    void StartActionTaskIfNeeded() {
        if (action_task_handle_ == nullptr) {
            xTaskCreate(ActionTask, "otto_action", 1024 * 3, this, configMAX_PRIORITIES - 1,
                        &action_task_handle_);
        }
    }

    void QueueAction(int action_type, int steps, int speed, int direction, int amount) {
        // Kiểm tra động tác tay
        if ((action_type >= ACTION_HANDS_UP && action_type <= ACTION_HAND_WAVE) || 
            (action_type == ACTION_WINDMILL) || (action_type == ACTION_TAKEOFF) || 
            (action_type == ACTION_FITNESS) || (action_type == ACTION_GREETING) ||
            (action_type == ACTION_SHY) || (action_type == ACTION_RADIO_CALISTHENICS) ||
            (action_type == ACTION_MAGIC_CIRCLE) ||
            (action_type == ACTION_BALLET) || (action_type == ACTION_HULA) ||
            (action_type == ACTION_TWIST) || (action_type == ACTION_ROBOT_DANCE)) {
            if (!has_hands_) {
                ESP_LOGW(TAG, "Yêu cầu động tác tay nhưng robot không có servo tay");
                return;
            }
        }

        ESP_LOGI(TAG, "Điều khiển động tác: loại=%d, bước=%d, tốc độ=%d, hướng=%d, biên độ=%d", action_type, steps,
                 speed, direction, amount);

        OttoActionParams params = {action_type, steps, speed, direction, amount, ""};
        xQueueSend(action_queue_, &params, portMAX_DELAY);
        StartActionTaskIfNeeded();
    }

    void QueueServoSequence(const char* servo_sequence_json) {
        if (servo_sequence_json == nullptr) {
            ESP_LOGE(TAG, "JSON chuỗi servo rỗng (nullptr)");
            return;
        }
        
        int input_len = strlen(servo_sequence_json);
        const int buffer_size = 512;  // Kích thước mảng servo_sequence_json
        ESP_LOGI(TAG, "Xếp hàng chuỗi servo, độ dài đầu vào=%d, kích thước buffer=%d", input_len, buffer_size);
        
        if (input_len >= buffer_size) {
            ESP_LOGE(TAG, "Chuỗi JSON quá dài! Độ dài đầu vào=%d, tối đa cho phép=%d", input_len, buffer_size - 1);
            return;
        }
        
        if (input_len == 0) {
            ESP_LOGW(TAG, "JSON chuỗi servo là chuỗi rỗng");
            return;
        }
        
        OttoActionParams params = {ACTION_SERVO_SEQUENCE, 0, 0, 0, 0, ""};
        // Sao chép chuỗi JSON vào struct (giới hạn độ dài)
        strncpy(params.servo_sequence_json, servo_sequence_json, sizeof(params.servo_sequence_json) - 1);
        params.servo_sequence_json[sizeof(params.servo_sequence_json) - 1] = '\0';
        
        ESP_LOGD(TAG, "Chuỗi đã thêm vào queue: %s", params.servo_sequence_json);
        
        xQueueSend(action_queue_, &params, portMAX_DELAY);
        StartActionTaskIfNeeded();
    }

    void LoadTrimsFromNVS() {
        Settings settings("otto_trims", false);

        int left_leg = settings.GetInt("left_leg", 0);
        int right_leg = settings.GetInt("right_leg", 0);
        int left_foot = settings.GetInt("left_foot", 0);
        int right_foot = settings.GetInt("right_foot", 0);
        int left_hand = settings.GetInt("left_hand", 0);
        int right_hand = settings.GetInt("right_hand", 0);

        ESP_LOGI(TAG, "Tải cài đặt trim từ NVS: chân trái=%d, chân phải=%d, bàn trái=%d, bàn phải=%d, tay trái=%d, tay phải=%d",
                 left_leg, right_leg, left_foot, right_foot, left_hand, right_hand);

        otto_.SetTrims(left_leg, right_leg, left_foot, right_foot, left_hand, right_hand);
    }

public:
    OttoController(const HardwareConfig& hw_config) {
        otto_.Init(
            hw_config.left_leg_pin, 
            hw_config.right_leg_pin, 
            hw_config.left_foot_pin, 
            hw_config.right_foot_pin, 
            hw_config.left_hand_pin,
            hw_config.right_hand_pin
        );

        has_hands_ = (hw_config.left_hand_pin != GPIO_NUM_NC && hw_config.right_hand_pin != GPIO_NUM_NC);
        ESP_LOGI(TAG, "Khởi tạo robot Otto %s servo tay", has_hands_ ? "có" : "không có");
        ESP_LOGI(TAG, "Cấu hình chân servo: LL=%d, RL=%d, LF=%d, RF=%d, LH=%d, RH=%d",
                 hw_config.left_leg_pin, hw_config.right_leg_pin,
                 hw_config.left_foot_pin, hw_config.right_foot_pin,
                 hw_config.left_hand_pin, hw_config.right_hand_pin);

        LoadTrimsFromNVS();

        action_queue_ = xQueueCreate(10, sizeof(OttoActionParams));

        QueueAction(ACTION_HOME, 1, 1000, 1, 0);  // direction=1 nghĩa là phục hồi cả tay

        RegisterMcpTools();
    }

    void RegisterMcpTools() {
        auto& mcp_server = McpServer::GetInstance();

        ESP_LOGI(TAG, "Bắt đầu đăng ký công cụ MCP...");

        // Công cụ động tác thống nhất (tất cả động tác trừ chuỗi servo)
        mcp_server.AddTool("self.otto.action",
                           "Thực hiện động tác robot. action: tên động tác; cung cấp tham số tương ứng theo loại động tác: direction: hướng, 1=tiến/quay trái, -1=lùi/quay phải; 0=cả hai tay; "
                           "steps: số bước, 1-100; speed: tốc độ, 100-3000, giá trị nhỏ hơn = nhanh hơn; amount: biên độ, 0-170; arm_swing: biên độ vung tay, 0-170; "
                           "Khi người dùng nói 'nhanh' hãy dùng speed=400, 'chậm' dùng speed=1200, 'mạnh' dùng amount=60, 'nhẹ nhàng' dùng amount=15; "
                           "Khi người dùng nói số bước (ví dụ: '3 bước', '5 lần', '2 cái') hãy dùng steps tương ứng; "
                           "--- ĐỘNG TÁC CƠ BẢN --- "
                           "walk(đi bộ, cần steps/speed/direction/arm_swing): "
                           "  Câu lệnh TIẾN: 'đi về phía trước','đi tới','tiến lên','bước về phía trước','đi thẳng','đi nhanh','đi chậm tới' -> direction=1; "
                           "  Câu lệnh LÙI: 'lùi lại','đi về phía sau','lùi về phía sau' -> direction=-1; "
                           "  Khi kèm 'vẫy tay' hoặc 'tay vung mạnh' thì tăng arm_swing=80; "
                           "turn(quay, cần steps/speed/direction/arm_swing): "
                           "  Câu TRÁI: 'quay trái','rẽ trái','xoay sang trái','quay về bên trái' -> direction=1; "
                           "  Câu PHẢI: 'quay phải','rẽ phải','xoay sang phải','quay về bên phải' -> direction=-1; "
                           "  'quay nửa vòng','xoay 180 độ' -> steps=4; 'quay một vòng' -> steps=8; "
                           "jump(nhảy, cần steps/speed): "
                           "  'nhảy lên','nhảy đi','nhảy lên đi','bật nhảy','nhảy một cái','nhảy mạnh' -> steps=1,speed=700; "
                           "  'nhảy nhanh' -> speed=400; 'nhảy chậm','nhảy nhẹ nhàng' -> speed=1200; "
                           "  'nhảy 3 lần' -> steps=3; 'nhảy 5 cái' -> steps=5; "
                           "--- ĐỘNG TÁC ĐẶC BIỆT --- "
                           "swing(lắc lư, cần steps/speed/amount): "
                           "  'lắc lư','rung lắc','lắc người','lay động','lắc qua lắc lại','nhún nhảy' -> mặc định; "
                           "  'lắc nhẹ nhàng' -> amount=10; 'lắc mạnh' -> amount=60; 'lắc 5 lần' -> steps=5; "
                           "moonwalk(bước mặt trăng, cần steps/speed/direction/amount): "
                           "  'moonwalk','bước đi mặt trăng','đi kiểu Michael Jackson','trượt lùi','làm động tác moonwalk' -> mặc định; "
                           "  'moonwalk sang trái','trượt sang bên trái' -> direction=1; "
                           "  'moonwalk sang phải','trượt sang bên phải' -> direction=-1; "
                           "bend(cúi người, cần steps/speed/direction): "
                           "  'cúi người','nghiêng người','uốn cong người','chúc' -> mặc định; "
                           "  'cúi sang trái','nghiêng sang trái' -> direction=1; "
                           "  'cúi sang phải','nghiêng sang phải' -> direction=-1; "
                           "shake_leg(rung chân, cần steps/speed/direction): "
                           "  'rung chân','lắc chân','giẫm chân','đạp chân' -> mặc định; "
                           "  'rung chân trái','lắc chân trái' -> direction=1; "
                           "  'rung chân phải','lắc chân phải' -> direction=-1; "
                           "sit(ngồi xuống, không cần tham số): "
                           "  'ngồi xuống','ngồi đi','ngồi','ngồi nghỉ','ngồi xuống nghỉ','tạm ngồi đi'; "
                           "updown(lên xuống, cần steps/speed/amount): "
                           "  'lên xuống','nhún lên xuống','squat','gập bụng','nhún người','chống đẩy','tập lên xuống'; "
                           "whirlwind_leg(chân lốc xoáy, cần steps/speed/amount): "
                           "  'chân lốc xoáy','đá lốc xoáy','quay chân','vung chân','kick nhanh','đá như lốc xoáy'; "
                           "--- ĐỘNG TÁC TAY (robot này CÓ servo tay, dùng bình thường) --- "
                           "hands_up(giơ tay, cần speed/direction): "
                           "  'giơ tay','đưa tay lên','giơ tay lên','giơ hai tay lên','giơ tay đầu hàng','làm hình chữ Y' -> direction=0; "
                           "  'giơ tay trái','đưa tay trái lên' -> direction=1; "
                           "  'giơ tay phải','đưa tay phải lên' -> direction=-1; "
                           "hands_down(hạ tay, cần speed/direction): "
                           "  'hạ tay xuống','buông tay','đưa tay xuống','hạ tay','buông hai tay' -> direction=0; "
                           "  'hạ tay trái' -> direction=1; 'hạ tay phải' -> direction=-1; "
                           "hand_wave(vẫy tay, cần direction): "
                           "  'vẫy tay','vẫy chào','vẫy tay chào','chào','say hi','chào tạm biệt','bye bye','vẫy hai tay' -> direction=0; "
                           "  'vẫy tay trái' -> direction=1; 'vẫy tay phải' -> direction=-1; "
                           "windmill(cối xay gió, cần steps/speed/amount): "
                           "  'quay tay như cối xay gió','làm động tác cối xay gió','vung tay vòng tròn','quay tay tròn','windmill'; "
                           "  'quay tay nhanh' -> speed=300; 'vung tay mạnh' -> amount=90; "
                           "takeoff(cất cánh, cần steps/speed/amount): "
                           "  'cất cánh','bay lên','làm động tác máy bay','giả vờ bay','chuẩn bị cất cánh','vung tay như cánh chim'; "
                           "fitness(tập thể dục, cần steps/speed/amount): "
                           "  'tập thể dục','tập gym','tập luyện','workout','làm bài tập','tập với tôi','tập thể dục buổi sáng'; "
                           "  LƯU Ý: 'tập thể dục buổi sáng' CÓ THỂ là fitness HOẶC radio_calisthenics, ưu tiên radio_calisthenics nếu không có yêu cầu rõ ràng; "
                           "greeting(chào hỏi, cần direction/steps): "
                           "  'chào hỏi','chào mọi người','làm động tác chào','đánh lạy','cúi chào','chào theo kiểu lịch sự'; "
                           "shy(xấu hổ/ngượng ngùng, cần direction/steps): "
                           "  'ngượng ngùng','xấu hổ','làm bộ ngại','che mặt','làm vẻ ủ ê','ngại ngùng quá'; "
                           "--- ĐỘNG TÁC TỔNG HỢP --- "
                           "showcase(biểu diễn, không cần tham số): "
                           "  'biểu diễn','biểu diễn một màn','trình diễn','múa một điệu','múa đi','nhảy một điệu gì đó'; "
                           "  'làm động tác đẹp','show hàng đi','cho xem tài nghệ','làm gì đó hay ho'; "
                           "radio_calisthenics(thể dục buổi sáng, không cần tham số): "
                           "  'thể dục buổi sáng','tập thể dục radio','làm bài thể dục','tập thể dục tập thể','thể dục rhythmic','tập theo radio'; "
                           "magic_circle(vòng tròn ma thuật, không cần tham số): "
                           "  'vòng tròn ma thuật','vẽ vòng tròn','xoay vòng tròn','làm vòng tròn kỳ diệu','quay vòng ma thuật','chuyển động tròn'; "
                           "--- ĐIỆU NHẢY MỞ RỘNG (5 điệu mới) --- "
                           "samba(điệu Samba, cần steps/speed/amount): "
                           "  'nhảy Samba','điệu Samba','nhún hông Samba','múa Samba','Samba đi','nhảy theo điệu Samba'; "
                           "  speed=700 (mặc định), amount=20; 'nhanh' speed=400, 'chậm' speed=1000; "
                           "ballet(điệu Ballet, cần steps/speed/amount): "
                           "  'nhảy Ballet','điệu Ballet','múa Ballet','ballet đi','uyển chuyển như vũ công Ballet','nhón chân nhảy múa'; "
                           "  speed=1200 (mặc định, chậm và thanh lịch); 'nhanh hơn' speed=800; "
                           "hula(điệu Hula, cần steps/speed/amount): "
                           "  'nhảy Hula','điệu Hula','Hula Hawaii','lắc hông Hawaii','múa Hawaii','điệu Hawaii'; "
                           "  speed=900 (mặc định); amount=25; "
                           "twist(điệu Twist, cần steps/speed/amount): "
                           "  'nhảy Twist','điệu Twist','xoắn người','Twist đi','múa Twist','nhảy điệu Twist'; "
                           "  speed=600 (mặc định, nhanh); 'chậm lại' speed=900; "
                           "robot_dance(điệu Robot, cần steps/speed/amount): "
                           "  'nhảy điệu robot','điệu Robot','nhảy như robot','múa kiểu robot','giật cục như robot','robot dance'; "
                           "  speed=600 (mặc định); steps=4; "
                           "--- LỆNH HỆ THỐNG --- "
                           "home(về vị trí ban đầu, không cần tham số): "
                           "  'về vị trí ban đầu','reset vị trí','đứng thẳng','về home','trở về tư thế nghỉ','về chỗ cũ','đứng yên'",
                           PropertyList({
                               Property("action", kPropertyTypeString, "sit"),
                               Property("steps", kPropertyTypeInteger, 3, 1, 100),
                               Property("speed", kPropertyTypeInteger, 700, 100, 3000),
                               Property("direction", kPropertyTypeInteger, 1, -1, 1),
                               Property("amount", kPropertyTypeInteger, 30, 0, 170),
                               Property("arm_swing", kPropertyTypeInteger, 50, 0, 170)
                           }),
                           [this](const PropertyList& properties) -> ReturnValue {
                               std::string action = properties["action"].value<std::string>();
                               // Tất cả tham số đều có giá trị mặc định, có thể truy cập trực tiếp
                               int steps = properties["steps"].value<int>();
                               int speed = properties["speed"].value<int>();
                               int direction = properties["direction"].value<int>();
                               int amount = properties["amount"].value<int>();
                               int arm_swing = properties["arm_swing"].value<int>();

                               // Động tác di chuyển cơ bản
                               if (action == "walk") {
                                   QueueAction(ACTION_WALK, steps, speed, direction, arm_swing);
                                   return true;
                               } else if (action == "turn") {
                                   QueueAction(ACTION_TURN, steps, speed, direction, arm_swing);
                                   return true;
                               } else if (action == "jump") {
                                   QueueAction(ACTION_JUMP, steps, speed, 0, 0);
                                   return true;
                               } else if (action == "swing") {
                                   QueueAction(ACTION_SWING, steps, speed, 0, amount);
                                   return true;
                               } else if (action == "moonwalk") {
                                   QueueAction(ACTION_MOONWALK, steps, speed, direction, amount);
                                   return true;
                               } else if (action == "bend") {
                                   QueueAction(ACTION_BEND, steps, speed, direction, 0);
                                   return true;
                               } else if (action == "shake_leg") {
                                   QueueAction(ACTION_SHAKE_LEG, steps, speed, direction, 0);
                                   return true;
                               } else if (action == "updown") {
                                   QueueAction(ACTION_UPDOWN, steps, speed, 0, amount);
                                   return true;
                               } else if (action == "whirlwind_leg") {
                                   QueueAction(ACTION_WHIRLWIND_LEG, steps, speed, 0, amount);
                                   return true;
                               }
                               // Động tác cố định
                               else if (action == "sit") {
                                   QueueAction(ACTION_SIT, 1, 0, 0, 0);
                                   return true;
                               } else if (action == "showcase") {
                                   QueueAction(ACTION_SHOWCASE, 1, 0, 0, 0);
                                   return true;
                               } else if (action == "home") {
                                   QueueAction(ACTION_HOME, 1, 1000, 1, 0);
                                   return true;
                               }
                               // Động tác tay
                               else if (action == "hands_up") {
                                   if (!has_hands_) {
                                       return "Lỗi: Động tác này yêu cầu servo tay";
                                   }
                                   QueueAction(ACTION_HANDS_UP, 1, speed, direction, 0);
                                   return true;
                               } else if (action == "hands_down") {
                                   if (!has_hands_) {
                                       return "Lỗi: Động tác này yêu cầu servo tay";
                                   }
                                   QueueAction(ACTION_HANDS_DOWN, 1, speed, direction, 0);
                                   return true;
                               } else if (action == "hand_wave") {
                                   if (!has_hands_) {
                                       return "Lỗi: Động tác này yêu cầu servo tay";
                                   }
                                   QueueAction(ACTION_HAND_WAVE, 1, 0, 0, direction);
                                   return true;
                               } else if (action == "windmill") {
                                   if (!has_hands_) {
                                       return "Lỗi: Động tác này yêu cầu servo tay";
                                   }
                                   QueueAction(ACTION_WINDMILL, steps, speed, 0, amount);
                                   return true;
                               } else if (action == "takeoff") {
                                   if (!has_hands_) {
                                       return "Lỗi: Động tác này yêu cầu servo tay";
                                   }
                                   QueueAction(ACTION_TAKEOFF, steps, speed, 0, amount);
                                   return true;
                               } else if (action == "fitness") {
                                   if (!has_hands_) {
                                       return "Lỗi: Động tác này yêu cầu servo tay";
                                   }
                                   QueueAction(ACTION_FITNESS, steps, speed, 0, amount);
                                   return true;
                               } else if (action == "greeting") {
                                   if (!has_hands_) {
                                       return "Lỗi: Động tác này yêu cầu servo tay";
                                   }
                                   QueueAction(ACTION_GREETING, steps, 0, direction, 0);
                                   return true;
                               } else if (action == "shy") {
                                   if (!has_hands_) {
                                       return "Lỗi: Động tác này yêu cầu servo tay";
                                   }
                                   QueueAction(ACTION_SHY, steps, 0, direction, 0);
                                   return true;
                               } else if (action == "radio_calisthenics") {
                                   if (!has_hands_) {
                                       return "Lỗi: Động tác này yêu cầu servo tay";
                                   }
                                   QueueAction(ACTION_RADIO_CALISTHENICS, 1, 0, 0, 0);
                                   return true;
                               } else if (action == "magic_circle") {
                                   if (!has_hands_) {
                                       return "Lỗi: Động tác này yêu cầu servo tay";
                                   }
                                   QueueAction(ACTION_MAGIC_CIRCLE, 1, 0, 0, 0);
                                   return true;
                               } else if (action == "samba") {
                                   QueueAction(ACTION_SAMBA, steps, speed, 0, amount);
                                   return true;
                               } else if (action == "ballet") {
                                   if (!has_hands_) {
                                       return "Lỗi: Động tác này yêu cầu servo tay";
                                   }
                                   QueueAction(ACTION_BALLET, steps, speed, 0, amount);
                                   return true;
                               } else if (action == "hula") {
                                   if (!has_hands_) {
                                       return "Lỗi: Động tác này yêu cầu servo tay";
                                   }
                                   QueueAction(ACTION_HULA, steps, speed, 0, amount);
                                   return true;
                               } else if (action == "twist") {
                                   if (!has_hands_) {
                                       return "Lỗi: Động tác này yêu cầu servo tay";
                                   }
                                   QueueAction(ACTION_TWIST, steps, speed, 0, amount);
                                   return true;
                               } else if (action == "robot_dance") {
                                   if (!has_hands_) {
                                       return "Lỗi: Động tác này yêu cầu servo tay";
                                   }
                                   QueueAction(ACTION_ROBOT_DANCE, steps, speed, 0, amount);
                                   return true;
                               } else {
                                   return "Lỗi: Tên động tác không hợp lệ. Các động tác hỗ trợ: walk, turn, jump, swing, moonwalk, bend, shake_leg, updown, whirlwind_leg, sit, showcase, home, hands_up, hands_down, hand_wave, windmill, takeoff, fitness, greeting, shy, radio_calisthenics, magic_circle, samba, ballet, hula, twist, robot_dance";
                               }
                           });


        // Công cụ chuỗi servo (hỗ trợ gửi từng phần, mỗi lần một chuỗi, tự động xếp hàng thực thi)
        mcp_server.AddTool(
            "self.otto.servo_sequences",
            "Lập trình động tác tùy chỉnh cho AI (động tác ứng tấu). Hỗ trợ gửi từng phần: nếu hơn 5 chuỗi, AI có thể gọi công cụ này nhiều lần liên tiếp, mỗi lần gửi một chuỗi ngắn, hệ thống tự động xếp hàng thực thi theo thứ tự. Hỗ trợ hai chế độ: di chuyển thông thường và dao động."
            "Cấu trúc robot: hai tay có thể vẫy lên xuống, hai chân có thể dạng/khép, hai bàn chân có thể lật lên xuống."
            "Mô tả servo:"
            "ll(chân trái): dạng/khép, 0 độ=hoàn toàn dạng, 90 độ=trung lập, 180 độ=hoàn toàn khép;"
            "rl(chân phải): dạng/khép, 0 độ=hoàn toàn khép, 90 độ=trung lập, 180 độ=hoàn toàn dạng;"
            "lf(bàn trái): lật lên xuống, 0 độ=hoàn toàn lên, 90 độ=ngang, 180 độ=hoàn toàn xuống;"
            "rf(bàn phải): lật lên xuống, 0 độ=hoàn toàn xuống, 90 độ=ngang, 180 độ=hoàn toàn lên;"
            "lh(tay trái): vẫy lên xuống, 0 độ=hoàn toàn xuống, 90 độ=ngang, 180 độ=hoàn toàn lên;"
            "rh(tay phải): vẫy lên xuống, 0 độ=hoàn toàn lên, 90 độ=ngang, 180 độ=hoàn toàn xuống;"
            "sequence: đối tượng chuỗi đơn, chứa mảng động tác 'a', tham số tùy chọn cấp cao nhất:"
            "'d'(độ trễ tính bằng mili giây sau khi hoàn thành chuỗi, dùng để nghỉ giữa các chuỗi)."
            "Mỗi đối tượng động tác gồm:"
            "Chế độ thông thường: 's' đối tượng vị trí servo (khóa: ll/rl/lf/rf/lh/rh, giá trị: 0-180 độ), 'v' tốc độ di chuyển 100-3000ms (mặc định 1000), 'd' độ trễ sau động tác (mặc định 0);"
            "Chế độ dao động: 'osc' đối tượng dao động, gồm 'a' biên độ (10-90 độ/servo, mặc định 20), 'o' góc tâm tuyệt đối (0-180 độ/servo, mặc định 90), 'ph' độ lệch pha (0-360 độ/servo, mặc định 0), 'p' chu kỳ 100-3000ms (mặc định 500), 'c' số chu kỳ 0.1-20.0 (mặc định 5.0);"
            "Cách dùng: AI có thể gọi công cụ này nhiều lần liên tiếp, mỗi lần một chuỗi, hệ thống tự xếp hàng thực thi theo thứ tự."
            "Lưu ý quan trọng: khi dao động chân/bàn chân trái phải, một chân phải ở 90 độ, nếu không sẽ hỏng robot. Nếu gửi nhiều chuỗi (>1), sau khi hoàn thành tất cả, AI nên gọi riêng self.otto.home để phục hồi vị trí, không đặt tham số phục hồi trong chuỗi."
            "Ví dụ chế độ thông thường - gửi 3 chuỗi, cuối cùng gọi phục hồi:"
            "Lần 1: {\"sequence\":\"{\\\"a\\\":[{\\\"s\\\":{\\\"ll\\\":100},\\\"v\\\":1000}],\\\"d\\\":500}\"},"
            "Lần 2: {\"sequence\":\"{\\\"a\\\":[{\\\"s\\\":{\\\"ll\\\":90},\\\"v\\\":800}],\\\"d\\\":500}\"},"
            "Lần 3: {\"sequence\":\"{\\\"a\\\":[{\\\"s\\\":{\\\"ll\\\":80},\\\"v\\\":800}]}\"},"
            "Cuối cùng gọi self.otto.home để phục hồi."
            "Ví dụ chế độ dao động:"
            "Ví dụ 1 - hai tay đồng bộ: {\"sequence\":\"{\\\"a\\\":[{\\\"osc\\\":{\\\"a\\\":{\\\"lh\\\":30,\\\"rh\\\":30},\\\"o\\\":{\\\"lh\\\":90,\\\"rh\\\":-90},\\\"p\\\":500,\\\"c\\\":5.0}}],\\\"d\\\":0}\"};"
            "Ví dụ 2 - hai chân luân phiên (sóng): {\"sequence\":\"{\\\"a\\\":[{\\\"osc\\\":{\\\"a\\\":{\\\"ll\\\":20,\\\"rl\\\":20},\\\"o\\\":{\\\"ll\\\":90,\\\"rl\\\":-90},\\\"ph\\\":{\\\"rl\\\":180},\\\"p\\\":600,\\\"c\\\":3.0}}],\\\"d\\\":0}\"};"
            "Ví dụ 3 - một chân dao động kết hợp bàn cố định (an toàn): {\"sequence\":\"{\\\"a\\\":[{\\\"osc\\\":{\\\"a\\\":{\\\"ll\\\":45},\\\"o\\\":{\\\"ll\\\":90,\\\"lf\\\":90},\\\"p\\\":400,\\\"c\\\":4.0}}],\\\"d\\\":0}\"};"
            "Ví dụ 4 - dao động nhiều servo (tay và chân): {\"sequence\":\"{\\\"a\\\":[{\\\"osc\\\":{\\\"a\\\":{\\\"lh\\\":25,\\\"rh\\\":25,\\\"ll\\\":15},\\\"o\\\":{\\\"lh\\\":90,\\\"rh\\\":90,\\\"ll\\\":90,\\\"lf\\\":90},\\\"ph\\\":{\\\"rh\\\":180},\\\"p\\\":800,\\\"c\\\":6.0}}],\\\"d\\\":500}\"};"
            "Ví dụ 5 - lắc nhanh: {\"sequence\":\"{\\\"a\\\":[{\\\"osc\\\":{\\\"a\\\":{\\\"ll\\\":30,\\\"rl\\\":30},\\\"o\\\":{\\\"ll\\\":90,\\\"rl\\\":90},\\\"ph\\\":{\\\"rl\\\":180},\\\"p\\\":300,\\\"c\\\":10.0}}],\\\"d\\\":0}\"}.",
            PropertyList({Property("sequence", kPropertyTypeString,
                                   "{\"a\":[{\"s\":{\"ll\":90,\"rl\":90},\"v\":1000}]}")}),
            [this](const PropertyList& properties) -> ReturnValue {
                std::string sequence = properties["sequence"].value<std::string>();
                // Kiểm tra xem có phải JSON object không (có thể là chuỗi hoặc đối tượng đã parse)
                // Nếu sequence là chuỗi JSON, dùng trực tiếp; nếu là chuỗi đối tượng cũng cần dùng
                QueueServoSequence(sequence.c_str());
                return true;
            });


        mcp_server.AddTool("self.otto.stop", "Dừng ngay tất cả động tác và về vị trí gốc", PropertyList(),
                           [this](const PropertyList& properties) -> ReturnValue {
                               if (action_task_handle_ != nullptr) {
                                   vTaskDelete(action_task_handle_);
                                   action_task_handle_ = nullptr;
                               }
                               is_action_in_progress_ = false;
                               PowerManager::ResumeBatteryUpdate();  // Tiếp tục cập nhật pin khi dừng động tác
                               xQueueReset(action_queue_);

                               QueueAction(ACTION_HOME, 1, 1000, 1, 0);
                               return true;
                           });

        mcp_server.AddTool(
            "self.otto.set_trim",
            "Hiệu chỉnh vị trí servo đơn. Đặt tham số tinh chỉnh cho servo được chỉ định để điều chỉnh tư thế đứng ban đầu của robot, cài đặt được lưu vĩnh viễn."
            "servo_type: loại servo (left_leg/right_leg/left_foot/right_foot/left_hand/right_hand); "
            "trim_value: giá trị tinh chỉnh (-50 đến 50 độ)",
            PropertyList({Property("servo_type", kPropertyTypeString, "left_leg"),
                          Property("trim_value", kPropertyTypeInteger, 0, -50, 50)}),
            [this](const PropertyList& properties) -> ReturnValue {
                std::string servo_type = properties["servo_type"].value<std::string>();
                int trim_value = properties["trim_value"].value<int>();

                ESP_LOGI(TAG, "Đặt trim servo: %s = %d độ", servo_type.c_str(), trim_value);

                // Lấy tất cả giá trị trim hiện tại
                Settings settings("otto_trims", true);
                int left_leg = settings.GetInt("left_leg", 0);
                int right_leg = settings.GetInt("right_leg", 0);
                int left_foot = settings.GetInt("left_foot", 0);
                int right_foot = settings.GetInt("right_foot", 0);
                int left_hand = settings.GetInt("left_hand", 0);
                int right_hand = settings.GetInt("right_hand", 0);

                // Cập nhật giá trị trim của servo được chỉ định
                if (servo_type == "left_leg") {
                    left_leg = trim_value;
                    settings.SetInt("left_leg", left_leg);
                } else if (servo_type == "right_leg") {
                    right_leg = trim_value;
                    settings.SetInt("right_leg", right_leg);
                } else if (servo_type == "left_foot") {
                    left_foot = trim_value;
                    settings.SetInt("left_foot", left_foot);
                } else if (servo_type == "right_foot") {
                    right_foot = trim_value;
                    settings.SetInt("right_foot", right_foot);
                } else if (servo_type == "left_hand") {
                    if (!has_hands_) {
                        return "Lỗi: Robot không có servo tay";
                    }
                    left_hand = trim_value;
                    settings.SetInt("left_hand", left_hand);
                } else if (servo_type == "right_hand") {
                    if (!has_hands_) {
                        return "Lỗi: Robot không có servo tay";
                    }
                    right_hand = trim_value;
                    settings.SetInt("right_hand", right_hand);
                } else {
                    return "Lỗi: Loại servo không hợp lệ, hãy dùng: left_leg, right_leg, left_foot, "
                           "right_foot, left_hand, right_hand";
                }

                otto_.SetTrims(left_leg, right_leg, left_foot, right_foot, left_hand, right_hand);

                QueueAction(ACTION_JUMP, 1, 500, 0, 0);

                return "Servo " + servo_type + " đã đặt trim thành " + std::to_string(trim_value) +
                       " độ, đã lưu vĩnh viễn";
            });

        mcp_server.AddTool("self.otto.get_trims", "Lấy cài đặt trim servo hiện tại", PropertyList(),
                           [this](const PropertyList& properties) -> ReturnValue {
                               Settings settings("otto_trims", false);

                               int left_leg = settings.GetInt("left_leg", 0);
                               int right_leg = settings.GetInt("right_leg", 0);
                               int left_foot = settings.GetInt("left_foot", 0);
                               int right_foot = settings.GetInt("right_foot", 0);
                               int left_hand = settings.GetInt("left_hand", 0);
                               int right_hand = settings.GetInt("right_hand", 0);

                               std::string result =
                                   "{\"left_leg\":" + std::to_string(left_leg) +
                                   ",\"right_leg\":" + std::to_string(right_leg) +
                                   ",\"left_foot\":" + std::to_string(left_foot) +
                                   ",\"right_foot\":" + std::to_string(right_foot) +
                                   ",\"left_hand\":" + std::to_string(left_hand) +
                                   ",\"right_hand\":" + std::to_string(right_hand) + "}";

                               ESP_LOGI(TAG, "Lấy cài đặt trim: %s", result.c_str());
                               return result;
                           });

        mcp_server.AddTool("self.otto.get_status", "Lấy trạng thái robot, trả về moving hoặc idle",
                           PropertyList(), [this](const PropertyList& properties) -> ReturnValue {
                               return is_action_in_progress_ ? "moving" : "idle";
                           });

        mcp_server.AddTool("self.battery.get_level", "Lấy mức pin và trạng thái sạc của robot", PropertyList(),
                           [](const PropertyList& properties) -> ReturnValue {
                               auto& board = Board::GetInstance();
                               int level = 0;
                               bool charging = false;
                               bool discharging = false;
                               board.GetBatteryLevel(level, charging, discharging);

                               std::string status =
                                   "{\"level\":" + std::to_string(level) +
                                   ",\"charging\":" + (charging ? "true" : "false") + "}";
                               return status;
                           });
                           
        mcp_server.AddTool("self.otto.get_ip", "Lấy địa chỉ IP WiFi của robot", PropertyList(),
                           [](const PropertyList& properties) -> ReturnValue {
                               auto& wifi = WifiManager::GetInstance();
                               std::string ip = wifi.GetIpAddress();
                               if (ip.empty()) {
                                   return "{\"ip\":\"\",\"connected\":false}";
                               }
                               std::string status = "{\"ip\":\"" + ip + "\",\"connected\":true}";
                               return status;
                           });                           

        ESP_LOGI(TAG, "Đăng ký công cụ MCP hoàn tất");
    }

    ~OttoController() {
        if (action_task_handle_ != nullptr) {
            vTaskDelete(action_task_handle_);
            action_task_handle_ = nullptr;
        }
        vQueueDelete(action_queue_);
    }
};

static OttoController* g_otto_controller = nullptr;

void InitializeOttoController(const HardwareConfig& hw_config) {
    if (g_otto_controller == nullptr) {
        g_otto_controller = new OttoController(hw_config);
        ESP_LOGI(TAG, "Bộ điều khiển Otto đã khởi tạo và đăng ký công cụ MCP");
    }
}
