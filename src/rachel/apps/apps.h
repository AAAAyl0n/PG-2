/**
 * @file apps.h
 * @author Bowen
 * @brief
 * @version 0.2
 * @date 2025-10-24
 *
 * @copyright Copyright (c) 2025
 *
 */
#pragma once
#include "launcher/launcher.h"
#include "app_template/app_template.h"
#include "app_settings/app_settings.h"
//#include "app_screencast/app_screencast.h"
#include "app_music/app_music.h"
#include "app_genshin/app_genshin.h"
//#include "app_raylib_games/app_raylib_games.h"
//#include "app_ble_gamepad/app_ble_gamepad.h"
//#include "app_nofrendo/app_nofrendo.h"
#include "app_bangboo/app_bangboo.h"
#include "app_timeview/app_timeview.h"
#include "app_imutest/app_imutest.h"
#include "app_asciiart/app_asciiart.h"
#include "app_poweroff/app_poweroff.h"
#include "app_recorder/app_recorder.h"
/* Header files locator(Don't remove) */

void rachel_app_install_callback(MOONCAKE::Mooncake* mooncake)
{
    mooncake->installApp(new MOONCAKE::APPS::AppSettings_Packer);
    
    mooncake->installApp(new MOONCAKE::APPS::AppBangboo_Packer);
    mooncake->installApp(new MOONCAKE::APPS::AppTimeview_Packer);
    //mooncake->installApp(new MOONCAKE::APPS::AppImutest_Packer);
    //mooncake->installApp(new MOONCAKE::APPS::AppAsciiart_Packer);
    //mooncake->installApp(new MOONCAKE::APPS::AppGenshin_Packer);
    //mooncake->installApp(new MOONCAKE::APPS::AppRecorder_Packer);
    //mooncake->installApp(new MOONCAKE::APPS::AppMusic_Packer);
    mooncake->installApp(new MOONCAKE::APPS::AppPoweroff_Packer);
    
    /* Install app locator(Don't remove) */
}
