/**
 * @file hal.h
 * @author Forairaaaaa
 * @brief
 * @version 0.1
 * @date 2023-11-04
 *
 * @copyright Copyright (c) 2023
 *
 */
#pragma once
#ifndef ESP_PLATFORM
#define LGFX_USE_V1
#include <LGFX_AUTODETECT.hpp>
#include <LovyanGFX.hpp>
#include "lgfx/v1/LGFXBase.hpp"
#include "lgfx/v1/LGFX_Sprite.hpp"
#include "lgfx/v1_autodetect/LGFX_AutoDetect_sdl.hpp"
#include <cstdint>
#include <iostream>
#include <string>
#include "lgfx_fx/lgfx_fx.h"
#else
#include <LovyanGFX.hpp>
#include "lgfx_fx/lgfx_fx.h"
#endif

namespace GAMEPAD
{
    /**
     * @brief Button enum
     *
     */
    enum GamePadButton_t
    {
        BTN_START = 0,
        BTN_SELECT,         // 编码器逆时针(左) 虚拟按键
        BTN_RIGHT,          // 编码器顺时针(右) 虚拟按键
        BTN_JOYSTICK,       // 摇杆按键 (GPIO7)
        BTN_BACK,           // BOOT 键, 返回/退出 (GPIO0)
        GAMEPAD_BUTTON_NUM,
    };
} // namespace GAMEPAD

namespace CONFIG
{
    /**
     * @brief System config
     *
     */
    struct SystemConfig_t
    {
        uint8_t brightness = 127;
        uint8_t volume = 127;
        // MIDI UI 模式：0=Normal（屏幕中央显示和弦名）, 1=BOX（九宫格高亮）
        uint8_t midi_ui_mode = 0;
        // 音频输出模式：0=Local（板载混音器播放 WAV）, 1=USB（仅 USB MIDI 输出）
        uint8_t audio_mode = 0;
        // 自动休眠时长：0=10min, 1=30min, 2=60min, 3=Never
        uint8_t auto_sleep_timeout = 0;
    };
} // namespace CONFIG

namespace IMU
{
    /**
     * @brief IMU data
     *
     */
    struct ImuData_t
    {
        float accelX;
        float accelY;
        float accelZ;
    };
} // namespace IMU

/**
 * @brief Singleton like pattern to simplify hal's getter
 * 1) Inherit and override methods to create a specific hal
 * 2) Use HAL::Inject() to inject your hal
 * 3) Use HAL:Get() to get this hal wherever you want
 */
class HAL
{
private:
    static HAL* _hal;

public:
    /**
     * @brief Get HAL pointer, 获取HAL实例
     *
     * @return HAL*
     */
    static HAL* Get();

    /**
     * @brief Check if HAL is valid, 检测HAL是否有效
     *
     * @return true
     * @return false
     */
    static bool Check();

    /**
     * @brief HAL injection, init() will be called here, HAL注入
     *
     * @param hal
     * @return true
     * @return false
     */
    static bool Inject(HAL* hal);

    /**
     * @brief Destroy HAL and free memory, 销毁释放HAL
     *
     */
    static void Destroy();

    /**
     * @brief Base class
     *
     */
public:
    HAL() : _display(nullptr), _canvas(nullptr), _is_sd_card_ready(false) {}
    virtual ~HAL() {}
    virtual std::string type() { return "Base"; }
    virtual void init() {}

    /**
     * @brief Components
     *
     */
protected:
    LGFX_Device* _display;
    LGFX_SpriteFx* _canvas;
    time_t _time_buffer;
    IMU::ImuData_t _imu_data;
    bool _is_sd_card_ready;
    CONFIG::SystemConfig_t _config;

    /**
     * @brief Getters
     *
     */
public:
    /**
     * @brief Display device, 获取屏幕驱动实例
     *
     * @return LGFX_Device*
     */
    static LGFX_Device* GetDisplay() { return Get()->_display; }

