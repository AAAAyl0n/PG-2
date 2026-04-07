/**
 * @file app_timeview.cpp
 * @author Forairaaaaa
 * @brief
 * @version 0.1
 * @date 2023-11-04
 *
 * @copyright Copyright (c) 2023
 *
 */
#include "app_timeview.h"
#include "spdlog/spdlog.h"
#include "../../hal/hal.h"
#include "../assets/theme/theme.h"
#include <string>
#include <ctime>
#include <cstdio>
#include <cstring>

using namespace MOONCAKE::APPS;

void AppTimeview::onCreate() { 
    spdlog::info("{} onCreate", getAppName()); 
    // 初始化弹窗动画配置
    _data.popupSlideAnim.setAnim(LVGL::ease_out, 0, 24, 300);
    _data.popupWidthAnim.setAnim(LVGL::ease_out, 0, 0, 250);
    _data.popupVisible = false;
    _data.popupAnimating = false;
    _data.popupText = "CLOCK";

    // 倒计时默认 5 分钟
    _data.timerTotalMs = 5 * 60 * 1000UL;
    _data.timerRemainMs = _data.timerTotalMs;
}

// Like setup()...
void AppTimeview::onResume() { 
    spdlog::info("{} onResume", getAppName()); 
    //HAL::LoadTextFont24();//为了防止阻塞
    HAL::GetCanvas()->setFont(&fonts::Font0);
}

// Like loop()...
void AppTimeview::onRunning()
{
    while(1){
        // 退出
        if (HAL::GetButton(GAMEPAD::BTN_SELECT)) {
            destroyApp();
            return;
        }

        // 输入与业务
        _handleInputs();
        _updateStopwatch();
        _updateTimer();

        // 渲染
        _renderMain();
        _renderPopup();
        HAL::CanvasUpdate();
        
        HAL::Delay(10);
    }
    
}

void AppTimeview::onDestroy() { spdlog::info("{} onDestroy", getAppName()); }

void AppTimeview::_renderMain()
{
    HAL::GetCanvas()->fillScreen(THEME_COLOR_BLACK);
    switch (_data.currentMode) {
        case Data_t::MODE_CLOCK: _renderClock(); break;
        case Data_t::MODE_STOPWATCH: _renderStopwatch(); break;
        case Data_t::MODE_TIMER: _renderTimer(); break;
    }
}
