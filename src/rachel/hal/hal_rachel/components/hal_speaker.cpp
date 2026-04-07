/**
 * @file hal_speaker.cpp
 * @author Forairaaaaa
 * @brief
 * @version 0.1
 * @date 2023-11-08
 *
 * @copyright Copyright (c) 2023
 *
 */
#include <mooncake.h>
#include "../hal_rachel.h"
#include <Arduino.h>
#include "../hal_config.h"

void HAL_Rachel::_spk_init()
{
    spdlog::info("buzz init");
    HAL_LOG_INFO("buzz init");
}

void HAL_Rachel::beep(float frequency, uint32_t duration)
{
    if (_config.volume == 0)
        return;

    // _spk->tone(frequency, duration);
    tone(HAL_PIN_BUZZ, frequency, duration);
}

void HAL_Rachel::beepStop() { noTone(HAL_PIN_BUZZ); }

void HAL_Rachel::setBeepVolume(uint8_t volume)
{
    // _spk->setVolume(volume);
}
