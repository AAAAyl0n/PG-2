/**
 * @file app_timeview_timer.cpp
 */
#include "app_timeview.h"
#include "../assets/theme/theme.h"
#include "../../hal/hal.h"
#include <cstdio>

using namespace MOONCAKE::APPS;

// 优化的时间缓存更新函数
void AppTimeview::_updateTimeCache(Data_t::TimeCache_t& cache, uint32_t ms)
{
    // 只有毫秒值改变且超过阈值时才重新计算
    uint32_t msThreshold = (&cache == &_data.stopwatchCache) ? 10 : 1000; // stopwatch需要分秒精度
    
    if (cache.lastMs == 0xFFFFFFFF || 
        (ms > cache.lastMs && ms - cache.lastMs >= msThreshold) ||
        (ms < cache.lastMs)) // 处理溢出或重置情况
    {
        cache.lastMs = ms;
        uint32_t total_sec = ms / 1000UL;
        
        // 一次性计算所有时间单位，避免重复除法
        cache.hh = total_sec / 3600UL;
        uint32_t remainder = total_sec % 3600UL;
        cache.mm = remainder / 60UL;
        cache.ss = remainder % 60UL;
        cache.centisec = (ms % 1000UL) / 10UL;
        cache.needsUpdate = true;
    }
}

const char* AppTimeview::_getFormattedTime(Data_t::TimeCache_t& cache, uint32_t ms, bool showCentisec)
{
    _updateTimeCache(cache, ms);
    
    if (cache.needsUpdate) {
        if (showCentisec) {
            snprintf(cache.timeStr, sizeof(cache.timeStr), "%02u:%02u:%02u.%02u", 
                    (unsigned)cache.hh, (unsigned)cache.mm, (unsigned)cache.ss, (unsigned)cache.centisec);
        } else {
            snprintf(cache.timeStr, sizeof(cache.timeStr), "%02u:%02u:%02u", 
                    (unsigned)cache.hh, (unsigned)cache.mm, (unsigned)cache.ss);
        }
        cache.needsUpdate = false;
    }
    
    return cache.timeStr;
}

void AppTimeview::_renderTimer()
{
    // 使用缓存避免重复计算
    _updateTimeCache(_data.timerCache, _data.timerRemainMs);
    
    char first_line[16];
    char second_line[16];
    
    // 第一排：TMR + 小时数
    snprintf(first_line, sizeof(first_line), "TMR %02u", (unsigned)_data.timerCache.hh);
    // 第二排：分钟:秒
    snprintf(second_line, sizeof(second_line), "%02u:%02u", (unsigned)_data.timerCache.mm, (unsigned)_data.timerCache.ss);

    HAL::GetCanvas()->setTextColor(THEME_COLOR_LawnGreen);
    HAL::GetCanvas()->setTextSize(2);
    HAL::GetCanvas()->drawCenterString(first_line, 120, 42);
    HAL::GetCanvas()->setTextSize(4);
    HAL::GetCanvas()->drawCenterString(second_line, 120, 68);
}

void AppTimeview::_updateTimer()
{
    if (_data.currentMode != Data_t::MODE_TIMER) return;
    if (_data.timerRunning) {
        uint32_t now = HAL::Millis();
        if (_data.timerLastTickMs == 0) _data.timerLastTickMs = now;
        uint32_t delta = now - _data.timerLastTickMs;
        _data.timerLastTickMs = now;
        if (delta > _data.timerRemainMs) delta = _data.timerRemainMs;
        _data.timerRemainMs -= delta;

        if (_data.timerRemainMs == 0) {
            _data.timerRunning = false;
            // 随机播放这两个音频文件中的一个
            int idx = rand() % 2;
            const char* audio_files[] = {
                "/bangboo_audio/ennaaa.wav",
                "/bangboo_audio/ennaenna.wav"
            };
            HAL::PlayWavFile(audio_files[idx]);
            if (idx == 0) {
                HAL::Delay(2500);
            } else {
                HAL::Delay(1500);
            }
        }
    } else {
        _data.timerLastTickMs = 0;
    }
}

