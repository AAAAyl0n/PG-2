/**
 * @file page_sound.cpp
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
#include "../../../hal/hal.h"
#include "../../utils//system/inputs/inputs.h"

using namespace MOONCAKE::APPS;
using namespace SYSTEM::UI;
using namespace SYSTEM::INPUTS;

static void _handle_volume_config()
{
    // 使用 0~255 存储在配置中，但映射到 40~80 的实际音频音量接口
    int volume_0_255 = HAL::GetSystemConfig().volume;
    bool is_changed = true;

    // 三键：SELECT 减小，RIGHT 增大，START 确认
    Button btn_left(GAMEPAD::BTN_SELECT);
    Button btn_right(GAMEPAD::BTN_RIGHT);
    Button btn_ok(GAMEPAD::BTN_START);

    std::string title;
    while (1)
    {
        if (is_changed)
        {
            is_changed = false;

            // 应用到全局配置
            HAL::GetSystemConfig().volume = static_cast<uint8_t>(volume_0_255);

            // 映射到 40~80 的音频音量，并设置静音
            uint8_t actual_percent = static_cast<uint8_t>(40 + (volume_0_255 * 40 + 127) / 255);
            HAL::SetAudioVolume(actual_percent);
            HAL::SetAudioMute(HAL::GetSystemConfig().volume == 0);

            // 渲染 ProgressWindow（0~100 显示相对于 40~80 的进度）
            uint8_t bar_percent = static_cast<uint8_t>(((int)actual_percent - 40) * 100 / 40);
            title = "Volume " + std::to_string(actual_percent);
            ProgressWindow(title, bar_percent);
            HAL::CanvasUpdate();
        }
        else
        {
            HAL::Delay(20);
        }

        // 读取输入
        if (btn_left.pressed())
        {
            volume_0_255 -= 16;
            if (volume_0_255 < 0)
                volume_0_255 = 0;
            is_changed = true;
        }
        else if (btn_right.pressed())
        {
            volume_0_255 += 16;
            if (volume_0_255 > 255)
                volume_0_255 = 255;
            is_changed = true;
        }
        else if (btn_ok.released())
        {
            break;
        }
    }
}

void AppSettings::_page_sound()
{
    while (1)
    {
        std::vector<std::string> items = {"[SOUND]"};

        // 全局 Sound 开关（音量==0 视为 OFF）
        items.push_back(HAL::GetSystemConfig().volume == 0 ? "Sound    OFF" : "Sound    ON");

        // 展示为 40~80 的实际音量
        uint8_t show_percent = static_cast<uint8_t>(40 + (HAL::GetSystemConfig().volume * 40 + 127) / 255);
        items.push_back("Volume    " + std::to_string(show_percent));
        items.push_back("Back");

        auto selected = _data.select_menu->waitResult(items);

        // Sound 开关
        if (selected == 1)
        {
            if (HAL::GetSystemConfig().volume == 0)
                HAL::GetSystemConfig().volume = 127; // 恢复到默认中间值
            else
                HAL::GetSystemConfig().volume = 0;   // 静音

            uint8_t actual_percent = static_cast<uint8_t>(40 + (HAL::GetSystemConfig().volume * 40 + 127) / 255);
            HAL::SetAudioVolume(actual_percent);
            HAL::SetAudioMute(HAL::GetSystemConfig().volume == 0);

            _data.is_config_changed = true;
        }
        // Volume 调节（ProgressWindow）
        else if (selected == 2)
        {
            _handle_volume_config();
            _data.is_config_changed = true;
        }
        else
            break;
    }
}
