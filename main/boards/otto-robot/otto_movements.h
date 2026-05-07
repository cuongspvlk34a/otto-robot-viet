#ifndef __OTTO_MOVEMENTS_H__
#define __OTTO_MOVEMENTS_H__

#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "oscillator.h"

//-- Hằng số
#define FORWARD 1
#define BACKWARD -1
#define LEFT 1
#define RIGHT -1
#define BOTH 0
#define SMALL 5
#define MEDIUM 15
#define BIG 30

// -- Giới hạn delta servo mặc định. độ / giây
#define SERVO_LIMIT_DEFAULT 240

// -- Chỉ số servo để truy cập nhanh
#define LEFT_LEG 0
#define RIGHT_LEG 1
#define LEFT_FOOT 2
#define RIGHT_FOOT 3
#define LEFT_HAND 4
#define RIGHT_HAND 5
#define SERVO_COUNT 6

class Otto {
public:
    Otto();
    ~Otto();

    //-- Khởi tạo Otto
    void Init(int left_leg, int right_leg, int left_foot, int right_foot, int left_hand = -1,
              int right_hand = -1);
    //-- Hàm gắn và tháo servo
    void AttachServos();
    void DetachServos();

    //-- Hiệu chỉnh bộ dao động
    void SetTrims(int left_leg, int right_leg, int left_foot, int right_foot, int left_hand = 0,
                  int right_hand = 0);

    //-- Các hàm chuyển động định sẵn
    void MoveServos(int time, int servo_target[]);
    void MoveSingle(int position, int servo_number);
    void OscillateServos(int amplitude[SERVO_COUNT], int offset[SERVO_COUNT], int period,
                         double phase_diff[SERVO_COUNT], float cycle);
    void Execute2(int amplitude[SERVO_COUNT], int center_angle[SERVO_COUNT], int period,
                  double phase_diff[SERVO_COUNT], float steps);

    //-- HOME = Otto ở tư thế nghỉ
    void Home(bool hands_down = true);
    bool GetRestState();
    void SetRestState(bool state);

    //-- Các hàm chuyển động định sẵn
    void Jump(float steps = 1, int period = 2000);

    void Walk(float steps = 4, int period = 1000, int dir = FORWARD, int amount = 0);
    void Turn(float steps = 4, int period = 2000, int dir = LEFT, int amount = 0);
    void Bend(int steps = 1, int period = 1400, int dir = LEFT);
    void ShakeLeg(int steps = 1, int period = 2000, int dir = RIGHT);
    void Sit();  // Ngồi xuống

    void UpDown(float steps = 1, int period = 1000, int height = 20);
    void Swing(float steps = 1, int period = 1000, int height = 20);
    void TiptoeSwing(float steps = 1, int period = 900, int height = 20);
    void Jitter(float steps = 1, int period = 500, int height = 20);
    void AscendingTurn(float steps = 1, int period = 900, int height = 20);

    void Moonwalker(float steps = 1, int period = 900, int height = 20, int dir = LEFT);
    void Crusaito(float steps = 1, int period = 900, int height = 20, int dir = FORWARD);
    void Flapping(float steps = 1, int period = 1000, int height = 20, int dir = FORWARD);
    void WhirlwindLeg(float steps = 1, int period = 300, int amplitude = 30);

    // -- Hành động tay
    void HandsUp(int period = 1000, int dir = 0);      // Hai tay giơ lên
    void HandsDown(int period = 1000, int dir = 0);    // Hai tay hạ xuống
    void HandWave(int dir = LEFT);  // Vẫy tay
    void Windmill(float steps = 10, int period = 500, int amplitude = 90);  // Cối xay gió lớn
    void Takeoff(float steps = 5, int period = 300, int amplitude = 40);   // Cất cánh
    void Fitness(float steps = 5, int period = 1000, int amplitude = 25);  // Tập thể dục
    void Greeting(int dir = LEFT, float steps = 5);  // Chào hỏi
    void Shy(int dir = LEFT, float steps = 5);  // Xấu hổ
    void RadioCalisthenics();  // Thể dục buổi sáng
    void MagicCircle();  // Xoay vòng phép màu tình yêu
    void Showcase();  // Trình diễn động tác (kết hợp nhiều động tác)

    // -- Giới hạn tốc độ servo
    void EnableServoLimit(int speed_limit_degree_per_sec = SERVO_LIMIT_DEFAULT);
    void DisableServoLimit();

private:
    Oscillator servo_[SERVO_COUNT];

    int servo_pins_[SERVO_COUNT];
    int servo_trim_[SERVO_COUNT];

    unsigned long final_time_;
    unsigned long partial_time_;
    float increment_[SERVO_COUNT];

    bool is_otto_resting_;
    bool has_hands_;  // Có servo tay hay không

    void Execute(int amplitude[SERVO_COUNT], int offset[SERVO_COUNT], int period,
                 double phase_diff[SERVO_COUNT], float steps);

};

#endif  // __OTTO_MOVEMENTS_H__