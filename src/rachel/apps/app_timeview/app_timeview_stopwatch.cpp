/**
 * @file app_timeview_stopwatch.cpp
 */
#include "app_timeview.h"
#include "../assets/theme/theme.h"
#include "../../hal/hal.h"
#include <cstdio>

using namespace MOONCAKE::APPS;

void AppTimeview::_renderStopwatch()
{
    uint32_t elapsed = _data.swAccumulatedMs + (_data.swRunning ? (HAL::Millis() - _data.swStartMs) : 0);
    
    // 使用缓存避免重复计算
    _updateTimeCache(_data.stopwatchCache, elapsed);
    
    char first_line[16];
    char second_line[16];
    char centisec_str[8];
    
    // 第一排：STW + 小时数
    snprintf(first_line, sizeof(first_line), "STW %02u", (unsigned)_data.stopwatchCache.hh);
    // 第二排：分钟:秒
    snprintf(second_line, sizeof(second_line), "%02u:%02u", (unsigned)_data.stopwatchCache.mm, (unsigned)_data.stopwatchCache.ss);
    // 分秒显示
    snprintf(centisec_str, sizeof(centisec_str), ".%02u", (unsigned)_data.stopwatchCache.centisec);

    HAL::GetCanvas()->setTextColor(THEME_COLOR_LawnGreen);
    HAL::GetCanvas()->setTextSize(2);
    HAL::GetCanvas()->drawCenterString(first_line, 120, 42);
    
    // 第二排：分钟:秒（大字体）
    HAL::GetCanvas()->setTextSize(4);
    HAL::GetCanvas()->drawCenterString(second_line, 120, 68);
    
    // 分秒计时（小字体，显示在秒数右侧）
    HAL::GetCanvas()->setTextSize(1);
    // 计算秒数右侧位置：120是中心，second_line大约占用的宽度的一半 + 分秒文本的偏移
    HAL::GetCanvas()->drawString(centisec_str, 120 + 57, 68 + 21, &fonts::Font0);
}

void AppTimeview::_updateStopwatch()
{
    // 逻辑已在渲染阶段按需计算
}


