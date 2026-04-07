/**
 * @file menu_render_callback.hpp
 * @author Forairaaaaa
 * @brief
 * @version 0.1
 * @date 2023-11-05
 *
 * @copyright Copyright (c) 2023
 *
 */
#pragma once
#include "../../../hal/hal.h"
#include "../../utils/smooth_menu/simple_menu/simple_menu.h"
#include "../../assets/theme/theme.h"
#include "../assets/launcher_bottom_panel.hpp"
#include <cmath>
#include <string>

class LauncherRenderCallBack : public SMOOTH_MENU::SimpleMenuCallback_t
{
private:
    int _x_offset;
    int _y_offset;
    int32_t _anim_value_buffer;
    std::string* _clock;

public:
    LauncherRenderCallBack() : _x_offset(0), _y_offset(0), _anim_value_buffer(0), _clock(nullptr) {}

    LVGL::Anim_Path statusBarAnim;
    LVGL::Anim_Path bottomPanelAnim;
    inline void setClock(std::string* clock) { _clock = clock; }

    /* Override render callback */
    void renderCallback(const std::vector<SMOOTH_MENU::Item_t*>& menuItemList,
                        const SMOOTH_MENU::RenderAttribute_t& selector,
                        const SMOOTH_MENU::RenderAttribute_t& camera) override
    {
        // Clear
        HAL::GetCanvas()->fillScreen(THEME_COLOR_DARK);

        // Render status bar
        /* --------------------------------------------------------------------------------------------- */
        // HAL::GetCanvas()->fillRect(0, 0, 240, 24, THEME_COLOR_LIGHT);

        // With anim
        // _anim_value_buffer = statusBarAnim.getValue(HAL::Millis());
        // HAL::GetCanvas()->fillRect(0, 0, 240, _anim_value_buffer, THEME_COLOR_NIGHT);
        // HAL::GetCanvas()->setTextSize(2);
        // HAL::GetCanvas()->setTextColor(TFT_WHITE, THEME_COLOR_NIGHT);
        // HAL::GetCanvas()->drawCenterString(_clock->c_str(), 120, _anim_value_buffer - 19, &fonts::Font0);
        // HAL::GetCanvas()->setTextSize(1);

        // With anim - 圆角矩形包裹时间
        _anim_value_buffer = statusBarAnim.getValue(HAL::Millis());
        if (_anim_value_buffer > 0) {
            // 计算圆角矩形尺寸
            int rect_width = 70;  // 适应大字体的宽度
            int rect_height = 24; // 适应大字体的高度
            int rect_x = (240 - rect_width) / 2; // 居中
            // 使用动画值控制下滑效果，顶住屏幕顶部
            int rect_y = 5 - rect_height + (_anim_value_buffer * rect_height / 24); // 从上方滑下，最终y=0

            // 绘制圆角矩形背景
            HAL::GetCanvas()->fillSmoothRoundRect(rect_x, rect_y, rect_width, rect_height, 8, THEME_COLOR_NIGHT);
            
            // 绘制时间文字
            HAL::GetCanvas()->setTextSize(2);
            HAL::GetCanvas()->setTextColor(TFT_WHITE, THEME_COLOR_NIGHT);
            HAL::GetCanvas()->drawCenterString(_clock->c_str(), 120, rect_y + 4, &fonts::Font0);
        }

        /* --------------------------------------------------------------------------------------------- */

        // Render bottom panel
        /* --------------------------------------------------------------------------------------------- */
        // HAL::GetCanvas()->pushImage(0, 210, 240, 30, image_data_launcher_bottom_panel);

        // With anim
        _anim_value_buffer = bottomPanelAnim.getValue(HAL::Millis());
        HAL::GetCanvas()->pushImage(0, _anim_value_buffer, 240, 30, image_data_launcher_bottom_panel);
        /* --------------------------------------------------------------------------------------------- */

        // Render items (Icons)
        /* --------------------------------------------------------------------------------------------- */
        // Get x offset (fixed selector, icon moves)
        _x_offset = -(selector.x) + HAL::GetCanvas()->width() / 2 - THEME_APP_ICON_WIDTH_HALF;

        for (const auto& item : menuItemList)
        {
            // Get y offset, lower the unselected one
            _y_offset = std::abs(selector.x - item->x) / 8;

            // Render icon
            HAL::GetCanvas()->pushImage(item->x + _x_offset,
                                        item->y + _y_offset,
                                        THEME_APP_ICON_WIDTH,
                                        THEME_APP_ICON_HEIGHT,
                                        (const uint16_t*)(item->userData));

            // If is the selected one
            if (item->id == selector.targetItem)
            {
                // // Render app name
                // HAL::GetCanvas()->setTextColor(THEME_COLOR_DARK, THEME_COLOR_LIGHT);
                // // HAL::GetCanvas()->drawCenterString(
                // //     item->tag.c_str(),
                // //     120,
                // //     THEME_APP_NAME_MARGIN_TOP
                // // );

                // // With anim
                // HAL::GetCanvas()->drawCenterString(item->tag.c_str(), 120, _anim_value_buffer + 15);
                HAL::GetCanvas()->setTextColor(TFT_WHITE);
                HAL::GetCanvas()->setTextSize(1);
                HAL::GetCanvas()->drawCenterString(
                    item->tag.c_str(), 
                    120, 
                    THEME_APP_ICON_MARGIN_TOP + THEME_APP_ICON_HEIGHT + 6,
                    &fonts::Font0
                );
            }
        }
        /* --------------------------------------------------------------------------------------------- */
    }
};
