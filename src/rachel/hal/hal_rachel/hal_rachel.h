/**
 * @file hal_rachel.h
 * @author Forairaaaaa
 * @brief
 * @version 0.1
 * @date 2023-11-07
 *
 * @copyright Copyright (c) 2023
 *
 */
#pragma once
#include "../hal.h"
#include "utils/m5unified/I2C_Class.hpp"
#include "utils/m5unified/RTC8563_Class.hpp"
#include "utils/m5unified/IMU_Class.hpp"
#include "utils/m5unified/audio/M5EchoBase.h"
#include <Adafruit_NeoPixel.h>

// 添加FreeRTOS相关头文件
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>

// 前向声明
class FS;

class HAL_Rachel : public HAL
{
private:
    std::array<uint8_t, 11> _gamepad_key_map;
    std::array<bool, 11> _key_state_list;
    m5::I2C_Class* _i2c_bus;
    m5::RTC8563_Class* _rtc;
    m5::IMU_Class* _imu;
    
    // 音频相关成员变量
    M5EchoBase* _echobase;
    TaskHandle_t _audio_task_handle;
    QueueHandle_t _audio_queue;
    volatile bool _is_audio_playing;
    volatile bool _should_stop_audio;
    uint8_t _audio_volume;
    bool _audio_muted;
    int _audio_sample_rate;

    // WS2812 LED
    Adafruit_NeoPixel* _leds;

    // MPR121 触摸
    bool _touch_ready;

    // 音频播放命令结构
    struct AudioCommand_t {
        char filename[256];
        FS* fs_ptr;
    };
    
    // WAV文件头结构
    struct __attribute__((packed)) WavHeader_t {
        char RIFF[4];
        uint32_t chunk_size;
        char WAVEfmt[8];
        uint32_t fmt_chunk_size;
        uint16_t audiofmt;
        uint16_t channel;
        uint32_t sample_rate;
        uint32_t byte_per_sec;
        uint16_t block_size;
        uint16_t bit_per_sample;
    };
    
    struct __attribute__((packed)) SubChunk_t {
        char identifier[4];
        uint32_t chunk_size;
        uint8_t data[1];
    };

private:
    void _power_init();
    void _disp_init();
    void _disp_logo();
    void _fs_init();
    void _i2c_init();
    void _rtc_init();
    void _imu_init();
    void _spk_init();
    void _sdcard_init();
    void _gamepad_init();
    void _audio_init();
    void _ws2812_init();
    void _touch_init();
    void _adjust_sys_time();
    void _calibrateRTCWithCompileTime();
    void _system_config_init();
    void _sum_up();
    
    // 编码器状态 (中断驱动，ISR 内判方向)
    volatile bool _encoder_left_flag = false;
    volatile bool _encoder_right_flag = false;
    volatile uint8_t _encoder_a_last = HIGH;
    static void IRAM_ATTR _encoderISR();
    void _pollEncoder();

    // 日志显示控制（单行刷新模式）
    bool _log_single_line_mode = false;
    int16_t _log_single_line_x = 0;
    int16_t _log_single_line_y = 0;

    // 音频相关私有方法
    static void _audioPlaybackTask(void* parameter);
    static bool _playWavFileInTask(FS& fs, const char* filename);

public:
    HAL_Rachel() : _i2c_bus(nullptr), _rtc(nullptr), _imu(nullptr),
                   _echobase(nullptr), _audio_task_handle(nullptr),
                   _audio_queue(nullptr), _is_audio_playing(false),
                   _should_stop_audio(false), _audio_volume(71), _audio_muted(false),
                   _audio_sample_rate(48000), _leds(nullptr),
                   _touch_ready(false)
    {
    }
    ~HAL_Rachel()
    {
        // 清理音频资源
        if (_audio_task_handle != nullptr) {
            vTaskDelete(_audio_task_handle);
            _audio_task_handle = nullptr;
        }
        if (_audio_queue != nullptr) {
            vQueueDelete(_audio_queue);
            _audio_queue = nullptr;
        }
        if (_echobase != nullptr) {
            delete _echobase;
            _echobase = nullptr;
        }
        
        delete _leds;
        delete _rtc;
        delete _imu;
        delete _i2c_bus;
    }

    inline std::string type() override { return "Rachel"; }

    inline void init() override
    {
        _power_init();
        _disp_init();
        _gamepad_init();
        _spk_init();
        _i2c_init();
        _rtc_init();
        _imu_init();
        _fs_init();
        _sdcard_init();
        _audio_init();
        _ws2812_init();
        _touch_init();
        _system_config_init();
        _sum_up();
    }

    // 设置单行日志模式与位置
    inline void setLogSingleLineMode(bool enabled, int16_t x = 0, int16_t y = 0)
    {
        _log_single_line_mode = enabled;
        _log_single_line_x = x;
        _log_single_line_y = y;
    }

    void reboot() override;
    void loadTextFont24() override;
    void loadTextFont16() override;
    void loadLauncherFont24() override;

    bool getButton(GAMEPAD::GamePadButton_t button) override;
    int getJoystickX() override;
    int getJoystickY() override;
    int getEncoderDelta() override;
    void powerOff() override;
    void setSystemTime(tm dateTime) override;
    void lightSleepWithButtonWakeup() override;
    void updateImuData() override;
    void beep(float frequency, uint32_t duration) override;
    void beepStop() override;
    void setBeepVolume(uint8_t volume) override;
    void loadSystemConfig() override;
    void saveSystemConfig() override;
    
    // 音频相关接口实现
    bool playWavFile(const char* filename) override;
    bool isAudioPlaying() override;
    void stopAudioPlayback() override;
    void setAudioVolume(uint8_t volume) override;
    uint8_t getAudioVolume() override;
    void setAudioMute(bool mute) override;

    // WS2812 LED 接口实现
    void setLedColor(uint8_t index, uint8_t r, uint8_t g, uint8_t b) override;
    void setAllLedColor(uint8_t r, uint8_t g, uint8_t b) override;
    void setLedOff() override;
    void ledShow() override;
    void setLedBrightness(uint8_t brightness) override;

    // MPR121 触摸接口实现
    uint16_t getTouchStatus() override;
    bool isTouched(uint8_t channel) override;
    uint16_t getTouchFiltered(uint8_t channel) override;
    uint16_t getTouchBaseline(uint8_t channel) override;
    bool checkTouch() override;

    // 录音相关（保存为原始PCM数据到SD卡）
    bool recordPcmToSd(const char* filename, int durationSeconds);
    int getRecordBufferSize(int durationSeconds);
    // 录音到SD卡WAV文件（带WAV头）
    bool recordWavToSd(const char* filename, int durationSeconds);

    // 非阻塞WAV录音：开始/步进/停止
    bool startWavRecording(const char* filename);
    bool recordWavStep(size_t chunkBytes = 8192, int16_t* outPeak = nullptr);
    bool stopWavRecording();
    bool isWavRecording();

    // I2C bus access
    inline m5::I2C_Class* getI2C() { return _i2c_bus; }
};
