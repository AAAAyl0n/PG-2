/**
 * @file app_asciiart.cpp
 * @brief Joystick test - red dot follows joystick input
 */
#include "app_asciiart.h"
#include "spdlog/spdlog.h"
#include "../../hal/hal.h"
#include "../assets/theme/theme.h"

using namespace MOONCAKE::APPS;

// 屏幕尺寸
static const int SCREEN_W = 240;
static const int SCREEN_H = 240;
static const int DOT_RADIUS = 8;
static const int DEADZONE = 150;  // ADC 死区 (中心附近忽略)
static const int NUM_LEDS = 11;

void AppAsciiart::onCreate()
{
    spdlog::info("{} onCreate", getAppName());
}

void AppAsciiart::onResume()
{
    spdlog::info("{} onResume", getAppName());
    HAL::GetCanvas()->setFont(&fonts::Font0);
    HAL::GetCanvas()->setTextSize(1);

    // 启动时读取当前摇杆值作为中位校准
    _data.center_x = HAL::GetJoystickX();
    _data.center_y = HAL::GetJoystickY();
    _data.dot_x = SCREEN_W / 2.0f;
    _data.dot_y = SCREEN_H / 2.0f;
    _data.calibrated = true;

    // LED 初始化：低亮度白色
    HAL::SetLedBrightness(5);   // ~2%
    HAL::SetLedOff();
}

void AppAsciiart::onRunning()
{
    while (true)
    {
        // BACK 键退出
        if (HAL::GetButton(GAMEPAD::BTN_BACK)) {
            HAL::SetLedOff();
            destroyApp();
            return;
        }

        // 读取摇杆原始值
        _data.raw_x = HAL::GetJoystickX();
        _data.raw_y = HAL::GetJoystickY();

        // 相对于中位的偏移
        int dx = _data.raw_x - _data.center_x;
        int dy = _data.raw_y - _data.center_y;

        // 死区处理
        if (abs(dx) < DEADZONE) dx = 0;
        if (abs(dy) < DEADZONE) dy = 0;

        // 映射到屏幕坐标 (ADC 0~4095, 中位 ~2048, 半幅 ~2048)
        // 减去死区后的有效范围
        float range = 2048.0f - DEADZONE;
        float norm_x = (float)dx / range;  // -1.0 ~ 1.0
        float norm_y = (float)dy / range;

        // clamp
        if (norm_x > 1.0f) norm_x = 1.0f;
        if (norm_x < -1.0f) norm_x = -1.0f;
        if (norm_y > 1.0f) norm_y = 1.0f;
        if (norm_y < -1.0f) norm_y = -1.0f;

        // 映射到屏幕 (留出圆点半径的边距)
        _data.dot_x = (SCREEN_W / 2.0f) + norm_x * (SCREEN_W / 2.0f - DOT_RADIUS);
        _data.dot_y = (SCREEN_H / 2.0f) + norm_y * (SCREEN_H / 2.0f - DOT_RADIUS);

        // 绘制
        HAL::GetCanvas()->fillScreen(TFT_BLACK);

        // 十字线
        HAL::GetCanvas()->drawLine(SCREEN_W / 2, 0, SCREEN_W / 2, SCREEN_H, TFT_DARKGRAY);
        HAL::GetCanvas()->drawLine(0, SCREEN_H / 2, SCREEN_W, SCREEN_H / 2, TFT_DARKGRAY);

        // 红色圆点
        HAL::GetCanvas()->fillSmoothCircle((int)_data.dot_x, (int)_data.dot_y, DOT_RADIUS, TFT_RED);

        // 数值显示
        HAL::GetCanvas()->setTextColor(TFT_WHITE, TFT_BLACK);
        HAL::GetCanvas()->setCursor(4, 4);
        HAL::GetCanvas()->printf("X:%4d  Y:%4d", _data.raw_x, _data.raw_y);
        HAL::GetCanvas()->setCursor(4, 14);
        HAL::GetCanvas()->printf("cx:%4d cy:%4d", _data.center_x, _data.center_y);

        // 编码器调试：显示 A/B 实时电平 + 累计计数
        static int enc_count = 0;
        if (HAL::GetButton(GAMEPAD::BTN_RIGHT)) enc_count++;
        if (HAL::GetButton(GAMEPAD::BTN_SELECT)) enc_count--;
        HAL::GetCanvas()->setCursor(4, 24);
        HAL::GetCanvas()->printf("ENC cnt:%d", enc_count);

        // 触摸原始值显示 + LED 跟随触摸
        // ELE5→LED0, ELE4→LED2, ELE3→LED4, ELE2→LED6, ELE1→LED8, ELE0→LED10
        static const int TOUCH_THRESHOLD = 25;
        static const int pad_to_led[6] = {10, 8, 6, 4, 2, 0};  // ELE0~ELE5 对应的 LED 位置
        static const uint8_t BRIGHT_CENTER = 255;
        static const uint8_t BRIGHT_SIDE = 90;

        uint8_t led_brightness[NUM_LEDS] = {0};

        HAL::GetCanvas()->setTextColor(TFT_WHITE, TFT_BLACK);
        for (int i = 0; i < 6; i++) {
            uint16_t filt = HAL::GetTouchFiltered(i);
            uint16_t base = HAL::GetTouchBaseline(i);
            bool touched = (base > TOUCH_THRESHOLD) && (filt < base - TOUCH_THRESHOLD);

            HAL::GetCanvas()->setCursor(4, SCREEN_H - 60 + i * 10);
            HAL::GetCanvas()->printf("E%d f:%4d b:%4d %c", i, filt, base, touched ? '*' : ' ');

            if (touched) {
                int center = pad_to_led[i];
                // 中心灯最亮
                if (led_brightness[center] < BRIGHT_CENTER)
                    led_brightness[center] = BRIGHT_CENTER;
                // 两侧灯稍暗
                if (center - 1 >= 0 && led_brightness[center - 1] < BRIGHT_SIDE)
                    led_brightness[center - 1] = BRIGHT_SIDE;
                if (center + 1 < NUM_LEDS && led_brightness[center + 1] < BRIGHT_SIDE)
                    led_brightness[center + 1] = BRIGHT_SIDE;
            }
        }

        for (int i = 0; i < NUM_LEDS; i++) {
            uint8_t b = led_brightness[i];
            HAL::SetLedColor(i, b, b, b);
        }
        HAL::LedShow();

        HAL::CanvasUpdate();
    }
}

void AppAsciiart::onDestroy()
{
    spdlog::info("{} onDestroy", getAppName());
}
