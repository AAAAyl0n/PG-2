/**
 * @file app_timeview_popup.cpp
 */
#include "app_timeview.h"
#include "../../hal/hal.h"
#include "../assets/theme/theme.h"
#include <cstring>

using namespace MOONCAKE::APPS;

void AppTimeview::_showPopup(const char* text, bool useYAnimation)
{
    if (text == nullptr) return;
    _data.popupText = text;
    _updatePopupWidthTarget(text);

    _data.popupVisible = true;
    _data.popupAnimating = true;
    _data.popupShowTime = HAL::Millis();
    _data.popupUseYAnimation = useYAnimation;
    
    if (useYAnimation) {
        // 有Y轴弹出动画
        _data.popupSlideAnim.setAnim(LVGL::ease_out, 0, 24, 300);
    } else {
        // 无Y轴动画，直接到位
        _data.popupSlideAnim.setAnim(LVGL::ease_out, 24, 24, 1);
    }
    _data.popupSlideAnim.resetTime(HAL::Millis());
}

void AppTimeview::_hidePopup()
{
    if (!_data.popupVisible || _data.popupAnimating) return;
    _data.popupAnimating = true;
    _data.popupSlideAnim.setAnim(LVGL::ease_out, 24, -10, 300);
    _data.popupSlideAnim.resetTime(HAL::Millis());
}

void AppTimeview::_updatePopupWidthTarget(const char* text)
{
    int charCount = (int)strlen(text);
    int estimatedWidth = charCount * 11 + 10 * 2;  // 字符11px，两边间距各10px
    if (estimatedWidth < 50) estimatedWidth = 50;
    if (estimatedWidth > 200) estimatedWidth = 200;

    _data.popupTargetWidth = estimatedWidth;
    _data.popupWidthAnim.setAnim(LVGL::ease_out, _data.popupCurrentWidth, _data.popupTargetWidth, 250);
    _data.popupWidthAnim.resetTime(HAL::Millis());
}

void AppTimeview::_renderPopup()
{
    if (_data.popupVisible && !_data.popupAnimating) {
        if (HAL::Millis() - _data.popupShowTime > _data.popupDisplayDuration) {
            _hidePopup();
        }
    }

    if (!_data.popupVisible && !_data.popupAnimating) return;

    int animValue = _data.popupSlideAnim.getValue(HAL::Millis());
    _data.popupCurrentWidth = _data.popupWidthAnim.getValue(HAL::Millis());

    if (animValue <= 0 && _data.popupAnimating) {
        if (_data.popupSlideAnim.isFinished(HAL::Millis())) {
            _data.popupAnimating = false;
            _data.popupVisible = false;
        }
        return;
    }

    int rect_width = _data.popupCurrentWidth > 0 ? _data.popupCurrentWidth : 50;
    int rect_height = 24;
    int rect_x = (240 - rect_width) / 2;
    int rect_y;
    
    if (_data.popupUseYAnimation) {
        // 使用Y轴动画
        if (_data.popupAnimating && !_data.popupVisible) {
            rect_y = animValue - rect_height;
        } else {
            rect_y = 5 - rect_height + (animValue * rect_height / 24);
        }
    } else {
        // 不使用Y轴动画，固定位置
        rect_y = 5 - rect_height + (24 * rect_height / 24);
    }

    HAL::GetCanvas()->fillSmoothRoundRect(rect_x, rect_y, rect_width, rect_height, 8, THEME_COLOR_NIGHT);
    HAL::GetCanvas()->setTextSize(2);
    HAL::GetCanvas()->setTextColor(TFT_WHITE, THEME_COLOR_NIGHT);
    HAL::GetCanvas()->drawCenterString(_data.popupText.c_str(), 120, rect_y + 4, &fonts::Font0);

    if (_data.popupAnimating && _data.popupSlideAnim.isFinished(HAL::Millis())) {
        _data.popupAnimating = false;
        if (animValue <= 0) {
            _data.popupVisible = false;
        }
    }
}

void AppTimeview::_updatePopupText(const char* text)
{
    if (text == nullptr || !_data.popupVisible) return;
    _data.popupText = text;
    _updatePopupWidthTarget(text);
    // 不重置动画状态，保持当前显示状态
}


