/**
 * @file app_asciiart.cpp
 * @author Forairaaaaa
 * @brief 
 * @version 0.1
 * @date 2023-11-04
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#include "app_asciiart.h"
#include "spdlog/spdlog.h"
#include "../../hal/hal.h"
#include "../assets/theme/theme.h"
#include <cmath>
#include <string>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

using namespace MOONCAKE::APPS;

void AppAsciiart::onCreate()
{
    spdlog::info("{} onCreate", getAppName());
}

// Like setup()...
void AppAsciiart::onResume()
{
    spdlog::info("{} onResume", getAppName());
    
    // 设置字体和颜色 - 使用 HAL_LOG_INFO 相同的字体
    HAL::GetCanvas()->setFont(&fonts::Font0);
    HAL::GetCanvas()->setTextSize(1);
    HAL::GetCanvas()->setTextColor(TFT_WHITE, TFT_BLACK);
    
    // 初始化数据
    _data.pattern_type = 0;
    _data.last_update_time = HAL::Millis();
    _data.last_button_time = 0;
    _data.last_clock_update = 0;
    _data.start_time = HAL::Millis(); // 记录启动时间
    
    // 初始化时间显示
    auto time_now = HAL::GetLocalTime();
    strftime(_data.date_buffer, sizeof(_data.date_buffer), "%a %d", time_now);
    strftime(_data.time_buffer, sizeof(_data.time_buffer), "%H:%M", time_now);
}

// chaos 模式的数学函数
float AppAsciiart::chaosPattern(float x, float y, float t)
{
    float noise1 = sin(x * 0.5f + t) * cos(y * 0.3f - t);
    float noise2 = sin(y * 0.4f + t * 0.5f) * cos(x * 0.2f + t * 0.7f);
    float noise3 = sin((x + y) * 0.2f + t * 0.8f);
    
    return noise1 * 0.3f + noise2 * 0.3f + noise3 * 0.4f;
}

// waves 模式的数学函数
float AppAsciiart::wavesPattern(float x, float y, float t)
{
    float wave1 = sin(x * 0.3f + t * 0.8f) * cos(y * 0.1f + t * 0.3f);
    float wave2 = sin(x * 0.2f + y * 0.15f + t * 0.6f) * 0.7f;
    float wave3 = sin(x * 0.1f + t * 1.2f) * sin(y * 0.05f + t * 0.4f) * 0.5f;
    
    return wave1 + wave2 + wave3;
}

// 根据数值获取对应的 ASCII 字符
char AppAsciiart::getAsciiChar(float value)
{
    if (value > 0.6f) {
        return '@';
    } else if (value > 0.4f) {
        return '0';
    } else if (value > 0.2f) {
        return '/';
    } else if (value > 0.0f) {
        return '=';
    } else if (value > -0.2f) {
        return '-';
    } else if (value > -0.4f) {
        return '.';
    } else {
        return ' ';
    }
}

// 直接绘制 ASCII 艺术到屏幕
void AppAsciiart::drawAsciiArt()
{
    // 使用毫秒时间，避免帧数无限递增
    float t = (HAL::Millis() * 0.001f) * 0.9f; // 控制动画速度
    
    // 计算填充进度 (3秒内从中心向外扩散)
    unsigned long elapsed_time = HAL::Millis() - _data.start_time;
    const unsigned long fill_duration = 3000; // 3秒填充时间
    float fill_progress = (float)elapsed_time / (float)fill_duration;
    if (fill_progress > 1.0f) fill_progress = 1.0f; // 限制在1.0
    
    // 计算中心点
    float center_x = _data.width / 2.0f;
    float center_y = _data.height / 2.0f;
    float max_distance = sqrt(center_x * center_x + center_y * center_y);
    
    int start_y = 0;
    int line_height = 7;
    int char_width = 6;
    
    for (int y = 0; y < _data.height; y++) {
        for (int x = 0; x < _data.width; x++) {
            // 计算到中心的距离
            float dx = x - center_x;
            float dy = y - center_y;
            float distance = sqrt(dx * dx + dy * dy);
            float normalized_distance = distance / max_distance;
            
            // 添加一些噪声让边缘更自然
            float noise = sin((x + y) * 0.5f + t * 2.0f) * 0.1f;
            float reveal_threshold = fill_progress + noise;
            
            // 只有当填充波前到达这个位置时才显示字符
            if (normalized_distance <= reveal_threshold) {
                float value;
                
                if (_data.pattern_type == 0) {
                    value = chaosPattern((float)x, (float)y, t);
                } else {
                    value = wavesPattern((float)x, (float)y, t);
                }
                
                char ascii_char = getAsciiChar(value);
                if (ascii_char != ' ') { // 只绘制非空字符，提升效率
                    int screen_x = (240 - _data.width * char_width) / 2 + x * char_width;
                    int screen_y = start_y + y * line_height;
                    HAL::GetCanvas()->setCursor(screen_x, screen_y);
                    HAL::GetCanvas()->print(ascii_char);
                }
            }
        }
    }
}

// Like loop()...
void AppAsciiart::onRunning()
{
    unsigned long current_time = HAL::Millis();
    
    // 检查按键输入 (START键切换模式)
    if (HAL::GetButton(GAMEPAD::BTN_START) && 
        (current_time - _data.last_button_time > _data.button_debounce)) {
        _data.pattern_type = (_data.pattern_type + 1) % 2;
        _data.last_button_time = current_time;
        _data.start_time = current_time; // 重新开始填充效果
        spdlog::info("切换到模式: {}", _data.pattern_type == 0 ? "chaos" : "waves");
    }
    
    // 检查按键SELECT退出
    if (HAL::GetButton(GAMEPAD::BTN_SELECT)) {
        destroyApp();
        return;
    }

    // 更新动画帧
    if (current_time - _data.last_update_time >= _data.frame_interval) {
        // 清屏 - 黑色背景
        HAL::GetCanvas()->fillScreen(TFT_BLACK);
        HAL::GetCanvas()->setTextColor(TFT_WHITE, TFT_BLACK);
        
        // 直接绘制 ASCII 艺术到上半屏幕
        drawAsciiArt();
        
        // 在下半部分显示按键提示信息
        int bottom_area_y = 110; // 调整位置为时间显示留空间
        const char* mode_name = (_data.pattern_type == 0) ? "chaos" : "waves";
        
        HAL::GetCanvas()->setCursor(10, bottom_area_y + 25);
        HAL::GetCanvas()->printf("Mode: %s", mode_name);
        
        HAL::GetCanvas()->setCursor(10, bottom_area_y + 30);
        HAL::GetCanvas()->print("START: Switch  SELECT: Exit");
        
        // 更新时间缓存 (只在需要时更新)
        if (current_time - _data.last_clock_update >= _data.clock_update_interval) {
            auto time_now = HAL::GetLocalTime();
            strftime(_data.date_buffer, sizeof(_data.date_buffer), "%a %d", time_now);
            strftime(_data.time_buffer, sizeof(_data.time_buffer), "%H:%M", time_now);
            _data.last_clock_update = current_time;
        }
        
        // 每次绘制都显示时间 (使用缓存的时间字符串)
        HAL::GetCanvas()->setTextColor(TFT_WHITE);
        HAL::GetCanvas()->setTextSize(2);
        HAL::GetCanvas()->drawCenterString(_data.date_buffer, 120, 160);
        HAL::GetCanvas()->setTextSize(4);
        HAL::GetCanvas()->drawCenterString(_data.time_buffer, 120, 186);
        HAL::GetCanvas()->setTextSize(1);
        
        // 更新屏幕
        HAL::CanvasUpdate();
        
        _data.last_update_time = current_time;
    }
    
    // 短暂延时
    //HAL::Delay(5);
}

void AppAsciiart::onDestroy()
{
    spdlog::info("{} onDestroy", getAppName());
}
