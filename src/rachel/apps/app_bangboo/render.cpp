/**
 * @file app_bangboo_expressions.cpp
 * @brief Expression drawing and sequencing for Bangboo app
 */

#include "app_bangboo.h"
#include "spdlog/spdlog.h"
#include "../../hal/hal.h"
#include "../assets/theme/theme.h"
#include "assets/Amillion_face.hpp"

using namespace MOONCAKE::APPS;

// 运行时根据型号决定眼睛颜色
static inline uint32_t _get_eye_color_by_model(uint8_t model)
{
    // 0=Eous, 1=Amillion, 2=paperboo
    if (model == 1)
        return THEME_COLOR_WHITE;
    if (model == 2)
        return THEME_COLOR_YELLOW;
    return THEME_COLOR_LawnGreen;
}

// Y轴偏移量（正值向下，负值向上）
static const int Y_OFFSET = 0;


void AppBangboo::drawExpression(ExpressionType_t expression)
{
    //HAL::GetCanvas()->fillScreen(THEME_COLOR_BLACK);
    uint8_t model = HAL::GetSystemConfig().model;
    bool isAmillion = (model == 1);
    bool isPaperboo = (model == 2);
    uint32_t eyeColor = _get_eye_color_by_model(model);

    // Amillion 叠加PNG底图
    if (isAmillion)
    {
        HAL::GetCanvas()->drawPng(Amillion_face, sizeof(Amillion_face));
    }

    switch (expression) {
        case EXPR_EYES:
            if (isAmillion)
            {
                // 仅“角色右眼”（屏幕左侧）
                HAL::GetCanvas()->fillSmoothCircle(60, 91 + Y_OFFSET, 32, eyeColor);
                HAL::GetCanvas()->fillSmoothCircle(60, 91 + Y_OFFSET, 21, THEME_COLOR_BLACK);
            }
            else
            {
                HAL::GetCanvas()->fillSmoothCircle(60, 91 + Y_OFFSET, 32, eyeColor);
                HAL::GetCanvas()->fillSmoothCircle(60, 91 + Y_OFFSET, 21, THEME_COLOR_BLACK);
                HAL::GetCanvas()->fillSmoothCircle(180, 91 + Y_OFFSET, 32, eyeColor);
                HAL::GetCanvas()->fillSmoothCircle(180, 91 + Y_OFFSET, 21, THEME_COLOR_BLACK);
            }
            break;

        case EXPR_BLINK:
            if (isAmillion)
            {
                HAL::GetCanvas()->fillSmoothRoundRect(29, 86 + Y_OFFSET, 64, 11, 2, eyeColor);
            }
            else
            {
                HAL::GetCanvas()->fillSmoothRoundRect(29, 86 + Y_OFFSET, 64, 11, 2, eyeColor);
                HAL::GetCanvas()->fillSmoothRoundRect(149, 86 + Y_OFFSET, 64, 11, 2, eyeColor);
            }
            break;

        case EXPR_SMILE:
            HAL::GetCanvas()->setColor(eyeColor);
            if (isAmillion)
            {
                HAL::GetCanvas()->fillArc(60, 150 + Y_OFFSET, 61, 71, 241, 299);
            }
            else
            {
                HAL::GetCanvas()->fillArc(60, 150 + Y_OFFSET, 61, 71, 241, 299);
                HAL::GetCanvas()->fillArc(180, 150 + Y_OFFSET, 61, 71, 241, 299);
            }
            break;

        case EXPR_ANGER:
            HAL::GetCanvas()->setColor(eyeColor);
            if (isAmillion)
            {
                HAL::GetCanvas()->fillSmoothCircle(60, 91 + Y_OFFSET, 32, eyeColor);
                HAL::GetCanvas()->fillSmoothCircle(60, 91 + Y_OFFSET, 21, THEME_COLOR_BLACK);
                // Amillion 不画额外三角
            }
            else
            {
                HAL::GetCanvas()->fillSmoothCircle(60, 91 + Y_OFFSET, 32, eyeColor);
                HAL::GetCanvas()->fillSmoothCircle(60, 91 + Y_OFFSET, 21, THEME_COLOR_BLACK);
                HAL::GetCanvas()->fillSmoothCircle(180, 91 + Y_OFFSET, 32, eyeColor);
                HAL::GetCanvas()->fillSmoothCircle(180, 91 + Y_OFFSET, 21, THEME_COLOR_BLACK);
                HAL::GetCanvas()->fillTriangle(0, 30 + Y_OFFSET, 60, 91 + Y_OFFSET, 150, 91 + Y_OFFSET, THEME_COLOR_BLACK);
                HAL::GetCanvas()->fillTriangle(240, 30 + Y_OFFSET, 180, 91 + Y_OFFSET, 90, 91 + Y_OFFSET, THEME_COLOR_BLACK);
            }
            break;

        case EXPR_SAD:
            HAL::GetCanvas()->setColor(eyeColor);
            if (isAmillion)
            {
                HAL::GetCanvas()->fillSmoothCircle(60, 91 + Y_OFFSET, 32, eyeColor);
                HAL::GetCanvas()->fillSmoothCircle(60, 91 + Y_OFFSET, 21, THEME_COLOR_BLACK);
                HAL::GetCanvas()->fillTriangle(20, 91 + Y_OFFSET, 50, 0 + Y_OFFSET, 100, 70 + Y_OFFSET, THEME_COLOR_BLACK);
            }
            else
            {
                HAL::GetCanvas()->fillSmoothCircle(60, 91 + Y_OFFSET, 32, eyeColor);
                HAL::GetCanvas()->fillSmoothCircle(60, 91 + Y_OFFSET, 21, THEME_COLOR_BLACK);
                HAL::GetCanvas()->fillSmoothCircle(180, 91 + Y_OFFSET, 32, eyeColor);
                HAL::GetCanvas()->fillSmoothCircle(180, 91 + Y_OFFSET, 21, THEME_COLOR_BLACK);
                HAL::GetCanvas()->fillTriangle(20, 91 + Y_OFFSET, 50, 0 + Y_OFFSET, 100, 70 + Y_OFFSET, THEME_COLOR_BLACK);
                HAL::GetCanvas()->fillTriangle(220, 91 + Y_OFFSET, 190, 0 + Y_OFFSET, 140, 70 + Y_OFFSET, THEME_COLOR_BLACK);
            }
            break;

        case EXPR_WINCE:
            if (isAmillion)
            {
                // 单眼的斜眼表情与遮挡
                HAL::GetCanvas()->fillSmoothTriangle(95, 91 + Y_OFFSET, 33, 55 + Y_OFFSET, 33, 127 + Y_OFFSET, eyeColor);
                HAL::GetCanvas()->fillSmoothTriangle(73, 91 + Y_OFFSET, 33, 67.72 + Y_OFFSET, 33, 114.28 + Y_OFFSET, THEME_COLOR_BLACK);

                HAL::GetCanvas()->fillSmoothTriangle(33, 73 + Y_OFFSET, 33, 55 + Y_OFFSET, 53, 55 + Y_OFFSET, THEME_COLOR_BLACK);
                HAL::GetCanvas()->fillSmoothTriangle(33, 109 + Y_OFFSET, 33, 127 + Y_OFFSET, 53, 127 + Y_OFFSET, THEME_COLOR_BLACK);

                HAL::GetCanvas()->fillTriangle(90, 55 + Y_OFFSET, 90, 127 + Y_OFFSET, 120, 91 + Y_OFFSET, THEME_COLOR_BLACK);
            }
            else
            {
                HAL::GetCanvas()->fillSmoothTriangle(95, 91 + Y_OFFSET, 33, 55 + Y_OFFSET, 33, 127 + Y_OFFSET, eyeColor);
                HAL::GetCanvas()->fillSmoothTriangle(73, 91 + Y_OFFSET, 33, 67.72 + Y_OFFSET, 33, 114.28 + Y_OFFSET, THEME_COLOR_BLACK);
                HAL::GetCanvas()->fillSmoothTriangle(145, 91 + Y_OFFSET, 207, 55 + Y_OFFSET, 207, 127 + Y_OFFSET, eyeColor);
                HAL::GetCanvas()->fillSmoothTriangle(167, 91 + Y_OFFSET, 207, 67.72 + Y_OFFSET, 207, 114.28 + Y_OFFSET, THEME_COLOR_BLACK);

                HAL::GetCanvas()->fillSmoothTriangle(33, 73 + Y_OFFSET, 33, 55 + Y_OFFSET, 53, 55 + Y_OFFSET, THEME_COLOR_BLACK);
                HAL::GetCanvas()->fillSmoothTriangle(33, 109 + Y_OFFSET, 33, 127 + Y_OFFSET, 53, 127 + Y_OFFSET, THEME_COLOR_BLACK);

                HAL::GetCanvas()->fillSmoothTriangle(207, 73 + Y_OFFSET, 207, 55 + Y_OFFSET, 187, 55 + Y_OFFSET, THEME_COLOR_BLACK);
                HAL::GetCanvas()->fillSmoothTriangle(207, 109 + Y_OFFSET, 207, 127 + Y_OFFSET, 187, 127 + Y_OFFSET, THEME_COLOR_BLACK);

                HAL::GetCanvas()->fillTriangle(90, 55 + Y_OFFSET, 90, 127 + Y_OFFSET, 120, 91 + Y_OFFSET, THEME_COLOR_BLACK);
                HAL::GetCanvas()->fillTriangle(150, 55 + Y_OFFSET, 150, 127 + Y_OFFSET, 120, 91 + Y_OFFSET, THEME_COLOR_BLACK);
            }
            break;
        
        case EXPR_SLEEP:
            if (isAmillion)
            {
                HAL::GetCanvas()->fillSmoothRoundRect(29, 86 + Y_OFFSET, 64, 11, 2, eyeColor);
            }
            else
            {
                HAL::GetCanvas()->fillSmoothRoundRect(29, 86 + Y_OFFSET, 64, 11, 2, eyeColor);
                HAL::GetCanvas()->fillSmoothRoundRect(149, 86 + Y_OFFSET, 64, 11, 2, eyeColor);
            }
            break;
    }

    //HAL::CanvasUpdate();
}

