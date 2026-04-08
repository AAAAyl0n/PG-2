/**
 * @file app_recorder.h
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
#include "assets/finish.hpp"
#include "assets/pause.hpp"
#include "assets/record.hpp"
#include "assets/resume.hpp"
#include "assets/set.hpp"
#include "assets/icon_app_recorder.hpp"
namespace MOONCAKE::APPS
{
    /**
     * @brief Recorder
     *
     */
    class AppRecorder : public APP_BASE
    {
    private:
        static const int WAVEFORM_W = 240;  // 波形宽度 = 屏幕宽度
        struct Data_t
        {
            unsigned long count = 0;
            bool isRecording = false;
            bool startWasPressed = false;
            bool selectWasPressed = false;
            unsigned long lastUiUpdateMs = 0;
            int durationSeconds = 5;
            char lastSavedPath[96] = {0};
            unsigned long recStartMs = 0;
            bool isPaused = false;
            bool rightWasPressed = false;
            unsigned long elapsedMsAccum = 0;
            unsigned long lastResumeMs = 0;
            // 波形环形缓冲区
            uint8_t waveform[240] = {0};  // 归一化到 0~120 的振幅
            int waveformIdx = 0;          // 写入位置
            int16_t peakHold = 0;         // 帧间最大 peak
        };
        Data_t _data;

    public:
        void onCreate() override {}
        void onResume() override;
        void onRunning() override;
        void onDestroy() override;
    };

    class AppRecorder_Packer : public APP_PACKER_BASE
    {
        std::string getAppName() override { return "Recorder"; }
        void* getAppIcon() override { return (void*)image_data_icon_app_recorder;  }
        void* newApp() override { return new AppRecorder; }
        void deleteApp(void* app) override { delete (AppRecorder*)app; }
    };
} // namespace MOONCAKE::APPS