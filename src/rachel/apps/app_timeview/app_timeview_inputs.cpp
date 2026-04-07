/**
 * @file app_timeview_inputs.cpp
 */
#include "app_timeview.h"
#include "../../hal/hal.h"

using namespace MOONCAKE::APPS;

void AppTimeview::_handleInputs()
{
    uint32_t now = HAL::Millis();

    // START 长按 1s 打开弹窗
    bool startDown = HAL::GetButton(GAMEPAD::BTN_START);
    if (startDown) {
        if (!_data.startWasPressed) {
            _data.startWasPressed = true;
            _data.startPressStartMs = now;
        } else {
            if (!_data.popupVisible && (now - _data.startPressStartMs >= 1000)) {
                const char* labels[] = {"CLOCK", "STOPWATCH", "TIMER"};
                _data.popupSelectIndex = (int)_data.currentMode;
                _data.popupOpenedByLongPress = true;
                _showPopup(labels[_data.popupSelectIndex], true);  // 长按START使用Y轴动画
            }
        }
    } else {
        if (_data.startWasPressed) {
            _data.startWasPressed = false;
            if (_data.popupVisible) {
                if (_data.popupOpenedByLongPress) {
                    // 长按松开，保持弹窗4s等待确认
                    _data.popupOpenedByLongPress = false;
                    _data.popupDisplayDuration = 4000;  // 4s
                    _data.popupShowTime = now;
                } else {
                    // 正常确认
                    _data.currentMode = (Data_t::Mode_t)_data.popupSelectIndex;
                    _hidePopup();
                    _data.popupDisplayDuration = 6000;  // 恢复默认6s
                }
            } else {
                if (_data.currentMode == Data_t::MODE_STOPWATCH) {
                    if (!_data.swRunning) {
                        _data.swRunning = true;
                        _data.swStartMs = now;
                    } else {
                        _data.swRunning = false;
                        _data.swAccumulatedMs += (now - _data.swStartMs);
                    }
                } else if (_data.currentMode == Data_t::MODE_TIMER) {
                    if (!_data.timerRunning) {
                        if (_data.timerRemainMs == 0 && _data.timerTotalMs > 0) {
                            _data.timerRemainMs = _data.timerTotalMs;
                        }
                        _data.timerRunning = (_data.timerRemainMs > 0);
                    } else {
                        _data.timerRunning = false;
                    }
                }
            }
        }
    }

    // RIGHT：切换弹窗选择；或 RESET；长按进入 TIMER 设置
    bool rightDown = HAL::GetButton(GAMEPAD::BTN_RIGHT);
    if (rightDown) {
        if (!_data.rightWasPressed) {
            _data.rightWasPressed = true;
            _data.rightPressStartMs = now;
            if (_data.popupVisible) {
                _data.popupSelectIndex = (_data.popupSelectIndex + 1) % 3;
                const char* labels[] = {"CLOCK", "STOPWATCH", "TIMER"};
                _updatePopupText(labels[_data.popupSelectIndex]);  // 仅更新文本，不重置动画状态
                _data.popupShowTime = now;  // 重置显示时间，刷新倒计时
            }
        } else {
            if (_data.currentMode == Data_t::MODE_TIMER && !_data.popupVisible) {
                if (now - _data.rightPressStartMs >= 800) {
                    _enterTimerSettings();
                    // 清理按键状态，防止设置完成后松开时触发短按逻辑
                    _data.rightWasPressed = false;
                    return;  // 立即返回，避免继续处理其他逻辑
                }
            }
        }
    } else {
        if (_data.rightWasPressed) {
            _data.rightWasPressed = false;
            if (!_data.popupVisible) {
                if (_data.currentMode == Data_t::MODE_STOPWATCH) {
                    _data.swRunning = false;
                    _data.swStartMs = 0;
                    _data.swAccumulatedMs = 0;
                } else if (_data.currentMode == Data_t::MODE_TIMER) {
                    _data.timerRunning = false;
                    _data.timerRemainMs = _data.timerTotalMs;
                } else if (_data.currentMode == Data_t::MODE_CLOCK) {
                    _data.clockFormat = (_data.clockFormat == Data_t::CLOCK_FMT_WDAY_DAY)
                        ? Data_t::CLOCK_FMT_MON_DAY
                        : Data_t::CLOCK_FMT_WDAY_DAY;
                }
            }
        }
    }
}


