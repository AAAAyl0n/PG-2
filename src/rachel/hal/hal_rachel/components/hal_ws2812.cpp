/**
 * @file hal_ws2812.cpp
 * @brief WS2812 LED driver (Adafruit NeoPixel)
 */
#include <mooncake.h>
#include "../hal_rachel.h"
#include "../hal_config.h"
#include <Adafruit_NeoPixel.h>

#define WS2812_NUM_LEDS 11
#define WS2812_DEFAULT_BRIGHTNESS 20  // 亮度不要太高 (0~255)

void HAL_Rachel::_ws2812_init()
{
    spdlog::info("ws2812 init");
    HAL_LOG_INFO("ws2812 init");

    _leds = new Adafruit_NeoPixel(WS2812_NUM_LEDS, HAL_PIN_WS2812, NEO_GRB + NEO_KHZ800);
    _leds->begin();
    _leds->setBrightness(WS2812_DEFAULT_BRIGHTNESS);
    _leds->clear();
    _leds->show();
}

void HAL_Rachel::setLedColor(uint8_t index, uint8_t r, uint8_t g, uint8_t b)
{
    if (_leds == nullptr || index >= WS2812_NUM_LEDS) return;
    _leds->setPixelColor(index, _leds->Color(r, g, b));
}

void HAL_Rachel::setAllLedColor(uint8_t r, uint8_t g, uint8_t b)
{
    if (_leds == nullptr) return;
    uint32_t color = _leds->Color(r, g, b);
    for (int i = 0; i < WS2812_NUM_LEDS; i++)
        _leds->setPixelColor(i, color);
}

void HAL_Rachel::setLedOff()
{
    if (_leds == nullptr) return;
    _leds->clear();
    _leds->show();
}

void HAL_Rachel::ledShow()
{
    if (_leds == nullptr) return;
    _leds->show();
}

void HAL_Rachel::setLedBrightness(uint8_t brightness)
{
    if (_leds == nullptr) return;
    _leds->setBrightness(brightness);
}
