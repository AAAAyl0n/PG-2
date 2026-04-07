/**
 * @file page_display.cpp
 * @author Forairaaaaa
 * @brief
 * @version 0.1
 * @date 2023-11-18
 *
 * @copyright Copyright (c) 2023
 *
 */
#include "../app_settings.h"
#include "../../utils/system/ui/ui.h"
#include "../../utils//system/inputs/inputs.h"
#include "../../../hal/hal.h"
#include "spdlog/spdlog.h"
#include <cstdint>
#include <string>

using namespace MOONCAKE::APPS;
using namespace SYSTEM::UI;
using namespace SYSTEM::INPUTS;

static const char* _sleep_timeout_text(uint8_t option)
{
    static const char* options[] = {"10min", "30min", "60min", "Never"};
    if (option > 3) option = 0;
    return options[option];
}

static void _handle_brightness_config()
{
    // 15~255 16/step
    int brightness = HAL::GetDisplay()->getBrightness();
    bool is_changed = true;

    // 使用新的三键配置
    Button btn_left(GAMEPAD::BTN_SELECT);    // SELECT 键减少亮度
    Button btn_right(GAMEPAD::BTN_RIGHT);    // RIGHT 键增加亮度
    Button btn_ok(GAMEPAD::BTN_START);       // START 键确认

    std::string title;
    while (1)
    {
        // Render brightness bar
        if (is_changed)
        {
            is_changed = false;

            // Apply change
            HAL::GetSystemConfig().brightness = brightness;
            HAL::GetDisplay()->setBrightness(HAL::GetSystemConfig().brightness);

            // Render brightness setting bar
            title = "Brightness " + std::to_string(brightness);
            ProgressWindow(title, brightness * 100 / 255);
            HAL::CanvasUpdate();
        }
        else
        {
            HAL::Delay(20);
        }

        // Read input
        if (btn_left.pressed())
        {
            brightness -= 16;
            if (brightness < 16)
                brightness = 16;
            is_changed = true;
        }
        else if (btn_right.pressed())
        {
            brightness += 16;
            if (brightness > 255)
                brightness = 255;
            is_changed = true;
        }
        else if (btn_ok.released())
        {
            break;
        }
    }
}

void AppSettings::_page_display()
{
    while (1)
    {
        std::vector<std::string> items = {"[DISPLAY]"};

        // Brightness
        items.push_back("Brightness  " + std::to_string(HAL::GetSystemConfig().brightness));

        // Device model select
        const char* model_names[] = {"Eous", "Amillion", "paperboo"};
        uint8_t model = HAL::GetSystemConfig().model;
        if (model > 2) model = 0;
        items.push_back(std::string("Device   ") + model_names[model]);

        // Auto sleep timeout select
        uint8_t sleep_timeout = HAL::GetSystemConfig().auto_sleep_timeout;
        if (sleep_timeout > 3) sleep_timeout = 0;
        items.push_back(std::string("Sleep    ") + _sleep_timeout_text(sleep_timeout));

        items.push_back("Back");

        auto selected = _data.select_menu->waitResult(items);

        // Brightness
        if (selected == 1)
        {
            _handle_brightness_config();
            _data.is_config_changed = true;
        }
        // Device model select
        else if (selected == 2)
        {
            std::vector<std::string> select_items = {"[DEVICE]", "Eous", "Amillion", "paperboo", "Back"};
            auto choose = _data.select_menu->waitResult(select_items);
            if (choose >= 1 && choose <= 3)
            {
                HAL::GetSystemConfig().model = static_cast<uint8_t>(choose - 1);
                _data.is_config_changed = true;
            }
        }
        // Auto sleep timeout select
        else if (selected == 3)
        {
            std::vector<std::string> select_items = {"[SLEEP TIMEOUT]", "10min", "30min", "60min", "Never", "Back"};
            auto choose = _data.select_menu->waitResult(select_items);
            if (choose >= 1 && choose <= 4)
            {
                HAL::GetSystemConfig().auto_sleep_timeout = static_cast<uint8_t>(choose - 1);
                _data.is_config_changed = true;
            }
        }
        else
            break;
    }
}
