/**
 * @file app_recorder.cpp
 * @author Forairaaaaa
 * @brief
 * @version 0.1
 * @date 2023-11-04
 *
 * @copyright Copyright (c) 2023
 *
 */
#include "app_recorder.h"
#include "spdlog/spdlog.h"
#include "../../hal/hal.h"
#include "../assets/theme/theme.h"
#ifdef ESP_PLATFORM
#include <FS.h>
#include <SD.h>
#include "../../hal/hal_rachel/hal_rachel.h"
#endif
#include <ctime>

using namespace MOONCAKE::APPS;

// Helpers for elapsed time
namespace {
    static unsigned long computeShownMs(unsigned long elapsedMsAccum, unsigned long lastResumeMs, bool isPaused, unsigned long nowMs)
    {
        unsigned long shown = elapsedMsAccum;
        if (!isPaused) {
            if (nowMs >= lastResumeMs)
                shown += (nowMs - lastResumeMs);
        }
        return shown;
    }

    static void formatElapsed(char* out, size_t len, unsigned long shownMs)
    {
        unsigned long totalSeconds = shownMs / 1000UL;
        unsigned long mm = (totalSeconds / 60UL) % 1000UL;
        unsigned long ss = totalSeconds % 60UL;
        unsigned long ms = shownMs % 1000UL;
        snprintf(out, len, "%02lu:%02lu", mm, ss);
        //snprintf(out, len, "%02lu:%02lu.%03lu", mm, ss, ms);
    }
}

// Like setup()...
void AppRecorder::onResume()
{
    spdlog::info("{} onResume", getAppName());
    _data.isRecording = false;
    _data.startWasPressed = false;
    _data.selectWasPressed = false;
    _data.lastUiUpdateMs = 0;

    // Use pure English font for better performance
    HAL::GetCanvas()->setFont(&fonts::Font0);
    HAL::GetCanvas()->setTextSize(1);
    HAL::GetCanvas()->setTextColor(TFT_WHITE, TFT_BLACK);
}