    /**
     * @brief Full screen canvas (sprite), 获取全屏Buffer实例
     *
     * @return LGFX_SpriteFx*
     */
    static LGFX_SpriteFx* GetCanvas() { return Get()->_canvas; }

public:
    ///
    /// System APIs
    ///

    /**
     * @brief Delay(ms), 延时(毫秒)
     *
     * @param milliseconds
     */
    static void Delay(unsigned long milliseconds) { Get()->delay(milliseconds); }
    virtual void delay(unsigned long milliseconds) { lgfx::delay(milliseconds); }

    /**
     * @brief Get the number of milliseconds passed since boot, 获取系统运行毫秒数
     *
     * @return unsigned long
     */
    static unsigned long Millis() { return Get()->millis(); }
    virtual unsigned long millis() { return lgfx::millis(); }

    /**
     * @brief Power off, 关机
     *
     */
    static void PowerOff() { Get()->powerOff(); }
    virtual void powerOff() {}

    /**
     * @brief Reboot, 重启
     *
     */
    static void Reboot() { Get()->reboot(); }
    virtual void reboot() {}

    /**
     * @brief Enter light sleep and wakeup by button, 进入轻睡眠并由按键唤醒
     *
     */
    static void LightSleepWithButtonWakeup() { Get()->lightSleepWithButtonWakeup(); }
    virtual void lightSleepWithButtonWakeup() {}

    /**
     * @brief Set RTC time, 设置RTC时间
     *
     * @param dateTime
     */
    static void SetSystemTime(tm dateTime) { return Get()->setSystemTime(dateTime); }
    virtual void setSystemTime(tm dateTime) {}

    /**
     * @brief Get local time(wrap of localtime()), 获取当前时间
     *
     * @return tm*
     */
    static tm* GetLocalTime() { return Get()->getLocalTime(); }
    virtual tm* getLocalTime();

    /**
     * @brief Update IMU data, 刷新IMU数据
     *
     */
    static void UpdateImuData() { Get()->updateImuData(); }
    virtual void updateImuData() {}

    /**
     * @brief Get the Imu Data, 获取IMU数据
     *
     * @return IMU::ImuData_t&
     */
    static IMU::ImuData_t& GetImuData() { return Get()->getImuData(); }
    IMU::ImuData_t& getImuData() { return _imu_data; }

    /**
     * @brief Buzzer beep, 蜂鸣器开始哔哔
     *
     * @param frequency
     * @param duration
     */
    static void Beep(float frequency, uint32_t duration = 4294967295U) { Get()->beep(frequency, duration); }
    virtual void beep(float frequency, uint32_t duration) {}

    /**
     * @brief Stop buzzer beep, 蜂鸣器别叫了
     *
     */
    static void BeepStop() { Get()->beepStop(); }
    virtual void beepStop() {}

    /**
     * @brief Set the Beep Volume
     *
     * @param volume
     */
    static void SetBeepVolume(uint8_t volume) { Get()->setBeepVolume(volume); }
    virtual void setBeepVolume(uint8_t volume) {}

    /**
     * @brief Check if sd card is valid, 检查SD卡是否可用
     *
     * @return true
     * @return false
     */
    static bool CheckSdCard() { return Get()->checkSdCard(); }
    virtual bool checkSdCard() { return _is_sd_card_ready; }

    ///
    /// Display APIs
    ///

    /**
     * @brief Push framebuffer, 推送framebuffer到显示屏
     *
     */
    static void CanvasUpdate() { Get()->canvasUpdate(); }
    virtual void canvasUpdate() { GetCanvas()->pushSprite(0, 0); }

    /**
     * @brief Render fps panel, 渲染FPS面板
     *
     */
    static void RenderFpsPanel() { Get()->renderFpsPanel(); }
    virtual void renderFpsPanel();

    /**
     * @brief Pop error message and wait reboot, 优雅地抛个蓝屏
     *
     * @param msg
     */
    static void PopFatalError(std::string msg) { Get()->popFatalError(msg); }
    virtual void popFatalError(std::string msg);

    ///
    /// File system APIs
    ///

