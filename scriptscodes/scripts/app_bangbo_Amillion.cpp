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
#include "assets/Amillion_face.hpp"

using namespace MOONCAKE::APPS;

// 邦布眼睛颜色
static const auto BANGBOO_EYE_COLOR = THEME_COLOR_WHITE;

// Y轴偏移量（正值向下，负值向上）
static const int Y_OFFSET = -6;

// 表情序列定义：一个循环周期的表情和时间
static const AppBangboo::ExpressionSequence_t expression_sequence[] = {
    {AppBangboo::EXPR_EYES, 800},   // 正常眼睛 0.1秒
    {AppBangboo::EXPR_BLINK, 100},   // 眨眼 0.1秒
    {AppBangboo::EXPR_EYES, 100},   // 正常眼睛 0.1秒
    {AppBangboo::EXPR_BLINK, 100},   // 眨眼 0.1秒
    {AppBangboo::EXPR_EYES, 3000},   // 正常眼睛 3秒
    {AppBangboo::EXPR_BLINK, 100},   // 眨眼 0.1秒
    {AppBangboo::EXPR_ANGER, 3000},  // 愤怒 3秒
    {AppBangboo::EXPR_BLINK, 100},   // 眨眼 0.1秒
    {AppBangboo::EXPR_EYES, 3000},   // 正常眼睛 3秒
    {AppBangboo::EXPR_BLINK, 100},   // 眨眼 0.1秒
    {AppBangboo::EXPR_SMILE, 3000},  // 微笑 3秒
    {AppBangboo::EXPR_BLINK, 100},   // 眨眼 0.1秒
    {AppBangboo::EXPR_EYES, 100},    // 正常眼睛 0.1秒
    {AppBangboo::EXPR_BLINK, 100},   // 眨眼 0.1秒
    {AppBangboo::EXPR_EYES, 3000},   // 正常眼睛 3秒
    //{AppBangboo::EXPR_BLINK, 130}    // 眨眼 0.13秒
};

static const int SEQUENCE_LENGTH = sizeof(expression_sequence) / sizeof(expression_sequence[0]);
static const int MAX_LOOPS = 100;