void AppTimeview::_enterTimerSettings()
{
    int hh = (int)(_data.timerTotalMs / 3600000UL);
    if (hh > 10) hh = 0;
    int mm = (int)((_data.timerTotalMs % 3600000UL) / 60000UL);
    int ss = (int)((_data.timerTotalMs % 60000UL) / 1000UL);

    // 不等待松开，立即进入设置
    _showPopup("SET HOUR", true);  // 设置界面使用Y轴动画

    // 要求先松开 RIGHT 后再响应（避免带入的长按 RIGHT 立刻生效）
    bool readyHour = false;
    bool selectPrevHour = false, rightPrevHour = false;
    uint32_t selectPressHour = 0, rightPressHour = 0;
    uint32_t selectLastRepHour = 0, rightLastRepHour = 0;
    while (1) {
        if (!readyHour) {
            if (!HAL::GetButton(GAMEPAD::BTN_RIGHT)) {
                readyHour = true;
            } else {
                HAL::GetCanvas()->fillScreen(THEME_COLOR_BLACK);
                char buf[20];
                snprintf(buf, sizeof(buf), "%d", hh);
                HAL::GetCanvas()->setTextColor(THEME_COLOR_LawnGreen);
                HAL::GetCanvas()->setTextSize(4);
                HAL::GetCanvas()->drawCenterString(buf, 120, 62);
                HAL::GetCanvas()->setTextSize(2);
                HAL::GetCanvas()->drawCenterString("-     +", 120, 72);
                _renderPopup();
                HAL::CanvasUpdate();
                HAL::Delay(10);
                continue;
            }
        }

        HAL::GetCanvas()->fillScreen(THEME_COLOR_BLACK);
        char buf[20];
        snprintf(buf, sizeof(buf), "%d", hh);
        HAL::GetCanvas()->setTextColor(THEME_COLOR_LawnGreen);
        HAL::GetCanvas()->setTextSize(4);
        HAL::GetCanvas()->drawCenterString(buf, 120, 62);
        HAL::GetCanvas()->setTextSize(2);
        HAL::GetCanvas()->drawCenterString("-     +", 120, 72);
        _renderPopup();
        HAL::CanvasUpdate();

        uint32_t now = HAL::Millis();
        bool selNow = HAL::GetButton(GAMEPAD::BTN_SELECT);
        bool rightNow = HAL::GetButton(GAMEPAD::BTN_RIGHT);

        // SELECT: 边沿触发一次，长按500ms后每20ms重复
        if (selNow && !selectPrevHour) {
            hh = (hh == 0) ? 10 : (hh - 1);
            selectPressHour = now;
            selectLastRepHour = now;
        } else if (selNow && (now - selectPressHour >= 500) && (now - selectLastRepHour >= 20)) {
            hh = (hh == 0) ? 10 : (hh - 1);
            selectLastRepHour = now;
        }

        // RIGHT: 边沿触发一次，长按500ms后每20ms重复
        if (rightNow && !rightPrevHour) {
            hh = (hh + 1) % 11;
            rightPressHour = now;
            rightLastRepHour = now;
        } else if (rightNow && (now - rightPressHour >= 500) && (now - rightLastRepHour >= 20)) {
            hh = (hh + 1) % 11;
            rightLastRepHour = now;
        }

        // START: 确认并进入下一步（等待松开）
        if (HAL::GetButton(GAMEPAD::BTN_START)) {
            while (HAL::GetButton(GAMEPAD::BTN_START)) HAL::Delay(10);
            break;
        }

        selectPrevHour = selNow;
        rightPrevHour = rightNow;
        HAL::Delay(10);
    }

    _showPopup("SET MINUTE", true);
    bool readyMinute = false;
    bool selectPrevMinute = false, rightPrevMinute = false;
    uint32_t selectPressMinute = 0, rightPressMinute = 0;
    uint32_t selectLastRepMinute = 0, rightLastRepMinute = 0;
    while (1) {
        if (!readyMinute) {
            if (!HAL::GetButton(GAMEPAD::BTN_RIGHT)) {
                readyMinute = true;
            } else {
                HAL::GetCanvas()->fillScreen(THEME_COLOR_BLACK);
                char buf[20];
                snprintf(buf, sizeof(buf), "%d", mm);
                HAL::GetCanvas()->setTextColor(THEME_COLOR_LawnGreen);
                HAL::GetCanvas()->setTextSize(4);
                HAL::GetCanvas()->drawCenterString(buf, 120, 62);
                HAL::GetCanvas()->setTextSize(2);
                HAL::GetCanvas()->drawCenterString("-     +", 120, 72);
                _renderPopup();
                HAL::CanvasUpdate();
                HAL::Delay(10);
                continue;
            }
        }

        HAL::GetCanvas()->fillScreen(THEME_COLOR_BLACK);
        char buf[20];
        snprintf(buf, sizeof(buf), "%d", mm);
        HAL::GetCanvas()->setTextColor(THEME_COLOR_LawnGreen);
        HAL::GetCanvas()->setTextSize(4);
        HAL::GetCanvas()->drawCenterString(buf, 120, 62);
        HAL::GetCanvas()->setTextSize(2);
        HAL::GetCanvas()->drawCenterString("-     +", 120, 72);
        _renderPopup();
        HAL::CanvasUpdate();

        uint32_t now = HAL::Millis();
        bool selNow = HAL::GetButton(GAMEPAD::BTN_SELECT);
        bool rightNow = HAL::GetButton(GAMEPAD::BTN_RIGHT);

        if (selNow && !selectPrevMinute) {
            mm = (mm == 0) ? 59 : (mm - 1);
            selectPressMinute = now;
            selectLastRepMinute = now;
        } else if (selNow && (now - selectPressMinute >= 500) && (now - selectLastRepMinute >= 20)) {
            mm = (mm == 0) ? 59 : (mm - 1);
            selectLastRepMinute = now;
        }

        if (rightNow && !rightPrevMinute) {
            mm = (mm + 1) % 60;
            rightPressMinute = now;
            rightLastRepMinute = now;
        } else if (rightNow && (now - rightPressMinute >= 500) && (now - rightLastRepMinute >= 20)) {
            mm = (mm + 1) % 60;
            rightLastRepMinute = now;
        }

        if (HAL::GetButton(GAMEPAD::BTN_START)) {
            while (HAL::GetButton(GAMEPAD::BTN_START)) HAL::Delay(10);
            break;
        }

        selectPrevMinute = selNow;
        rightPrevMinute = rightNow;
        HAL::Delay(10);
    }

    _showPopup("SET SECOND", true);
    bool readySecond = false;
    bool selectPrevSecond = false, rightPrevSecond = false;
    uint32_t selectPressSecond = 0, rightPressSecond = 0;
    uint32_t selectLastRepSecond = 0, rightLastRepSecond = 0;
    while (1) {
        if (!readySecond) {
            if (!HAL::GetButton(GAMEPAD::BTN_RIGHT)) {
                readySecond = true;
            } else {
                HAL::GetCanvas()->fillScreen(THEME_COLOR_BLACK);
                char buf[20];
                snprintf(buf, sizeof(buf), "%d", ss);
                HAL::GetCanvas()->setTextColor(THEME_COLOR_LawnGreen);
                HAL::GetCanvas()->setTextSize(4);
                HAL::GetCanvas()->drawCenterString(buf, 120, 62);
                HAL::GetCanvas()->setTextSize(2);
                HAL::GetCanvas()->drawCenterString("-     +", 120, 72);
                _renderPopup();
                HAL::CanvasUpdate();
                HAL::Delay(10);
                continue;
            }
        }

        HAL::GetCanvas()->fillScreen(THEME_COLOR_BLACK);
        char buf[20];
        snprintf(buf, sizeof(buf), "%d", ss);
        HAL::GetCanvas()->setTextColor(THEME_COLOR_LawnGreen);
        HAL::GetCanvas()->setTextSize(4);
        HAL::GetCanvas()->drawCenterString(buf, 120, 62);
        HAL::GetCanvas()->setTextSize(2);
        HAL::GetCanvas()->drawCenterString("-     +", 120, 72);
        _renderPopup();
        HAL::CanvasUpdate();

        uint32_t now = HAL::Millis();
        bool selNow = HAL::GetButton(GAMEPAD::BTN_SELECT);
        bool rightNow = HAL::GetButton(GAMEPAD::BTN_RIGHT);

        if (selNow && !selectPrevSecond) {
            ss = (ss == 0) ? 59 : (ss - 1);
            selectPressSecond = now;
            selectLastRepSecond = now;
        } else if (selNow && (now - selectPressSecond >= 500) && (now - selectLastRepSecond >= 20)) {
            ss = (ss == 0) ? 59 : (ss - 1);
            selectLastRepSecond = now;
        }

        if (rightNow && !rightPrevSecond) {
            ss = (ss + 1) % 60;
            rightPressSecond = now;
            rightLastRepSecond = now;
        } else if (rightNow && (now - rightPressSecond >= 500) && (now - rightLastRepSecond >= 20)) {
            ss = (ss + 1) % 60;
            rightLastRepSecond = now;
        }

        if (HAL::GetButton(GAMEPAD::BTN_START)) {
            while (HAL::GetButton(GAMEPAD::BTN_START)) HAL::Delay(10);
            break;
        }

        selectPrevSecond = selNow;
        rightPrevSecond = rightNow;
        HAL::Delay(10);
    }

    // 退出前等待 RIGHT 松开，避免长按 RIGHT 立即再次触发进入设置
    while (HAL::GetButton(GAMEPAD::BTN_RIGHT)) HAL::Delay(10);
    _hidePopup();
    _data.timerTotalMs = (uint32_t)hh * 3600000UL + (uint32_t)mm * 60000UL + (uint32_t)ss * 1000UL;
    _data.timerRemainMs = _data.timerTotalMs;
}


