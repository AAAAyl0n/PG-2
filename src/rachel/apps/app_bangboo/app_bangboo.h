/**
 * @file app_bangboo.h
 * @brief USB MIDI Guitar App
 * @version 1.0
 * @date 2025-06-11
 */
#pragma once
#include <mooncake.h>
#include <string>
#include "assets/icon_app_bangboo.hpp"
#include "../assets/icons/icons.h"
#include "../utils/smooth_menu/lv_anim/lv_anim.h"

namespace MOONCAKE::APPS
{
    class AppBangboo : public APP_BASE
    {
    private:
        struct Data_t
        {
            bool stringTouched[6] = {false};
            uint8_t activeNote[6] = {0};   // 当前发声的 MIDI note (0=无)
            uint32_t releaseTime[6] = {0}; // 松手时间戳

            uint8_t sustainLevel = 0;      // 0=OFF, 1-4 = 500/1000/1500/2000ms
            bool joystickPressed = false;

            uint8_t currentChord = 0;      // 0-8 和弦索引
            uint32_t lastChordActivity = 0;// 上次摇杆推到非中心方向的时间
            int centerX = 2048;            // 摇杆校准中心
            int centerY = 2048;

            uint8_t groupIndex = 0;        // 组索引（不同八度）
            bool encoderCwPressed = false; // 编码器去抖
            bool encoderCcwPressed = false;

            // ---- Group-name dropdown popup（参考 timeview） ----
            std::string popupText;
            bool popupVisible = false;
            bool popupAnimating = false;
            uint32_t popupShowTime = 0;
            uint32_t popupDisplayDuration = 1500;  // 1.5s 后自动收起
            LVGL::Anim_Path popupSlideAnim;
        };
        Data_t _data;

        int _getJoystickZone();
        void _onChordChange(uint8_t newChord);
        void _render();
        void _showGroupPopup(const char* text);
        void _renderPopup();

    public:
        void onCreate() override;
        void onResume() override;
        void onRunning() override;
        void onDestroy() override;
    };

    class AppBangboo_Packer : public APP_PACKER_BASE
    {
        std::string getAppName() override { return "Daizy"; }
        void* getAppIcon() override { return (void*)image_data_icon_app_bangboo; }
        void* newApp() override { return new AppBangboo; }
        void deleteApp(void* app) override { delete (AppBangboo*)app; }
    };
} // namespace MOONCAKE::APPS