void AppBangboo::drawExpression(ExpressionType_t expression)
{
    // 先绘制背景图片
    HAL::GetCanvas()->fillScreen(THEME_COLOR_BLACK);
    HAL::GetCanvas()->drawPng(Amillion_face, sizeof(Amillion_face));
    
    switch (expression) {
        case EXPR_EYES:
            HAL::GetCanvas()->fillSmoothCircle(60, 91 + Y_OFFSET, 32, BANGBOO_EYE_COLOR);
            HAL::GetCanvas()->fillSmoothCircle(60, 91 + Y_OFFSET, 21, THEME_COLOR_BLACK);
            // HAL::GetCanvas()->fillSmoothCircle(180, 91 + Y_OFFSET, 32, BANGBOO_EYE_COLOR);
            // HAL::GetCanvas()->fillSmoothCircle(180, 91 + Y_OFFSET, 21, THEME_COLOR_BLACK);
            break;
            
        case EXPR_BLINK:
            HAL::GetCanvas()->fillSmoothRoundRect(29, 86 + Y_OFFSET, 64, 11, 2, BANGBOO_EYE_COLOR);
            // HAL::GetCanvas()->fillSmoothRoundRect(149, 86 + Y_OFFSET, 64, 11, 2, BANGBOO_EYE_COLOR);
            break;
            
        case EXPR_SMILE:
            HAL::GetCanvas()->setColor(BANGBOO_EYE_COLOR);
            HAL::GetCanvas()->fillArc(60, 150 + Y_OFFSET, 61, 71, 241, 299);
            // HAL::GetCanvas()->fillArc(180, 150 + Y_OFFSET, 61, 71, 241, 299);
            break;
            
        case EXPR_ANGER:
            HAL::GetCanvas()->setColor(BANGBOO_EYE_COLOR);
            HAL::GetCanvas()->fillSmoothCircle(60, 91 + Y_OFFSET, 32, BANGBOO_EYE_COLOR);
            HAL::GetCanvas()->fillSmoothCircle(60, 91 + Y_OFFSET, 21, THEME_COLOR_BLACK);
            // HAL::GetCanvas()->fillSmoothCircle(180, 91 + Y_OFFSET, 32, BANGBOO_EYE_COLOR);
            // HAL::GetCanvas()->fillSmoothCircle(180, 91 + Y_OFFSET, 21, THEME_COLOR_BLACK);
            HAL::GetCanvas()->fillSmoothTriangle(0, 30 + Y_OFFSET, 60, 91 + Y_OFFSET, 120, 91 + Y_OFFSET, THEME_COLOR_BLACK);
            HAL::GetCanvas()->fillSmoothTriangle(0, 20 + Y_OFFSET, 60, 81 + Y_OFFSET, 120, 81 + Y_OFFSET, THEME_COLOR_BLACK);
            // HAL::GetCanvas()->fillSmoothTriangle(240, 30 + Y_OFFSET, 180, 91 + Y_OFFSET, 90, 91 + Y_OFFSET, THEME_COLOR_BLACK);
            break;
            
        case EXPR_SAD:
            HAL::GetCanvas()->setColor(BANGBOO_EYE_COLOR);
            HAL::GetCanvas()->fillSmoothCircle(60, 91 + Y_OFFSET, 32, BANGBOO_EYE_COLOR);
            HAL::GetCanvas()->fillSmoothCircle(60, 91 + Y_OFFSET, 21, THEME_COLOR_BLACK);
            // HAL::GetCanvas()->fillSmoothCircle(180, 91 + Y_OFFSET, 32, BANGBOO_EYE_COLOR);
            // HAL::GetCanvas()->fillSmoothCircle(180, 91 + Y_OFFSET, 21, THEME_COLOR_BLACK);
            HAL::GetCanvas()->fillSmoothTriangle(20, 91 + Y_OFFSET, 50, 0 + Y_OFFSET, 100, 70 + Y_OFFSET, THEME_COLOR_BLACK);
            // HAL::GetCanvas()->fillSmoothTriangle(220, 91 + Y_OFFSET, 190, 0 + Y_OFFSET, 140, 70 + Y_OFFSET, THEME_COLOR_BLACK);
            break;
            
        case EXPR_WINCE:
            HAL::GetCanvas()->fillSmoothTriangle(95, 91 + Y_OFFSET, 33, 55 + Y_OFFSET, 33, 127 + Y_OFFSET, BANGBOO_EYE_COLOR);
            HAL::GetCanvas()->fillSmoothTriangle(73, 91 + Y_OFFSET, 33, 67.72 + Y_OFFSET, 33, 114.28 + Y_OFFSET, THEME_COLOR_BLACK);
            // HAL::GetCanvas()->fillSmoothTriangle(145, 91 + Y_OFFSET, 207, 55 + Y_OFFSET, 207, 127 + Y_OFFSET, BANGBOO_EYE_COLOR);
            // HAL::GetCanvas()->fillSmoothTriangle(167, 91 + Y_OFFSET, 207, 67.72 + Y_OFFSET, 207, 114.28 + Y_OFFSET, THEME_COLOR_BLACK);

            HAL::GetCanvas()->fillSmoothTriangle(33, 73 + Y_OFFSET, 33, 55 + Y_OFFSET, 53, 55 + Y_OFFSET, THEME_COLOR_BLACK);
            HAL::GetCanvas()->fillSmoothTriangle(33, 109 + Y_OFFSET, 33, 127 + Y_OFFSET, 53, 127 + Y_OFFSET, THEME_COLOR_BLACK);

            // HAL::GetCanvas()->fillSmoothTriangle(207, 73 + Y_OFFSET, 207, 55 + Y_OFFSET, 187, 55 + Y_OFFSET, THEME_COLOR_BLACK);
            // HAL::GetCanvas()->fillSmoothTriangle(207, 109 + Y_OFFSET, 207, 127 + Y_OFFSET, 187, 127 + Y_OFFSET, THEME_COLOR_BLACK);

            HAL::GetCanvas()->fillTriangle(90, 55 + Y_OFFSET, 90, 127 + Y_OFFSET, 120, 91 + Y_OFFSET, THEME_COLOR_BLACK);
            // HAL::GetCanvas()->fillTriangle(150, 55 + Y_OFFSET, 150, 127 + Y_OFFSET, 120, 91 + Y_OFFSET, THEME_COLOR_BLACK);
            break;
    }
    
    HAL::CanvasUpdate();
}

