/**
 * @file app_timeview.h
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
#include "assets/icon_app_timeview.h"
#include "../utils/smooth_menu/lv_anim/lv_anim.h"

namespace MOONCAKE::APPS
{
    /**
     * @brief Timeview
     *
     */
    class AppTimeview : public APP_BASE
    {
    private:
        struct Data_t
        {
            unsigned long count = 0;

            // 弹窗/状态栏相关
            std::string popupText;
            bool popupVisible = false;
            bool popupAnimating = false;   // true 时进行进/出场动画
            uint32_t popupShowTime = 0;    // 弹窗开始展示或最近交互时间
            uint32_t popupDisplayDuration = 6000; // 6s 无交互自动隐藏

            // 弹窗动画：Y 方向滑入/滑出
            LVGL::Anim_Path popupSlideAnim;
            // 弹窗宽度动画（根据文本宽度自适应）
            LVGL::Anim_Path popupWidthAnim;
            int popupCurrentWidth = 0;     // 当前渲染宽度
            int popupTargetWidth = 0;      // 目标宽度
            
            // 弹窗状态控制
            bool popupUseYAnimation = false;        // 本次显示是否使用Y轴动画
            bool popupOpenedByLongPress = false;    // 是否由长按打开（松开时保持4s）

            // 长按检测
            bool startWasPressed = false;
            uint32_t startPressStartMs = 0;
            bool rightWasPressed = false;
            uint32_t rightPressStartMs = 0;

            // 模式与选择
            enum Mode_t { MODE_CLOCK = 0, MODE_STOPWATCH, MODE_TIMER };
            Mode_t currentMode = MODE_CLOCK; // 确认后的模式
            int popupSelectIndex = 0;        // 弹窗中选择的索引

            // CLOCK 子模式：显示格式
            enum ClockFormat_t { CLOCK_FMT_MON_DAY = 0, CLOCK_FMT_WDAY_DAY };
            ClockFormat_t clockFormat = CLOCK_FMT_WDAY_DAY;

            // STOPWATCH（正计时）
            bool swRunning = false;
            uint32_t swStartMs = 0;         // 最近一次开始时间
            uint32_t swAccumulatedMs = 0;   // 暂停累计时间

                    // TIMER（倒计时）
        bool timerRunning = false;
        uint32_t timerTotalMs = 0;      // 目标总时长
        uint32_t timerRemainMs = 0;     // 剩余时间
        bool timerWasRunning = false;    // 用于检测完成沿
        uint32_t timerLastTickMs = 0;    // 计时器内部tick
        
        // 缓存的时间格式化结果，避免重复计算
        struct TimeCache_t {
            uint32_t lastMs = 0xFFFFFFFF;  // 上次计算的毫秒值
            uint32_t hh = 0, mm = 0, ss = 0, centisec = 0;
            char timeStr[16] = {0};         // 格式化字符串缓存
            bool needsUpdate = true;
        };
        TimeCache_t timerCache;
        TimeCache_t stopwatchCache;
        };
        Data_t _data;

    public:
        void onCreate() override;
        void onResume() override;
        void onRunning() override;
        void onDestroy() override;

    private:
        // 弹窗相关
        void _showPopup(const char* text, bool useYAnimation = false);
        void _hidePopup();
        void _renderPopup();
        void _updatePopupWidthTarget(const char* text);
        void _updatePopupText(const char* text);  // 仅更新文本和宽度，不重置动画状态

        // 业务逻辑
        void _handleInputs();
        void _renderMain();
        void _renderClock();
        void _renderStopwatch();
        void _renderTimer();
        void _updateStopwatch();
        void _updateTimer();
        void _enterTimerSettings();
        
        // 优化的时间计算函数
        void _updateTimeCache(Data_t::TimeCache_t& cache, uint32_t ms);
        const char* _getFormattedTime(Data_t::TimeCache_t& cache, uint32_t ms, bool showCentisec = false);
    };

    class AppTimeview_Packer : public APP_PACKER_BASE
    {
        std::string getAppName() override { return "Timeview"; }
        void* getAppIcon() override { return (void*)image_data_icon_app_timeview; }
        void* newApp() override { return new AppTimeview; }
        void deleteApp(void* app) override { delete (AppTimeview*)app; }
    };
} // namespace MOONCAKE::APPS