//------------------------------------------状态栏------------------------------------------

// 显示一条状态栏消息，durationMs=0 则使用默认显示时长
void AppBangboo::showStatusMessage(const char* msg, uint32_t durationMs) {
    if (msg == nullptr) {
        return;
    }
    _data.statusBarMessage = msg;
    // 设定消息过期时间
    uint32_t now = HAL::Millis();
    uint32_t showDuration = durationMs == 0 ? _data.statusBarDisplayDuration : durationMs;
    _data.statusBarDisplayDuration = showDuration;  // 更新显示时长
    _data.statusBarMessageExpire = now + showDuration + 600;

    // 如果状态栏已可见且不在动画中，仅刷新显示时间，避免重复触发弹入动画
    if (_data.statusBarVisible && !_data.statusBarAnimating) {
        _data.statusBarShowTime = now;
        return;
    }

    // 触发状态栏显示 
    _show_status_bar();
}

void AppBangboo::_update_clock(bool updateNow)
{
    if ((HAL::Millis() - _data.clock_update_count) > _data.clock_update_interval || updateNow)
    {
        // 更新时钟
        strftime(_data.string_buffer, 10, "%H:%M", HAL::GetLocalTime());
        _data.clock = _data.string_buffer;
        _data.clock_update_count = HAL::Millis();
    }
}

