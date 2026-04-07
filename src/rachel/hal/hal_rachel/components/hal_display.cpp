/**
 * @file hal_gamepad.cpp
 * @author Forairaaaaa
 * @brief Ref: https://github.com/lovyan03/LovyanGFX/blob/master/examples/HowToUse/2_user_setting/2_user_setting.ino
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
// 启动图：使用设置页的内置 PNG 资源（page_init_emptyos）
#include "../../../apps/app_settings/assets/page_init_emptyos.hpp"

class LGFX_Rachel : public lgfx::LGFX_Device
{
    lgfx::Panel_ST7789 _panel_instance;
    lgfx::Bus_SPI _bus_instance;
    lgfx::Light_PWM _light_instance;

public:
    LGFX_Rachel(void)
    {
        {
            auto cfg = _bus_instance.config();

            cfg.pin_mosi = HAL_PIN_LCD_MOSI;
            cfg.pin_miso = HAL_PIN_SD_MISO;
            cfg.pin_sclk = HAL_PIN_LCD_SCLK;
            cfg.pin_dc = HAL_PIN_LCD_DC;
            cfg.freq_write = 80000000;
            cfg.spi_3wire = false;

            _bus_instance.config(cfg);
            _panel_instance.setBus(&_bus_instance);
        }
        {
            auto cfg = _panel_instance.config();

            cfg.invert = true;
            cfg.pin_cs = HAL_PIN_LCD_CS;
            cfg.pin_rst = HAL_PIN_LCD_RST;
            cfg.pin_busy = HAL_PIN_LCD_BUSY;
            cfg.panel_width = 240;
            cfg.panel_height = 240;
            cfg.offset_x = 0;
            cfg.offset_y = 0;
            cfg.bus_shared = true;

            _panel_instance.config(cfg);
        }
        {
            auto cfg = _light_instance.config();

            cfg.pin_bl = HAL_PIN_LCD_BL;
            cfg.invert = false;
            cfg.freq = 44100;
            cfg.pwm_channel = 7;

            _light_instance.config(cfg);
            _panel_instance.setLight(&_light_instance);
        }

        setPanel(&_panel_instance);
    }
};

void HAL_Rachel::_disp_init()
{
    spdlog::info("display init");

    _display = new LGFX_Rachel;
    _display->init();

    _canvas = new LGFX_SpriteFx(_display);
    _canvas->createSprite(_display->width(), _display->height());

    HAL_LOGGER_INIT();
    // 可选：将日志改为单行刷新模式，固定在顶部一行
    setLogSingleLineMode(true, 0, 230);
    _disp_logo();
    HAL_LOG_INFO("bangboo init");
}

static const std::string _logo = R"()";

void HAL_Rachel::_disp_logo()
{
    // 清屏并绘制启动图片
    _canvas->fillScreen(TFT_BLACK);
    _canvas->drawPng(page_init_emptyos, page_init_emptyos_size, 0, 50);
    _canvas->pushSprite(0, 0);
}

void HAL_Rachel::_sum_up()
{
    // Pause if encoder turned left (SELECT)
    if (getButton(GAMEPAD::BTN_SELECT))
    {
        HAL_LOG_WARN("encoder SELECT pressed and pause");
        HAL_LOG_WARN("press btn start to proceed");
        while (1)
        {
            delay(100);
            if (getButton(GAMEPAD::BTN_START))
                break;
        }
    }

    HAL_LOG_INFO("hal init success!");
    HAL_LOG_INFO("starting mooncake");
    HAL_LOG_INFO("bye");
}
