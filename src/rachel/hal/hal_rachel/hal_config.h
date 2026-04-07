/**
 * @file hal_common_config.h
 * @author Forairaaaaa
 * @brief
 * @version 0.1
 * @date 2023-11-07
 *
 * @copyright Copyright (c) 2023
 *
 */
#pragma once

/**
 * @brief Pin configs
 *
 */
// LCD
#define HAL_PIN_LCD_MOSI 40
#define HAL_PIN_LCD_MISO -1
#define HAL_PIN_LCD_SCLK 41
#define HAL_PIN_LCD_DC 42
#define HAL_PIN_LCD_CS 44
#define HAL_PIN_LCD_RST 12
#define HAL_PIN_LCD_BUSY -1
#define HAL_PIN_LCD_BL 11

// Power
#define HAL_PIN_PWR_HOLD 10

// Button
#define HAL_PIN_GAMEPAD_START 8     // START 键 / 编码器按键 (GPIO8)
#define HAL_PIN_BTN_BACK 0          // BOOT 键用作返回/退出 (GPIO0)

// Joystick (模拟摇杆)
#define HAL_PIN_JOYSTICK_X 4        // 摇杆 AD-X (GPIO4)
#define HAL_PIN_JOYSTICK_Y 5        // 摇杆 AD-Y (GPIO5)
#define HAL_PIN_JOYSTICK_BTN 7      // 摇杆按键 (GPIO7)

// Rotary Encoder (旋转编码器/滚轮)
#define HAL_PIN_ENC_A 16            // 编码器 A 相 (GPIO16)
#define HAL_PIN_ENC_B 17            // 编码器 B 相 (GPIO17)

// WS2812 LED
#define HAL_PIN_WS2812 21           // WS2812 数据线 (GPIO21)

// MIDI
#define HAL_PIN_MIDI_TX 9           // MIDI UART TX (GPIO9)

// SD card
#define HAL_PIN_SD_CS 15
#define HAL_PIN_SD_MOSI 40
#define HAL_PIN_SD_MISO 39
#define HAL_PIN_SD_CLK 41

// I2C
#define HAL_PIN_I2C_SCL 13
#define HAL_PIN_I2C_SDA 14

// Buzzer
#define HAL_PIN_BUZZ 46

// HAL internal display logger
#define HAL_LOGGER_INIT()                                                                                                      \
    _canvas->setFont(&fonts::Font0);                                                                                           \
    _canvas->setTextSize(1);                                                                                                   \
    _canvas->setTextScroll(true);                                                                                              \
    _canvas->setCursor(0, 0)

#define HAL_LOG(fmt, args...)                                                                                                  \
    if (_log_single_line_mode)                                                                                                 \
    {                                                                                                                          \
        _canvas->setTextColor(TFT_LIGHTGRAY, TFT_BLACK);                                                                       \
        _canvas->printf(fmt, ##args);                                                                                          \
        _canvas->pushSprite(0, 0);                                                                                             \
    }                                                                                                                          \
    else                                                                                                                       \
    {                                                                                                                          \
        _canvas->setTextColor(TFT_LIGHTGRAY, TFT_BLACK);                                                                       \
        _canvas->printf(fmt, ##args);                                                                                          \
        _canvas->print('\n');                                                                                                  \
        _canvas->pushSprite(0, 0);                                                                                             \
    }

#define HAL_LOG_TAG_START()                                                                                                    \
    _canvas->setTextColor(TFT_LIGHTGRAY, TFT_BLACK);                                                                           \
    if (_log_single_line_mode)                                                                                                 \
    {                                                                                                                          \
        int16_t __h = _canvas->fontHeight();                                                                                   \
        _canvas->fillRect(0, _log_single_line_y, _canvas->width(), __h, TFT_BLACK);                                            \
        _canvas->setCursor(_log_single_line_x, _log_single_line_y);                                                            \
    }                                                                                                                          \
    _canvas->print(" [")

#define HAL_LOG_TAG_END()                                                                                                      \
    _canvas->setTextColor(TFT_LIGHTGRAY, TFT_BLACK);                                                                           \
    _canvas->print("] ")

#define HAL_LOG_INFO(fmt, args...)                                                                                             \
    HAL_LOG_TAG_START();                                                                                                       \
    _canvas->setTextColor(TFT_GREENYELLOW, TFT_BLACK);                                                                         \
    _canvas->print("info");                                                                                                    \
    HAL_LOG_TAG_END();                                                                                                         \
    HAL_LOG(fmt, ##args)

#define HAL_LOG_WARN(fmt, args...)                                                                                             \
    HAL_LOG_TAG_START();                                                                                                       \
    _canvas->setTextColor(TFT_YELLOW, TFT_BLACK);                                                                              \
    _canvas->print("warn");                                                                                                    \
    HAL_LOG_TAG_END();                                                                                                         \
    HAL_LOG(fmt, ##args)

#define HAL_LOG_ERROR(fmt, args...)                                                                                            \
    HAL_LOG_TAG_START();                                                                                                       \
    _canvas->setTextColor(TFT_RED, TFT_BLACK);                                                                                 \
    _canvas->print("error");                                                                                                   \
    HAL_LOG_TAG_END();                                                                                                         \
    HAL_LOG(fmt, ##args)