void AppBangboo::_render_status_bar()
{
    if (!_data.statusBarVisible && !_data.statusBarAnimating) {
        return;
    }

    // 获取当前动画值
    float anim_value = _data.statusBarAnim.getValue(HAL::Millis());
    
    if (anim_value > 0) {
        // 计算圆角矩形尺寸
        int rect_width = 70;  // 适应大字体的宽度
        int rect_height = 24; // 适应大字体的高度
        int rect_x = (240 - rect_width) / 2; // 居中
        
        // 使用动画值控制下滑/上浮效果
        int rect_y;
        if (_data.statusBarAnimating && !_data.statusBarVisible) {
            // 进场动画 
            rect_y = anim_value - rect_height;
        } else {
            // 退场动画
            rect_y = 5 - rect_height + (anim_value * rect_height / 24);
        }

        // 绘制圆角矩形背景
        HAL::GetCanvas()->fillSmoothRoundRect(rect_x, rect_y, rect_width, rect_height, 8, THEME_COLOR_NIGHT);
        
        // 决定显示内容：优先显示消息，否则显示时间
        const char* toShow = nullptr;
        uint32_t now = HAL::Millis();

        if(_data.statusBarMessage == "time") {
            toShow = _data.clock.c_str();
        } else {
            toShow = _data.statusBarMessage.c_str();
        }

        HAL::GetCanvas()->setTextSize(2);
        HAL::GetCanvas()->setTextColor(TFT_WHITE, THEME_COLOR_NIGHT);
        HAL::GetCanvas()->drawCenterString(toShow, 120, rect_y + 4, &fonts::Font0);

        // 检查动画是否结束
        if (_data.statusBarAnimating) {
            if (_data.statusBarAnim.isFinished(HAL::Millis())) {
                _data.statusBarAnimating = false;
                // 如果是隐藏动画结束，将状态栏标记为不可见
                if (anim_value <= 0) {
                    _data.statusBarVisible = false;
                }
            }
        }
    }
}

void AppBangboo::_show_status_bar()
{
    // 如果状态栏已经可见，只重置显示时间
    if (_data.statusBarVisible && !_data.statusBarAnimating) {
        _data.statusBarShowTime = HAL::Millis();
        return;
    }

    // 如果正在隐藏动画中，重新显示
    _data.statusBarVisible = true;
    _data.statusBarAnimating = true;
    _data.statusBarShowTime = HAL::Millis();

    // 设置下滑动画
    _data.statusBarAnim.setAnim(
        LVGL::ease_out,  // 缓动函数
        0,               // 起始值
        24,              // 结束值
        600             // 动画时长(ms)
    );
    _data.statusBarAnim.resetTime(HAL::Millis());
}

void AppBangboo::_hide_status_bar()
{
    if (!_data.statusBarVisible || _data.statusBarAnimating) {
        return;
    }

    _data.statusBarAnimating = true;
    // 注意：_data.statusBarVisible 将在隐藏动画结束时设置为 false

    // 设置上浮动画
    _data.statusBarAnim.setAnim(
        LVGL::ease_out,  // 缓动函数
        24,              // 起始值
        -10,               // 结束值
        600             // 动画时长(ms)
    );
    _data.statusBarAnim.resetTime(HAL::Millis());
}

void AppBangboo::render(){
    HAL::GetCanvas()->fillScreen(THEME_COLOR_BLACK);
    drawExpression(_data.currentExpression);
    _render_status_bar();
    HAL::CanvasUpdate();
}
