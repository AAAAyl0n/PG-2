/**
 * @file app_bangboo.h
 * @brief USB MIDI Guitar App
 * @version 1.0
 * @date 2025-06-11
 */
#pragma once
#include <mooncake.h>
#include "assets/icon_app_bangboo.hpp"
#include "../assets/icons/icons.h"

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
            int centerX = 2048;            // 摇杆校准中心
            int centerY = 2048;
        };
        Data_t _data;

        int _getJoystickZone();
        void _onChordChange(uint8_t newChord);
        void _render();

    public:
        void onCreate() override;
        void onResume() override;
        void onRunning() override;
        void onDestroy() override;
    };

    class AppBangboo_Packer : public APP_PACKER_BASE
    {
        std::string getAppName() override { return "MIDI"; }
        void* getAppIcon() override { return (void*)image_data_icon_app_bangboo; }
        void* newApp() override { return new AppBangboo; }
        void deleteApp(void* app) override { delete (AppBangboo*)app; }
    };
} // namespace MOONCAKE::APPS