void AppBangboo::drawBlinkWithColor(uint32_t color)
{
    // 先绘制背景图片
    HAL::GetCanvas()->fillScreen(THEME_COLOR_BLACK);
    HAL::GetCanvas()->drawPng(Amillion_face, sizeof(Amillion_face));
    
    HAL::GetCanvas()->fillSmoothRoundRect(29, 86 + Y_OFFSET, 64, 11, 2, color);
    // HAL::GetCanvas()->fillSmoothRoundRect(149, 86 + Y_OFFSET, 64, 11, 2, color);
    HAL::CanvasUpdate();
}

void AppBangboo::onCreate() { 
    spdlog::info("{} onCreate", getAppName());
    initExpressionSequence();
}

void AppBangboo::onResume() { 
    spdlog::info("{} onResume", getAppName()); 
}

void AppBangboo::initExpressionSequence()
{
    _data.current_time = HAL::Millis();
    _data.sequence_start_time = _data.current_time;
    _data.app_start_time = _data.current_time;  // 记录app启动时间
    _data.current_sequence_index = -1; // -1 表示初始启动动画状态
    _data.current_loop = 0;
    _data.current_expression = EXPR_BLINK;
    _data.gpio_low_set = false;  // 初始化GPIO状态标志
    
    // 显示初始黑色眨眼状态
    drawBlinkWithColor(THEME_COLOR_BLACK);
}

void AppBangboo::updateExpressionSequence()
{
    _data.current_time = HAL::Millis();
    unsigned long elapsed_time = _data.current_time - _data.sequence_start_time;
    
    if (_data.current_sequence_index == -1) {
        if (elapsed_time >= 500) {
            // 开始循环序列
            _data.current_sequence_index = 0;
            _data.sequence_start_time = _data.current_time;
            _data.current_expression = expression_sequence[0].expression;
            drawExpression(_data.current_expression);
        } else {
            // 启动动画：眨眼状态下颜色从黑色渐变到绿色
            float progress = (float)elapsed_time / 500.0f;
            progress = progress > 1.0f ? 1.0f : progress;
            
            // 从黑色(0,0,0)到BANGBOO_EYE_COLOR的RGB插值
            // THEME_COLOR_LawnGreen = 0x7CFC00 = RGB(124, 252, 0)
            uint8_t r = (uint8_t)(124 * progress);
            uint8_t g = (uint8_t)(252 * progress);
            uint8_t b = (uint8_t)(0 * progress);
            uint32_t current_color = (r << 16) | (g << 8) | b;
            
            drawBlinkWithColor(current_color);
        }
        return;
    }
    
    // 检查当前表情是否到时间了
    if (elapsed_time >= expression_sequence[_data.current_sequence_index].duration_ms) {
        // 切换到下一个表情
        _data.current_sequence_index++;
        
        // 检查是否完成了一个循环
        if (_data.current_sequence_index >= SEQUENCE_LENGTH) {
            _data.current_loop++;
            
            // 检查是否完成了所有循环
            if (_data.current_loop >= MAX_LOOPS) {
                spdlog::info("Bangboo animation completed {} loops", MAX_LOOPS);
                destroyApp();
                return;
            }
            
            // 重新开始循环
            _data.current_sequence_index = 0;
        }
        
        // 更新当前表情并绘制
        _data.current_expression = expression_sequence[_data.current_sequence_index].expression;
        drawExpression(_data.current_expression);
        _data.sequence_start_time = _data.current_time;
    }
}

void AppBangboo::onRunning()
{
    _data.current_time = HAL::Millis();
    
    // 初始化GPIO1和GPIO2为输出模式并设为高电平
    if (!_data.gpio_low_set) {
        pinMode(1, OUTPUT);
        pinMode(2, OUTPUT);
        digitalWrite(1, HIGH);
        digitalWrite(2, HIGH);
    }
    
    // 检查是否已经过了6秒 (6000毫秒)
    if (!_data.gpio_low_set && (_data.current_time - _data.app_start_time) >= 6000) {
        digitalWrite(1, LOW);
        digitalWrite(2, LOW);
        _data.gpio_low_set = true;  // 标记已设为低电平
        spdlog::info("GPIO1 and GPIO2 set to LOW after 6 seconds");
    }
    
    // 检查按键SELECT退出
    if (HAL::GetButton(GAMEPAD::BTN_SELECT)) {
        digitalWrite(1, LOW);
        digitalWrite(2, LOW);
        destroyApp();
        return;
    }
    
    // 更新表情序列
    updateExpressionSequence();
}

void AppBangboo::onDestroy() { spdlog::info("{} onDestroy", getAppName()); }
