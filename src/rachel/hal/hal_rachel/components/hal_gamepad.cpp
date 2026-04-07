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

// 中断服务：A 变化时立即读 B 判方向（ISR 内读 GPIO 最及时）
static HAL_Rachel* _hal_instance = nullptr;

void IRAM_ATTR HAL_Rachel::_encoderISR()
{
    if (!_hal_instance) return;

    uint8_t a = digitalRead(HAL_PIN_ENC_A);
    // 只在 B==HIGH 时处理（B 处于稳定高电平）
    if (digitalRead(HAL_PIN_ENC_B) == HIGH && a != _hal_instance->_encoder_a_last)
    {
        if (a == LOW)
            _hal_instance->_encoder_left_flag = true;
        else
            _hal_instance->_encoder_right_flag = true;
    }
    _hal_instance->_encoder_a_last = a;
}

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

    // Encoder init (中断驱动，A+B 都变才计数)
    gpio_reset_pin((gpio_num_t)HAL_PIN_ENC_A);
    gpio_reset_pin((gpio_num_t)HAL_PIN_ENC_B);
    pinMode(HAL_PIN_ENC_A, INPUT_PULLUP);
    pinMode(HAL_PIN_ENC_B, INPUT_PULLUP);
    _encoder_a_last = digitalRead(HAL_PIN_ENC_A);
    _hal_instance = this;
    attachInterrupt(digitalPinToInterrupt(HAL_PIN_ENC_A), _encoderISR, CHANGE);

    _key_state_list.fill(false);
}

// ISR 已完成方向判断，pollEncoder 只是为了兼容调用接口
void HAL_Rachel::_pollEncoder()
{
    // 方向已在 ISR 中判定，无需额外处理
}

bool HAL_Rachel::getButton(GAMEPAD::GamePadButton_t button)
{
    // 每次查询按键时顺带处理编码器
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

    // 物理按键（START / JOYSTICK / BACK）
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
