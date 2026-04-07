/**
 * @file progress_window.cpp
 * @author Bowen
 * @brief 
 * @version 0.1
 * @date 2025-07-13
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#include "progress_window.h"
#include "../../../../../hal/hal.h"
#include "../common/data_structs.hpp"
#include "../../../../assets/theme/theme.h"
#include "../../../../assets/fonts/fonts.h"
#include <cmath>


void SYSTEM::UI::ProgressWindow(std::string title, uint8_t progress, bool lightMode, bool useCanvas)
{
    // 设置颜色方案：黑色背景
    uint32_t background_color = THEME_COLOR_BLACK;
    uint32_t text_color = THEME_COLOR_LIGHT;
    uint32_t ring_bg_color = THEME_COLOR_DARK2;
    uint32_t ring_progress_color = THEME_COLOR_LIGHT; 

    // Clear screen - 黑色背景
    HAL::GetCanvas()->fillScreen(background_color);

    // 计算圆角矩形进度条参数
    int center_x = HAL::GetCanvas()->width() / 2;
    int center_y = HAL::GetCanvas()->height() / 4 + 34; // 稍微下移一点给标题留空间
    int bar_width = HAL::GetCanvas()->width() * 2 / 3; // 进度条宽度为屏幕宽度的2/3
    int bar_height = 16; // 进度条高度
    int corner_radius = 4; // 圆角半径
    
    // 计算进度条位置
    int bar_x = center_x - bar_width / 2;
    int bar_y = center_y - bar_height / 2;
    
    // 限制进度值
    if (progress > 100)
        progress = 100;
    
    // 绘制进度条背景 (圆角矩形)
    HAL::GetCanvas()->fillRoundRect(bar_x, bar_y, bar_width, bar_height, corner_radius, ring_bg_color);
    
    // 绘制进度条 (白色实心圆角矩形)
    if (progress > 0) {
        int progress_width = bar_width * progress / 100; // 计算进度对应的宽度
        if (progress_width > 0) {
            HAL::GetCanvas()->fillRoundRect(bar_x, bar_y, progress_width, bar_height, corner_radius, ring_progress_color);
        }
    }

    // 在进度条上方显示标题
    HAL::GetCanvas()->setTextColor(text_color, background_color);
    HAL::GetCanvas()->setFont(&fonts::Font0);
    HAL::GetCanvas()->setTextSize(1);
    HAL::GetCanvas()->drawCenterString(
        title.c_str(), 
        center_x,
        bar_y - 25  // 在进度条上方显示标题
    );
    
    // 在标题上方显示进度数值
    char progress_text[8];
    sprintf(progress_text, "%d%%", progress);
    HAL::GetCanvas()->setTextSize(2);
    HAL::GetCanvas()->drawCenterString(
        progress_text, 
        center_x,
        bar_y - 50  // 在标题上方显示百分比
    );
}

