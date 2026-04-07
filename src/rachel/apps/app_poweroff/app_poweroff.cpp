/**
 * @file app_poweroff.cpp
 * @author Forairaaaaa
 * @brief
 * @version 0.1
 * @date 2023-11-04
 *
 * @copyright Copyright (c) 2023
 *
 */
#include "app_poweroff.h"
#include "spdlog/spdlog.h"
#include "../../hal/hal.h"
#include "../assets/theme/theme.h"

using namespace MOONCAKE::APPS;

// Like setup()...
void AppPoweroff::onCreate() { spdlog::info("{} onCreate", getAppName()); }

void AppPoweroff::onResume() { spdlog::info("{} onResume", getAppName()); }

// Like loop()...
void AppPoweroff::onRunning()
{
    HAL::Delay(100);
    HAL::PowerOff();
}

void AppPoweroff::onDestroy() { spdlog::info("{} onDestroy", getAppName()); }
