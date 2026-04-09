/**
 * @file app_asciiart.h
 * @author Forairaaaaa
 * @brief Joystick test app
 * @version 0.2
 * @date 2023-11-04
 *
 * @copyright Copyright (c) 2023
 *
 */
#include <mooncake.h>
#include "../assets/icons/icons.h"
#include "assets/icon_app_asciiart.hpp"

namespace MOONCAKE::APPS
{
    class AppAsciiart : public APP_BASE
    {
    private:
        struct Data_t
        {
            float dot_x = 120.0f;   // 圆点屏幕坐标
            float dot_y = 120.0f;
            int raw_x = 2048;       // 原始 ADC 值
            int raw_y = 2048;
            int center_x = 2048;    // 摇杆中位校准值
            int center_y = 2048;
            bool calibrated = false;
        };
        Data_t _data;

    public:
        void onCreate() override;
        void onResume() override;
        void onRunning() override;
        void onDestroy() override;
    };

    class AppAsciiart_Packer : public APP_PACKER_BASE
    {
        std::string getAppName() override { return "Check"; }
        void* getAppIcon() override { return (void*)image_data_icon_app_asciiart; }
        void* newApp() override { return new AppAsciiart; }
        void deleteApp(void* app) override { delete (AppAsciiart*)app; }
    };
}