    /**
     * @brief Load 24px height text font from SD card(slower render), 从SD卡导入24px高文本字体(渲染很慢)
     * Path: /fonts/font_text_24.vlw
     * 路径: /fonts/font_text_24.vlw
     */
    static void LoadTextFont24() { Get()->loadTextFont24(); }
    virtual void loadTextFont24() {}

    /**
     * @brief Load 16px height text font from SD card(slower render), 从SD卡导入16px高文本字体(渲染很慢)
     * Path: /fonts/font_text_16.vlw
     * 路径: /fonts/font_text_16.vlw
     */
    static void LoadTextFont16() { Get()->loadTextFont16(); }
    virtual void loadTextFont16() {}

    /**
     * @brief Load 24px height launcher font from flash(fast render), 从flash导入24px高启动器用字体(渲染很快)
     *
     */
    static void LoadLauncherFont24() { Get()->loadLauncherFont24(); }
    virtual void loadLauncherFont24() {}

    ///
    /// Gamepad APIs
    ///

    /**
     * @brief Get button state, 获取按键状态
     *
     * @param button
     * @return true Pressing, 按下
     * @return false Released, 松开
     */
    static bool GetButton(GAMEPAD::GamePadButton_t button) { return Get()->getButton(button); }
    virtual bool getButton(GAMEPAD::GamePadButton_t button) { return false; }

    /**
     * @brief Get any button state, 获取任意按键状态
     *
     * @return true Pressing, 按下
     * @return false Released, 松开
     */
    static bool GetAnyButton() { return Get()->getAnyButton(); }
    virtual bool getAnyButton();

    ///
    /// Joystick APIs (模拟摇杆)
    ///

    /**
     * @brief Get joystick X axis value, 获取摇杆X轴值
     *
     * @return int 原始ADC值 (0~4095)
     */
    static int GetJoystickX() { return Get()->getJoystickX(); }
    virtual int getJoystickX() { return 2048; }

    /**
     * @brief Get joystick Y axis value, 获取摇杆Y轴值
     *
     * @return int 原始ADC值 (0~4095)
     */
    static int GetJoystickY() { return Get()->getJoystickY(); }
    virtual int getJoystickY() { return 2048; }

    ///
    /// Encoder APIs (旋转编码器/滚轮)
    ///

    /**
     * @brief Get encoder delta since last call, 获取编码器增量
     * 正值=顺时针, 负值=逆时针, 0=无变化
     *
     * @return int 增量值
     */
    static int GetEncoderDelta() { return Get()->getEncoderDelta(); }
    virtual int getEncoderDelta() { return 0; }

    ///
    /// System config APIs
    ///

    /**
     * @brief Load system config from FS, 从内部FS导入系统配置
     *
     */
    static void LoadSystemConfig() { Get()->loadSystemConfig(); }
    virtual void loadSystemConfig() {}

    /**
     * @brief Save system config to FS, 保存系统配置到内部FS
     *
     */
    static void SaveSystemConfig() { Get()->saveSystemConfig(); }
    virtual void saveSystemConfig() {}

    /**
     * @brief Get the System Config, 获取系统配置
     *
     * @return CONFIG::SystemConfig_t&
     */
    static CONFIG::SystemConfig_t& GetSystemConfig() { return Get()->_config; }

    /**
     * @brief Set the System Config, 设置系统配置
     *
     * @param cfg
     */
    static void SetSystemConfig(CONFIG::SystemConfig_t cfg) { Get()->_config = cfg; }

    /**
     * @brief Update device to the system config, 以系统配置刷新设备
     *
     */
    static void UpdateSystemFromConfig() { Get()->updateSystemFromConfig(); }
    virtual void updateSystemFromConfig();

    ///
    /// Audio APIs
    ///

    /**
     * @brief 播放WAV音频文件（异步）
     * 
     * @param filename SD卡上的WAV文件路径
     * @return true 播放命令发送成功
     * @return false 播放失败
     */
    static bool PlayWavFile(const char* filename) { return Get()->playWavFile(filename); }
    virtual bool playWavFile(const char* filename) { return false; }

