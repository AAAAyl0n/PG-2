/**
 * @file app_bangboo.cpp
 * @author bowen
 * @brief
 * @version 0.1
 * @date 2025-06-11
 *
 * @copyright Copyright (c) 2025
 *
 */
 #include "app_bangboo.h"
 #include "spdlog/spdlog.h"
 #include "../../hal/hal.h"
 #include "../assets/theme/theme.h"
 #include <cmath>
 #include <cstdlib>

using namespace MOONCAKE::APPS;

static unsigned long _get_auto_sleep_timeout_ms()
{
    switch (HAL::GetSystemConfig().auto_sleep_timeout)
    {
    case 0: return 10UL * 60UL * 1000UL;  // 10min
    case 1: return 30UL * 60UL * 1000UL;  // 30min
    case 2: return 60UL * 60UL * 1000UL;  // 60min
    case 3: return 0;                     // Never
    default: return 10UL * 60UL * 1000UL;
    }
}

void AppBangboo::CheckStatusBar() {
    // 检测START按键
    if (HAL::GetButton(GAMEPAD::BTN_START)) {
        //_show_status_bar();
        showStatusMessage("time", 3000);
    }

    if (HAL::GetButton(GAMEPAD::BTN_RIGHT)) {
        //_show_status_bar();
        showStatusMessage("Hi~", 3000);
        // 随机播放两个音频中的一个
        if (rand() % 2 == 0) {
            HAL::PlayWavFile("/bangboo_audio/bada1.wav");
        } else {
            HAL::PlayWavFile("/bangboo_audio/bada2.wav");
        }
        changeState(STATE_PREHAPPY);   
    }

    if (_data.statusBarVisible && !_data.statusBarAnimating) {
        if (HAL::Millis() - _data.statusBarShowTime > _data.statusBarDisplayDuration) {
            _hide_status_bar();
        }
    }
}

//------------------------------------------APP核心区------------------------------------------


void AppBangboo::onCreate() {
    spdlog::info("{} onCreate", getAppName());
    
    // 初始化状态栏动画
    _data.statusBarAnim.setAnim(LVGL::ease_out, 0, 24, 600);
    _data.statusBarVisible = false;
    _data.statusBarAnimating = false;
    
    // 初始化状态机
    _data.currentState = STATE_PREIDLE;
    _data.stateStartTime = HAL::Millis();
    _data.stateTimer = 0;
}

void AppBangboo::onResume() {
    spdlog::info("{} onResume", getAppName());
    HAL::GetCanvas()->setFont(&fonts::Font0);
    _data.lastActivityTime = HAL::Millis();
}

void AppBangboo::onRunning()
{
    // 初始化随机种子
    srand(HAL::Millis());

    while (true)
    {
        unsigned long currentTime = HAL::Millis();

        if (HAL::GetAnyButton()) {
            _data.lastActivityTime = currentTime;
        }

        // BOOT 键退出
        if (HAL::GetButton(GAMEPAD::BTN_BACK)) {
            destroyApp();
            return;
        }

        // 长时间无操作自动进入轻睡眠（按 START 唤醒）。
        unsigned long sleep_timeout_ms = _get_auto_sleep_timeout_ms();
        if (sleep_timeout_ms > 0 &&
            currentTime - _data.lastActivityTime >= sleep_timeout_ms) {
            HAL::GetCanvas()->fillScreen(TFT_BLACK);
            HAL::CanvasUpdate();
            HAL::LightSleepWithButtonWakeup();
            _data.lastActivityTime = HAL::Millis();
            continue;
        }
        // 更新IMU检测状态
        imu_detection(); 

        // 更新状态机 - 自动管理表情变化
        updateStateMachine();

        // 更新时钟
        _update_clock();

        // 检查状态栏
        CheckStatusBar();

        // 渲染画面
        render();
    }
}


void AppBangboo::onDestroy() { spdlog::info("{} onDestroy", getAppName()); }
 