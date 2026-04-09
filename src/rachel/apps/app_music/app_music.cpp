/**
 * @file app_music.cpp
 * @author Forairaaaaa
 * @brief
 * @version 0.1
 * @date 2023-11-04
 *
 * @copyright Copyright (c) 2023
 *
 */
#include "app_music.h"
#include "spdlog/spdlog.h"
#include "../../hal/hal.h"
#include "../assets/theme/theme.h"
#include "../utils/system/ui/ui.h"
#ifdef ESP_PLATFORM
#include <Arduino.h>
#include <FS.h>
#include <SD.h>
#include "../../hal/hal_rachel/hal_rachel.h"
#endif

using namespace MOONCAKE::APPS;
using namespace SYSTEM::UI;

void AppMusic::onCreate() { spdlog::info("{} onCreate", getAppName()); }

void AppMusic::onResume()
{
    spdlog::info("{} onResume", getAppName());
    _data.wavList.clear();
    _data.currentIndex = 0;
    _data.startWasPressed = false;
    _data.rightWasPressed = false;
    _data.selectWasPressed = false;
    _data.lastUiUpdateMs = 0;
    HAL::GetCanvas()->setFont(&fonts::Font0);
    HAL::GetCanvas()->setTextSize(1);

#ifdef ESP_PLATFORM
    if (!HAL::CheckSdCard())
        HAL::PopFatalError("No SD card");

    if (!SD.exists("/audio"))
        HAL::PopFatalError("/audio not found");

    File dir = SD.open("/audio");
    while (1)
    {
        File entry = dir.openNextFile();
        if (!entry) break;
        if (!entry.isDirectory()) {
            const char* name = entry.name();
            if (std::strstr(name, ".wav") != NULL || std::strstr(name, ".WAV") != NULL) {
                _data.wavList.emplace_back(name);
            }
        }
        entry.close();
    }
    if (_data.wavList.empty())
        HAL::PopFatalError("No wav files");
#endif
}

void AppMusic::onRunning()
{
    // Inputs
    bool startDown = HAL::GetButton(GAMEPAD::BTN_START);
    bool rightDown = HAL::GetButton(GAMEPAD::BTN_RIGHT);
    bool selectDown = HAL::GetButton(GAMEPAD::BTN_BACK);

    // START: play current (restart if already playing)
    if (startDown) {
        if (!_data.startWasPressed) {
            _data.startWasPressed = true;
#ifdef ESP_PLATFORM
            if (_data.currentIndex < 0 || _data.currentIndex >= (int)_data.wavList.size())
                _data.currentIndex = 0;
            std::string path = "/audio/";
            path += _data.wavList[_data.currentIndex];
            HAL::StopAudioPlayback();
            HAL::PlayWavFile(path.c_str());
#endif
        }
    } else {
        _data.startWasPressed = false;
    }

    // RIGHT: next file
    if (rightDown) {
        if (!_data.rightWasPressed) {
            _data.rightWasPressed = true;
#ifdef ESP_PLATFORM
            _data.currentIndex = (_data.currentIndex + 1) % (int)_data.wavList.size();
            std::string path = "/audio/";
            path += _data.wavList[_data.currentIndex];
            HAL::StopAudioPlayback();
            HAL::PlayWavFile(path.c_str());
#endif
        }
    } else {
        _data.rightWasPressed = false;
    }

    // SELECT: stop and exit
    if (selectDown) {
        if (!_data.selectWasPressed) {
            _data.selectWasPressed = true;
#ifdef ESP_PLATFORM
            HAL::StopAudioPlayback();
#endif
            destroyApp();
            return;
        }
    } else {
        _data.selectWasPressed = false;
    }

    // UI (simple file name and hint)
    unsigned long now = HAL::Millis();
    if (now - _data.lastUiUpdateMs > 100) {
        _data.lastUiUpdateMs = now;
        HAL::GetCanvas()->fillScreen(TFT_BLACK);
        HAL::GetCanvas()->setTextColor(TFT_WHITE, TFT_BLACK);
        //HAL::GetCanvas()->setCursor(100, 30);
        //HAL::GetCanvas()->print("Audio Player");
        HAL::GetCanvas()->setCursor(30, 60);
        if (!_data.wavList.empty()) {
            HAL::GetCanvas()->drawCenterString("Playing: ", 120, 45);
            HAL::GetCanvas()->setTextSize(2);
            HAL::GetCanvas()->drawCenterString(_data.wavList[_data.currentIndex].c_str(), 120, 65);
            HAL::GetCanvas()->setTextSize(1);
        } else {
            HAL::GetCanvas()->print("No wav files");
        }

        HAL::GetCanvas()->drawCenterString("Exit      Play      Next", 120, 106);
        HAL::CanvasUpdate();
    }
}

void AppMusic::onDestroy() { spdlog::info("{} onDestroy", getAppName()); }