    /**
     * @brief 检查是否正在播放音频
     * 
     * @return true 正在播放
     * @return false 未播放
     */
    static bool IsAudioPlaying() { return Get()->isAudioPlaying(); }
    virtual bool isAudioPlaying() { return false; }

    /**
     * @brief 停止当前音频播放
     */
    static void StopAudioPlayback() { Get()->stopAudioPlayback(); }
    virtual void stopAudioPlayback() {}

    /**
     * @brief 设置音频音量
     * 
     * @param volume 音量（0-100）
     */
    static void SetAudioVolume(uint8_t volume) { Get()->setAudioVolume(volume); }
    virtual void setAudioVolume(uint8_t volume) {}

    /**
     * @brief 获取音频音量
     * 
     * @return uint8_t 当前音量（0-100）
     */
    static uint8_t GetAudioVolume() { return Get()->getAudioVolume(); }
    virtual uint8_t getAudioVolume() { return 70; }

    /**
     * @brief 设置音频静音状态
     * 
     * @param mute true为静音，false为取消静音
     */
    static void SetAudioMute(bool mute) { Get()->setAudioMute(mute); }
    virtual void setAudioMute(bool mute) {}

    ///
    /// LED APIs (WS2812)
    ///

    /**
     * @brief 设置单颗 LED 颜色
     * @param index LED 索引 (0~10)
     * @param r, g, b 颜色分量 (0~255)
     */
    static void SetLedColor(uint8_t index, uint8_t r, uint8_t g, uint8_t b) { Get()->setLedColor(index, r, g, b); }
    virtual void setLedColor(uint8_t index, uint8_t r, uint8_t g, uint8_t b) {}

    /**
     * @brief 设置所有 LED 为同一颜色
     */
    static void SetAllLedColor(uint8_t r, uint8_t g, uint8_t b) { Get()->setAllLedColor(r, g, b); }
    virtual void setAllLedColor(uint8_t r, uint8_t g, uint8_t b) {}

    /**
     * @brief 关闭所有 LED
     */
    static void SetLedOff() { Get()->setLedOff(); }
    virtual void setLedOff() {}

    /**
     * @brief 推送 LED 数据（调用 setLedColor 后需要 show）
     */
    static void LedShow() { Get()->ledShow(); }
    virtual void ledShow() {}

    /**
     * @brief 设置 LED 全局亮度 (0~255)
     */
    static void SetLedBrightness(uint8_t brightness) { Get()->setLedBrightness(brightness); }
    virtual void setLedBrightness(uint8_t brightness) {}

    ///
    /// Touch APIs (MPR121)
    ///

    /**
     * @brief 获取 12 通道触摸状态位掩码
     * bit0=ELE0, bit1=ELE1, ... bit11=ELE11, 1=touched
     *
     * @return uint16_t 触摸状态
     */
    static uint16_t GetTouchStatus() { return Get()->getTouchStatus(); }
    virtual uint16_t getTouchStatus() { return 0; }

    /**
     * @brief 检查指定通道是否被触摸
     * @param channel 通道号 (0~11)
     */
    static bool IsTouched(uint8_t channel) { return Get()->isTouched(channel); }
    virtual bool isTouched(uint8_t channel) { return false; }

    /**
     * @brief 获取指定通道的原始滤波值（可用于压力感应）
     * @param channel 通道号 (0~11)
     * @return uint16_t 滤波后的 ADC 值
     */
    static uint16_t GetTouchFiltered(uint8_t channel) { return Get()->getTouchFiltered(channel); }
    virtual uint16_t getTouchFiltered(uint8_t channel) { return 0; }

    /**
     * @brief 获取指定通道的基线值
     * @param channel 通道号 (0~11)
     * @return uint16_t 基线值
     */
    static uint16_t GetTouchBaseline(uint8_t channel) { return Get()->getTouchBaseline(channel); }
    virtual uint16_t getTouchBaseline(uint8_t channel) { return 0; }

    /**
     * @brief 检查 MPR121 是否就绪
     */
    static bool CheckTouch() { return Get()->checkTouch(); }
    virtual bool checkTouch() { return false; }
};
