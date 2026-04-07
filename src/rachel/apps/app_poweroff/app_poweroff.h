/**
 * @file app_poweroff.h
 * @author Forairaaaaa
 * @brief
 * @version 0.1
 * @date 2023-11-04
 *
 * @copyright Copyright (c) 2023
 *
 */
#include <mooncake.h>
#include "../assets/icons/icons.h"
#include "assets/icon_app_poweroff.hpp"

namespace MOONCAKE::APPS
{
    /**
     * @brief Poweroff
     *
     */
    class AppPoweroff : public APP_BASE
    {
    private:
        struct Data_t
        {
            unsigned long count = 0;
        };
        Data_t _data;

    public:
        void onCreate() override;
        void onResume() override;
        void onRunning() override;
        void onDestroy() override;
    };
    

    class AppPoweroff_Packer : public APP_PACKER_BASE
    {
        std::string getAppName() override { return "Poweroff"; }
        void* getAppIcon() override { return (void*)image_data_icon_app_poweroff; }
        void* newApp() override { return new AppPoweroff; }
        void deleteApp(void* app) override { delete (AppPoweroff*)app; }
    };
} // namespace MOONCAKE::APPS