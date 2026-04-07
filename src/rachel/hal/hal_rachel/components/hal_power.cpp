/**
 * @file hal_power.cpp
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
#include <esp_sleep.h>
#include <driver/i2s.h>
#include "../hal_config.h"

namespace {
constexpr uint8_t BMI270_I2C_ADDR = 0x68;
constexpr uint8_t BMI270_PWR_CONF_ADDR = 0x7C;
constexpr uint8_t BMI270_PWR_CTRL_ADDR = 0x7D;
} // namespace

void HAL_Rachel::_power_init()
{
    // Hold power
    gpio_reset_pin((gpio_num_t)HAL_PIN_PWR_HOLD);
    pinMode(HAL_PIN_PWR_HOLD, OUTPUT);
    digitalWrite(HAL_PIN_PWR_HOLD, 1);
}

void HAL_Rachel::powerOff()
{
    digitalWrite(HAL_PIN_PWR_HOLD, 0);
    delay(1000);
}

void HAL_Rachel::reboot()
{
    beepStop();
    esp_restart();
}

void HAL_Rachel::lightSleepWithButtonWakeup()
{
    const bool was_audio_muted = _audio_muted;

    // 进入休眠前停止外设声音，避免唤醒后残留状态。
    stopAudioPlayback();
    beepStop();
    setAudioMute(true);
    i2s_zero_dma_buffer(I2S_NUM_0);
    i2s_stop(I2S_NUM_0);

    // BMI270 进入低功耗：关闭ACC/GYRO并启用advance power save。
    if (_i2c_bus != nullptr) {
        _i2c_bus->writeRegister8(BMI270_I2C_ADDR, BMI270_PWR_CTRL_ADDR, 0x00, 400000);
        _i2c_bus->writeRegister8(BMI270_I2C_ADDR, BMI270_PWR_CONF_ADDR, 0x03, 400000);
    }

    // START 按键为低电平触发（INPUT_PULLUP + 按下接地）。
    pinMode(HAL_PIN_GAMEPAD_START, INPUT_PULLUP);
    gpio_wakeup_enable((gpio_num_t)HAL_PIN_GAMEPAD_START, GPIO_INTR_LOW_LEVEL);
    esp_sleep_enable_gpio_wakeup();

    // 关闭显示输出并关闭背光，减少休眠电流。
    _display->startWrite();
    _display->writeCommand(0x28); // Display OFF
    _display->writeCommand(0x10); // Sleep IN
    _display->endWrite();
    delay(10);
    _display->setBrightness(0);
    digitalWrite(HAL_PIN_LCD_BL, 0);

    esp_light_sleep_start();

    // 醒来后恢复显示。
    _display->startWrite();
    _display->writeCommand(0x11); // Sleep OUT
    _display->endWrite();
    delay(120);
    _display->startWrite();
    _display->writeCommand(0x29); // Display ON
    _display->endWrite();
    _display->setBrightness(_config.brightness);
    digitalWrite(HAL_PIN_LCD_BL, 1);

    // 恢复BMI270正常工作。
    if (_i2c_bus != nullptr) {
        _i2c_bus->writeRegister8(BMI270_I2C_ADDR, BMI270_PWR_CONF_ADDR, 0x00, 400000);
        _i2c_bus->writeRegister8(BMI270_I2C_ADDR, BMI270_PWR_CTRL_ADDR, 0x0F, 400000);
    }

    // 恢复音频链路默认状态。
    i2s_start(I2S_NUM_0);
    setAudioMute(was_audio_muted);

    // 清理唤醒源配置。
    gpio_wakeup_disable((gpio_num_t)HAL_PIN_GAMEPAD_START);

    // 简单防抖，避免一次按下触发多次动作。
    delay(120);
}
