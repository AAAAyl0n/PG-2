/**
 * @file app_timeview_clock.cpp
 */
#include "app_timeview.h"
#include "../../hal/hal.h"
#include "../assets/theme/theme.h"
#include <ctime>

using namespace MOONCAKE::APPS;

void AppTimeview::_renderClock()
{
    auto t = HAL::GetLocalTime();
    char date_buffer[32];
    char time_buffer[16];

    if (_data.clockFormat == Data_t::CLOCK_FMT_MON_DAY) {
        strftime(date_buffer, sizeof(date_buffer), "%b %d", t);
    } else {
        strftime(date_buffer, sizeof(date_buffer), "%a %d", t);
    }
    strftime(time_buffer, sizeof(time_buffer), "%H:%M", t);

    HAL::GetCanvas()->setTextColor(THEME_COLOR_LawnGreen);
    HAL::GetCanvas()->setTextSize(2);
    HAL::GetCanvas()->drawCenterString(date_buffer, 120, 42);
    HAL::GetCanvas()->setTextSize(4);
    HAL::GetCanvas()->drawCenterString(time_buffer, 120, 68);
}