// Like loop()...
void AppRecorder::onRunning()
{
    // Inputs
    bool startDown = HAL::GetButton(GAMEPAD::BTN_START);
    bool selectDown = HAL::GetButton(GAMEPAD::BTN_SELECT);
    bool rightDown = HAL::GetButton(GAMEPAD::BTN_RIGHT);

    if (startDown) {
        if (!_data.startWasPressed) {
            _data.startWasPressed = true;

            if (!HAL::CheckSdCard()) {
                HAL::PopFatalError("No SD card");
                return;
            }

            if (!_data.isRecording) {
                // Ensure directory and build path with timestamp YYMMDDHHMMSS
                auto time_now = HAL::GetLocalTime();
                char ts[32];
                strftime(ts, sizeof(ts), "%y%m%d%H%M%S", time_now);

                if (!SD.exists("/audio")) {
                    SD.mkdir("/audio");
                }

                snprintf(_data.lastSavedPath, sizeof(_data.lastSavedPath), "/audio/%s.wav", ts);

                bool okStart = static_cast<HAL_Rachel*>(HAL::Get())->startWavRecording(_data.lastSavedPath);
                if (okStart) {
                    _data.isRecording = true;
                    _data.isPaused = false;
                    _data.recStartMs = HAL::Millis();
                    _data.elapsedMsAccum = 0;
                    _data.lastResumeMs = _data.recStartMs;
                } else {
                    HAL::GetCanvas()->fillScreen(TFT_BLACK);
                    HAL::GetCanvas()->setCursor(10, 60);
                    HAL::GetCanvas()->print("Start record failed");
                    HAL::CanvasUpdate();
                }
            } else {
                // Toggle pause/resume within the same session
                _data.isPaused = !_data.isPaused;
                if (_data.isPaused) {
                    // accumulate current segment
                    unsigned long nowMs = HAL::Millis();
                    if (nowMs >= _data.lastResumeMs)
                        _data.elapsedMsAccum += (nowMs - _data.lastResumeMs);
                } else {
                    // resume new segment
                    _data.lastResumeMs = HAL::Millis();
                }
            }
        }
    } else {
        _data.startWasPressed = false;
    }

    // RIGHT ends and saves when recording session exists
    if (rightDown) {
        if (!_data.rightWasPressed) {
            _data.rightWasPressed = true;
            if (_data.isRecording) {
                bool okStop = static_cast<HAL_Rachel*>(HAL::Get())->stopWavRecording();
                _data.isRecording = false;
                _data.isPaused = false;
                _data.elapsedMsAccum = 0;

                //HAL::GetCanvas()->drawCenterString("START: Record  SELECT: Exit", 120, 160);
                HAL::GetCanvas()->pushImage(27, 159, 55, 55, image_data_set);
                HAL::GetCanvas()->pushImage(93, 159, 55, 55, image_data_record);
                HAL::GetCanvas()->pushImage(159, 159, 55, 55, image_data_finish);
                HAL::CanvasUpdate();
            }
        }
    } else {
        _data.rightWasPressed = false;
    }

    // SELECT exits only when not recording
    if (selectDown) {
        if (!_data.selectWasPressed && !_data.isRecording) {
            _data.selectWasPressed = true;
            destroyApp();
            return;
        }
    } else {
        _data.selectWasPressed = false;
    }

    // Recording step and UI
    if (_data.isRecording) {
        int16_t peak = 0;
        unsigned long tLoopStart = HAL::Millis();
        if (!_data.isPaused) {
            static_cast<HAL_Rachel*>(HAL::Get())->recordWavStep(8192, &peak);
            // 归一化到 0~120 (波形区高度)
            uint8_t h = (uint8_t)((long)peak * 120 / 32768);
            if (h > 120) h = 120;
            _data.waveform[_data.waveformIdx] = h;
            _data.waveformIdx = (_data.waveformIdx + 1) % WAVEFORM_W;
        }

        unsigned long tAfterRec = HAL::Millis();
        HAL::GetCanvas()->fillScreen(TFT_BLACK);

        // 上半屏：波形显示 (y: 0~120, 中线 y=60)
        int centerY = 60;
        for (int i = 0; i < WAVEFORM_W; i++) {
            // 从最旧到最新，向左滚动
            int bufIdx = (_data.waveformIdx + i) % WAVEFORM_W;
            int half = _data.waveform[bufIdx] / 2;
            if (half > 0) {
                HAL::GetCanvas()->drawLine(i, centerY - half, i, centerY + half, TFT_WHITE);
            }
        }
        // 中线
        HAL::GetCanvas()->drawLine(0, centerY, WAVEFORM_W - 1, centerY, 0x2104);

        // 下半屏：录制状态 + 时间 + 按钮
        unsigned long nowMs = HAL::Millis();
        unsigned long shownMs = computeShownMs(_data.elapsedMsAccum, _data.lastResumeMs, _data.isPaused, nowMs);
        char elapsedBuf[40];
        formatElapsed(elapsedBuf, sizeof(elapsedBuf), shownMs);

        // 红点 + 时间 + 状态
        HAL::GetCanvas()->setTextSize(2);
        HAL::GetCanvas()->fillSmoothCircle(31, 137, 4, THEME_COLOR_RED);
        HAL::GetCanvas()->drawString(elapsedBuf, 45, 130, &fonts::Font0);
        HAL::GetCanvas()->drawString(_data.isPaused ? "PAUSED" : "REC", 140, 130, &fonts::Font0);
        HAL::GetCanvas()->setTextSize(1);

        // 底部按钮图标
        HAL::GetCanvas()->pushImage(27, 159, 55, 55, image_data_set);
        if (_data.isPaused) {
            HAL::GetCanvas()->pushImage(93, 159, 55, 55, image_data_resume);
        } else {
            HAL::GetCanvas()->pushImage(93, 159, 55, 55, image_data_pause);
        }
        HAL::GetCanvas()->pushImage(159, 159, 55, 55, image_data_finish);
        HAL::CanvasUpdate();
        unsigned long tEnd = HAL::Millis();
        unsigned long recMs = tAfterRec - tLoopStart;
        unsigned long uiMs = tEnd - tAfterRec;
        unsigned long totalMs = tEnd - tLoopStart;
        if (totalMs > 40) {
            spdlog::warn("recLoop: rec={}ms ui={}ms total={}ms", recMs, uiMs, totalMs);
        }
        return;
    }
    
    // UI idle screen refresh (~10 fps)
    unsigned long now = HAL::Millis();
    if (!_data.isRecording && (now - _data.lastUiUpdateMs > 100)) {
        _data.lastUiUpdateMs = now;
        HAL::GetCanvas()->fillScreen(TFT_BLACK);
        //HAL::GetCanvas()->setTextSize(2);
        //HAL::GetCanvas()->drawCenterString("Recorder", 120, 42);
        HAL::GetCanvas()->setTextSize(2);
        HAL::GetCanvas()->drawCenterString("Recorder Ready", 120, 68);
        HAL::GetCanvas()->setTextSize(1);
        HAL::GetCanvas()->setCursor(12, 115);
        if (_data.lastSavedPath[0] != '\0') {
            HAL::GetCanvas()->print("Saved as:");
            HAL::GetCanvas()->print(_data.lastSavedPath);
        }
        HAL::GetCanvas()->pushImage(27, 159, 55, 55, image_data_set);
        HAL::GetCanvas()->pushImage(93, 159, 55, 55, image_data_record);
        HAL::GetCanvas()->pushImage(159, 159, 55, 55, image_data_finish);
        HAL::CanvasUpdate();
        //HAL::GetCanvas()->drawCenterString("START: Record  SELECT: Exit", 120, 160);
        //HAL::CanvasUpdate();
    }
}

void AppRecorder::onDestroy() { spdlog::info("{} onDestroy", getAppName()); }
