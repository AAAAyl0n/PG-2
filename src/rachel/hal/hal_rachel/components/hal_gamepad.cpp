/**
 * @file hal_gamepad.cpp
 * @author Forairaaaaa
 * @brief
 * @version 0.1
 * @date 2023-11-07
 *
 * @copyright Copyright (c) 2023
 *
 */
#include <mooncake.h>
#include "../hal_rachel.h"
#include <Arduino.h>
#include "../hal_config.h"

void HAL_Rachel::_gamepad_init()
{
    spdlog::info("gamepad init");
    HAL_LOG_INFO("gamepad init");

    // Button pin mapping
    _gamepad_key_map[GAMEPAD::BTN_START] = HAL_PIN_GAMEPAD_START;
    _gamepad_key_map[GAMEPAD::BTN_JOYSTICK] = HAL_PIN_JOYSTICK_BTN;
    _gamepad_key_map[GAMEPAD::BTN_BACK] = HAL_PIN_BTN_BACK;

    // GPIO init - 按键
    gpio_reset_pin((gpio_num_t)HAL_PIN_GAMEPAD_START);
    pinMode(HAL_PIN_GAMEPAD_START, INPUT_PULLUP);
    gpio_reset_pin((gpio_num_t)HAL_PIN_JOYSTICK_BTN);
    pinMode(HAL_PIN_JOYSTICK_BTN, INPUT_PULLUP);
    gpio_reset_pin((gpio_num_t)HAL_PIN_BTN_BACK);
    pinMode(HAL_PIN_BTN_BACK, INPUT_PULLUP);

    // Joystick ADC init
    pinMode(HAL_PIN_JOYSTICK_X, INPUT);
    pinMode(HAL_PIN_JOYSTICK_Y, INPUT);

    // Encoder init
    gpio_reset_pin((gpio_num_t)HAL_PIN_ENC_A);
    gpio_reset_pin((gpio_num_t)HAL_PIN_ENC_B);
    pinMode(HAL_PIN_ENC_A, INPUT_PULLUP);
    pinMode(HAL_PIN_ENC_B, INPUT_PULLUP);
    _encoder_last_a = digitalRead(HAL_PIN_ENC_A);
    _encoder_delta = 0;

    _key_state_list.fill(false);
}

// 轮询编码器：仅在 A 下降沿检测，用 B 相判方向，带时间消抖
#define ENC_DEBOUNCE_MS 30  // 两次触发最小间隔(ms)

void HAL_Rachel::_pollEncoder()
{
    int a = digitalRead(HAL_PIN_ENC_A);
    if (a != _encoder_last_a)
    {
        unsigned long now = millis();
        if (a == LOW && (now - _encoder_last_trigger_ms >= ENC_DEBOUNCE_MS))
        {
            int b = digitalRead(HAL_PIN_ENC_B);
            if (b == HIGH)
                _encoder_left_flag = true;
            else
                _encoder_right_flag = true;
            _encoder_last_trigger_ms = now;
        }
        _encoder_last_a = a;
    }
}

bool HAL_Rachel::getButton(GAMEPAD::GamePadButton_t button)
{
    // 每次查询按键时顺带轮询编码器
    _pollEncoder();

    // 编码器虚拟按键：读后即清
    if (button == GAMEPAD::BTN_RIGHT)
    {
        bool val = _encoder_right_flag;
        _encoder_right_flag = false;
        return val;
    }
    if (button == GAMEPAD::BTN_SELECT)
    {
        bool val = _encoder_left_flag;
        _encoder_left_flag = false;
        return val;
    }

    // 物理按键（START / JOYSTICK）
    if (!digitalRead(_gamepad_key_map[button]))
    {
        if (!_key_state_list[button])
            _key_state_list[button] = true;
        return true;
    }

    if (_key_state_list[button])
        _key_state_list[button] = false;

    return false;
}

int HAL_Rachel::getJoystickX()
{
    return 4095 - analogRead(HAL_PIN_JOYSTICK_X);
}

int HAL_Rachel::getJoystickY()
{
    return 4095 - analogRead(HAL_PIN_JOYSTICK_Y);
}

int HAL_Rachel::getEncoderDelta()
{
    _pollEncoder();
    if (_encoder_right_flag)
    {
        _encoder_right_flag = false;
        return 1;
    }
    if (_encoder_left_flag)
    {
        _encoder_left_flag = false;
        return -1;
    }
    return 0;
}
