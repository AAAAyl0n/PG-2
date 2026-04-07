/**
 * @file app_bangboo.h
 * @author Bowen
 * @brief
 * @version 0.1
 * @date 2023-11-04
 *
 * @copyright Copyright (c) 2023
 *
 */
#include <mooncake.h>
#include "assets/icon_app_bangboo.hpp"
#include "../assets/icons/icons.h"
#include "../utils/smooth_menu/lv_anim/lv_anim.h"
#include <string>

namespace MOONCAKE::APPS
{
    /**
     * @brief Bangboo
     *
     */
    class AppBangboo : public APP_BASE
    {
    public:
        enum ExpressionType_t {
            EXPR_EYES,     // 正常眼睛
            EXPR_BLINK,    // 眨眼
            EXPR_SMILE,    // 微笑
            EXPR_ANGER,    // 愤怒
            EXPR_SAD,      // 悲伤
            EXPR_WINCE,    // 眯眼
            EXPR_SLEEP,   // 睡眠
        };
        
    private:
        enum BangbooState_t {
            STATE_PREIDLE,     // 空闲状态前
            STATE_IDLE,        // 空闲状态  

            STATE_SLEEPING,    // 睡眠状态
            STATE_POSTSLEEP,   // 睡眠状态后
            STATE_SLEEPEND,   // 睡眠状态结束

            STATE_PREHAPPY,       // 开心状态
            STATE_HAPPY,       // 开心状态

            STATE_BLINKING0,   // 眨眼状态0
            STATE_BLINKTEMP,    // 眨眼状态临时
            STATE_BLINKING1,   // 眨眼状态1

            STATE_PREANGER,    // 愤怒状态前
            STATE_ANGER,       // 愤怒状态

            STATE_PREWINCE1,
            STATE_PREWINCE2,    // 晕眩状态前
            STATE_WINCE,       // 晕眩状态
            STATE_POSTWINCE,  // 晕眩状态后

            STATE_PRESAD,      // 悲伤状态前
            STATE_SAD,         // 悲伤状态
            STATE_POSTSAD,     // 悲伤状态后


            STATE_CURIOUS,     // 好奇
            STATE_DULL,         // 呆滞

            STATE_SHAKED,      // 摇晃后
            
        };

        enum ShakeState_t {
            SHAKE_IDLE,        // 空闲状态，显示"- -"
            SHAKE_DETECTING,   // 检测摇晃中
            SHAKE_TRIGGERED    // 摇晃触发，显示"> <"
        };
    
        struct Data_t
        {
            unsigned long count = 0;

            // 表情相关
            ExpressionType_t currentExpression = EXPR_SLEEP;
            
            // 状态栏相关
            std::string clock;                     // 显示的时间字符串
            uint32_t clock_update_count = 0;       // 时钟更新计数器
            const uint32_t clock_update_interval = 1000;  // 1秒更新间隔
            char string_buffer[10];                // 时间格式化缓冲区
            
            // 状态栏动画和控制
            LVGL::Anim_Path statusBarAnim;         // 状态栏动画对象
            bool statusBarVisible = false;         // 状态栏是否可见
            uint32_t statusBarShowTime = 0;        // 状态栏显示时间
            uint32_t statusBarDisplayDuration = 3000;  // 状态栏默认显示持续时间
            bool statusBarAnimating = false;       // 是否正在动画中
            bool buttonPressed = false;            // 按钮状态追踪

            // 状态栏消息
            std::string statusBarMessage;          // 要显示的消息文本（优先于时间）
            uint32_t statusBarMessageExpire = 0;   // 消息过期时间（毫秒时间戳）；0 表示无消息
            
            // 状态机相关
            BangbooState_t currentState = STATE_PREIDLE;  // 当前状态
            uint32_t stateStartTime = 0;                   // 状态开始时间
            uint32_t stateTimer = 0;                       // 状态持续时间
            int sleepZLastIndex = -1;                      // 睡眠状态下“z/zz/zzz”循环上次索引

            // imu相关
            ShakeState_t shakeState = SHAKE_IDLE;    // 摇晃状态
            unsigned long lastTime = 0;               // 上一次检测时间
            unsigned long shakeStartTime = 0;         // 摇晃开始时间
            unsigned long triggerStartTime = 0;       // 触发开始时间
            unsigned long shakeCooldownUntil = 0;     // 摇晃冷却结束时间
            float lastAccelX = 0.0f;                  // 上一次加速度X
            float lastAccelY = 0.0f;                  // 上一次加速度Y
            float lastAccelZ = 0.0f;                  // 上一次加速度Z
            float shakeIntensity = 0.0f;             // 摇晃强度
            static constexpr float SHAKE_THRESHOLD = 2.8f;  // 摇晃阈值 (m/s²)
            static constexpr unsigned long SHAKE_DETECT_TIME = 500;  // 检测时间 0.5s
            static constexpr float SHAKE_DECAY = 0.80f;     // 摇晃强度衰减系数
            static constexpr unsigned long SHAKE_COOLDOWN_TIME = 1200;  // 触发后冷却时间

            // 自动休眠
            unsigned long lastActivityTime = 0; // 最后一次用户活动时间

        };
        Data_t _data;

    public:
        void onCreate() override;
        void onResume() override;
        void onRunning() override;
        void onDestroy() override;
        
        // 表情绘制相关
        void drawExpression(ExpressionType_t expression);
        
        // 状态栏相关方法
        void _update_clock(bool updateNow = false);
        void _render_status_bar();
        void _show_status_bar();
        void _hide_status_bar();
        void CheckStatusBar();
        void showStatusMessage(const char* msg, uint32_t durationMs = 0);

        // 状态机相关方法
        void updateStateMachine();
        void changeState(BangbooState_t newState);
        ExpressionType_t getExpressionForState(BangbooState_t state);

        // imu相关方法
        void imu_detection();

        void render();
    };

    class AppBangboo_Packer : public APP_PACKER_BASE
    {
        std::string getAppName() override { return "Bangboo"; }
        void* getAppIcon() override { return (void*)image_data_icon_app_bangboo; }
        void* newApp() override { return new AppBangboo; }
        void deleteApp(void* app) override { delete (AppBangboo*)app; }
    };
} // namespace MOONCAKE::APPS
