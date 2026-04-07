/**
 * @file app_anim.cpp
 * @author Forairaaaaa
 * @brief
 * @version 0.1
 * @date 2023-11-05
 *
 * @copyright Copyright (c) 2023
 *
 */
#include "../launcher.h"
#include "menu_render_callback.hpp"

using namespace MOONCAKE::APPS;

void Launcher::_play_app_anim(bool open)
{
    if (open)
        _data.app_anim.setAnim(LVGL::ease_out, THEME_APP_ICON_WIDTH, 320, 360);
    else
        _data.app_anim.setAnim(LVGL::ease_out, 320, THEME_APP_ICON_WIDTH, 360);

    _data.app_anim.resetTime(HAL::Millis());

    int anim_value = 0;
    int offset = 0;
    while (!_data.app_anim.isFinished(HAL::Millis()))
    {
        if (!open)
            _data.menu->update(HAL::Millis());

        anim_value = _data.app_anim.getValue(HAL::Millis());
        offset = anim_value / 2;
        
        // 计算颜色过渡
        uint32_t fill_color;
        if (open) {
            // 打开动画：从 THEME_COLOR_LIGHT 到 THEME_COLOR_BLACK
            float progress = (float)(anim_value - THEME_APP_ICON_WIDTH) / (float)(320 - THEME_APP_ICON_WIDTH);
            progress = progress < 0.0f ? 0.0f : (progress > 1.0f ? 1.0f : progress); // 限制在0-1之间
            
            // 加速颜色过渡：当progress超过0.6时就完全变黑
            float color_progress = progress / 0.6f;
            color_progress = color_progress > 1.0f ? 1.0f : color_progress;
            
            // 颜色插值：从 0xEFEFEF (239,239,239) 到 0x000000 (0,0,0)
            uint8_t r = (uint8_t)(239 * (1.0f - color_progress));
            uint8_t g = (uint8_t)(239 * (1.0f - color_progress));
            uint8_t b = (uint8_t)(239 * (1.0f - color_progress));
            fill_color = (r << 16) | (g << 8) | b;
        } else {
            // 关闭动画：从 THEME_COLOR_BLACK 到 THEME_COLOR_LIGHT
            float progress = (float)(320 - anim_value) / (float)(320 - THEME_APP_ICON_WIDTH);
            progress = progress < 0.0f ? 0.0f : (progress > 1.0f ? 1.0f : progress); // 限制在0-1之间
            
            // 延迟颜色过渡：当progress超过0.8时才开始变浅色，让黑色持续更长时间
            float color_progress = 0.0f;
            if (progress > 0.4f) {
                color_progress = (progress - 0.4f) / 0.4f; // 从0.8-1.0映射到0-1
                if (color_progress > 1.0f) {
                    color_progress = 1.0f;
                }
            }
            
            // 颜色插值：从 0x000000 (0,0,0) 到 0xEFEFEF (239,239,239)
            uint8_t r = (uint8_t)(239 * color_progress);
            uint8_t g = (uint8_t)(239 * color_progress);
            uint8_t b = (uint8_t)(239 * color_progress);
            fill_color = (r << 16) | (g << 8) | b;
        }
        
        HAL::GetCanvas()->fillSmoothRoundRect(120 - offset,
                                              THEME_APP_ICON_MARGIN_TOP + THEME_APP_ICON_HEIGHT_HALF - offset,
                                              anim_value,
                                              anim_value,
                                              60,
                                              fill_color);

        HAL::CanvasUpdate();
    }
}
