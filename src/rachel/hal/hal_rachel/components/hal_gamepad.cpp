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

    // Map pin to button - 只保留三个按键
    _gamepad_key_map[GAMEPAD::BTN_START] = HAL_PIN_GAMEPAD_START;
    _gamepad_key_map[GAMEPAD::BTN_SELECT] = HAL_PIN_GAMEPAD_SELECT;
    _gamepad_key_map[GAMEPAD::BTN_RIGHT] = HAL_PIN_GAMEPAD_RIGHT;
    
    // 注释掉不再使用的按键
    // _gamepad_key_map[GAMEPAD::BTN_UP] = HAL_PIN_GAMEPAD_UP;
    // _gamepad_key_map[GAMEPAD::BTN_LEFT] = HAL_PIN_GAMEPAD_LEFT;
    // _gamepad_key_map[GAMEPAD::BTN_DOWN] = HAL_PIN_GAMEPAD_DOWN;
    // _gamepad_key_map[GAMEPAD::BTN_X] = HAL_PIN_GAMEPAD_X;
    // _gamepad_key_map[GAMEPAD::BTN_Y] = HAL_PIN_GAMEPAD_Y;
    // _gamepad_key_map[GAMEPAD::BTN_A] = HAL_PIN_GAMEPAD_A;
    // _gamepad_key_map[GAMEPAD::BTN_B] = HAL_PIN_GAMEPAD_B;
    // _gamepad_key_map[GAMEPAD::BTN_LEFT_STICK] = HAL_PIN_GAMEPAD_LS;

    // GPIO init - 只初始化保留的三个按键
    gpio_reset_pin((gpio_num_t)HAL_PIN_GAMEPAD_START);
    pinMode(HAL_PIN_GAMEPAD_START, INPUT_PULLUP);
    gpio_reset_pin((gpio_num_t)HAL_PIN_GAMEPAD_SELECT);
    pinMode(HAL_PIN_GAMEPAD_SELECT, INPUT_PULLUP);
    gpio_reset_pin((gpio_num_t)HAL_PIN_GAMEPAD_RIGHT);
    pinMode(HAL_PIN_GAMEPAD_RIGHT, INPUT_PULLUP);

    _key_state_list.fill(false);
}

bool HAL_Rachel::getButton(GAMEPAD::GamePadButton_t button)
{
    // return !(bool)(digitalRead(_gamepad_key_map[button]));

    if (!digitalRead(_gamepad_key_map[button]))
    {
        // If just pressed
        if (!_key_state_list[button])
        {
            _key_state_list[button] = true;
            //beep(600, 20);
            //HAL::PlayWavFile("/system_audio/Klick.wav");
        }

        return true;
    }

    // If just released
    if (_key_state_list[button])
    {
        _key_state_list[button] = false;
        //beep(800, 20);
    }

    return false;
}
