/**
 * @file menu.cpp
 * @author Forairaaaaa
 * @brief
 * @version 0.1
 * @date 2023-11-05
 *
 * @copyright Copyright (c) 2023
 *
 */
#include "../launcher.h"
#include "lgfx/v1/lgfx_fonts.hpp"
#include "menu_render_callback.hpp"
#include "spdlog/spdlog.h"

using namespace MOONCAKE::APPS;

void Launcher::_create_menu()
{
    spdlog::info("create menu");

    // Create menu and render callback
    _data.menu = new SMOOTH_MENU::Simple_Menu(HAL::GetCanvas()->width(), HAL::GetCanvas()->height());
    _data.menu_render_cb = new LauncherRenderCallBack;

    // Setup
    _data.menu->setRenderCallback(_data.menu_render_cb);
    _data.menu->setFirstItem(1);

    // Set selector anim, in this launcher case, is the icon's moving anim (fixed selector)
    auto cfg_selector = _data.menu->getSelector()->config();
    cfg_selector.animPath_x = LVGL::ease_out;
    cfg_selector.animTime_x = 300;
    _data.menu->getSelector()->config(cfg_selector);

    // Set menu open anim
    auto cfg_menu = _data.menu->getMenu()->config();
    cfg_menu.animPath_open = LVGL::ease_out;
    cfg_menu.animTime_open = 1000;
    _data.menu->getMenu()->config(cfg_menu);

    // Record when menu open animation ends to block START during opening
    _data.menu_open_end_time = HAL::Millis() + cfg_menu.animTime_open;

    // Allow selector go loop
    _data.menu->setMenuLoopMode(true);

    // Get installed app list
    spdlog::info("installed apps num: {}", mcAppGetFramework()->getAppRegister().getInstalledAppNum());
    int i = 0;
    for (const auto& app : mcAppGetFramework()->getAppRegister().getInstalledAppList())
    {
        // Pass the launcher
        if (app->getAddr() == getAppPacker())
            continue;

        // spdlog::info("app: {} icon: {}", app->getAppName(), app->getAppIcon());
        spdlog::info("push app: {} into menu", app->getAppName());

        // Push items into menu, use icon pointer as the item user data
        _data.menu->getMenu()->addItem(app->getAppName(),
                                       THEME_APP_ICON_GAP + i * (THEME_APP_ICON_WIDTH + THEME_APP_ICON_GAP),
                                       THEME_APP_ICON_MARGIN_TOP,
                                       THEME_APP_ICON_WIDTH,
                                       THEME_APP_ICON_HEIGHT,
                                       app->getAppIcon());
        i++;
    }

    // Setup some widget shit
    // Pass clock string pointer for redner
    ((LauncherRenderCallBack*)_data.menu_render_cb)->setClock(&_data.clock);

    // Setup anims
    ((LauncherRenderCallBack*)_data.menu_render_cb)->statusBarAnim.setAnim(LVGL::ease_out, 0, 24, 600);
    ((LauncherRenderCallBack*)_data.menu_render_cb)->statusBarAnim.resetTime(HAL::Millis() + 1000);

    ((LauncherRenderCallBack*)_data.menu_render_cb)->bottomPanelAnim.resetTime(HAL::Millis());
    ((LauncherRenderCallBack*)_data.menu_render_cb)->bottomPanelAnim.setAnim(LVGL::ease_out, 240, 210, 600);
}

void Launcher::_update_menu()
{
    
    if ((HAL::Millis() - _data.menu_update_count) > _data.menu_update_interval)
    {
        // 检查按钮输入
        bool any_button_pressed = false;

        // Update navigation - 适应新的三键配置
        // SELECT 键向前导航
        if (HAL::GetButton(GAMEPAD::BTN_SELECT))
        {
            any_button_pressed = true;
            if (!_data.menu_wait_button_released)
            {
                HAL::PlayWavFile("/system_audio/Klick.wav");
                _data.menu->goLast();
                _data.menu_wait_button_released = true;
            }
        }

        // RIGHT 键向后导航
        else if (HAL::GetButton(GAMEPAD::BTN_RIGHT))
        {
            any_button_pressed = true;
            if (!_data.menu_wait_button_released)
            {
                HAL::PlayWavFile("/system_audio/Klick.wav");
                _data.menu->goNext();
                _data.menu_wait_button_released = true;
            }
        }

        // START 键打开应用
        else if (HAL::GetButton(GAMEPAD::BTN_START) && HAL::Millis() >= _data.menu_open_end_time)
        {
            any_button_pressed = true;
            auto selected_item = _data.menu->getSelector()->getTargetItem();
            // spdlog::info("select: {} try create", selected_item);

            // Skip launcher
            selected_item++;
            // Get packer, apps are arranged by install order, so simply use index is ok
            auto app_packer = mcAppGetFramework()->getInstalledAppList()[selected_item];
            // spdlog::info("try create app: {}", app_packer->getAppName());
            // Try create and start app
            if (mcAppGetFramework()->createAndStartApp(app_packer))
            {
                HAL::PlayWavFile("/system_audio/Enter.wav");
                spdlog::info("app: {} opened", app_packer->getAppName());
                closeApp();
            }
            else
                spdlog::error("open app: {} failed", app_packer->getAppName());
        }

        // Unlock if no button is pressing
        else
        {
            _data.menu_wait_button_released = false;
        }

        // 更新最后输入时间
        if (any_button_pressed)
        {
            _data.last_input_time = HAL::Millis();
        }

        // 检查自动启动条件
        if (_data.auto_startup_enabled && 
            (HAL::Millis() - _data.last_input_time) > _data.auto_startup_delay)
        {
            // 查找 Bangboo 应用
            auto app_list = mcAppGetFramework()->getInstalledAppList();
            for (size_t i = 1; i < app_list.size(); i++) { // 跳过 launcher (index 0)
                if (app_list[i]->getAppName() == _data.auto_startup_app_name) {
                    if (mcAppGetFramework()->createAndStartApp(app_list[i])) {
                        spdlog::info("app: {} auto opened after 10s of inactivity", app_list[i]->getAppName());
                        closeApp();
                        return; // 启动成功后直接返回
                    }
                    break;
                }
            }
            
            // 如果没找到Bangboo应用，重置计时器以避免一直尝试
            _data.last_input_time = HAL::Millis();
        }

        // Update menu
        _data.menu->update(HAL::Millis());

        // Push frame buffer
        // HAL::RenderFpsPanel();
        HAL::CanvasUpdate();

        _data.menu_update_count = HAL::Millis();
    }
}

void Launcher::_destroy_menu()
{
    spdlog::info("destroy menu");
    delete _data.menu;
    delete _data.menu_render_cb;
}
