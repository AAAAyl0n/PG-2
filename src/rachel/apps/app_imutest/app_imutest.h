/**
 * @file app_imutest.h
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

namespace MOONCAKE::APPS
{
    /**
     * @brief Imutest
     *
     */
    class AppImutest : public APP_BASE
    {
    private:
        enum ShakeState_t {
            SHAKE_IDLE,        // 空闲状态，显示"- -"
            SHAKE_DETECTING,   // 检测摇晃中
            SHAKE_TRIGGERED    // 摇晃触发，显示"> <"
        };
        
        struct Data_t
        {
            unsigned long count = 0;
            
            // 摇晃检测相关
            ShakeState_t shakeState = SHAKE_IDLE;
            unsigned long lastTime = 0;
            unsigned long shakeStartTime = 0;
            unsigned long triggerStartTime = 0;
            
            // 加速度相关
            float lastAccelX = 0.0f;
            float lastAccelY = 0.0f;
            float lastAccelZ = 0.0f;
            float shakeIntensity = 0.0f;
            
            // 眨眼相关
            unsigned long lastBlinkTime = 0;
            unsigned long blinkStartTime = 0;
            unsigned long nextBlinkDelay = 6000;  // 初始眨眼间隔6秒
            bool isBlinking = false;
            
            // 阈值和时间常数
            static constexpr float SHAKE_THRESHOLD = 2.7f;  // 摇晃阈值 (m/s²)
            static constexpr unsigned long SHAKE_DETECT_TIME = 500;  // 检测时间 0.5s
            static constexpr unsigned long SHAKE_DISPLAY_TIME = 2100; // 显示时间 1.6s
            static constexpr float SHAKE_DECAY = 0.87f;     // 摇晃强度衰减系数
            static constexpr unsigned long BLINK_DURATION = 100;  // 眨眼持续时间100ms
            static constexpr unsigned long BLINK_MIN_INTERVAL = 6000;  // 最小眨眼间隔6s
            static constexpr unsigned long BLINK_MAX_INTERVAL = 8000;  // 最大眨眼间隔8s
        };
        Data_t _data;

    public:
        void onCreate() override;
        void onResume() override;
        void onRunning() override;
        void onDestroy() override;
    };

    class AppImutest_Packer : public APP_PACKER_BASE
    {
        std::string getAppName() override { return "Imutest"; }
        void* getAppIcon() override { return (void*)image_data_icon_app_default; }
        void* newApp() override { return new AppImutest; }
        void deleteApp(void* app) override { delete (AppImutest*)app; }
    };
} // namespace MOONCAKE::APPS