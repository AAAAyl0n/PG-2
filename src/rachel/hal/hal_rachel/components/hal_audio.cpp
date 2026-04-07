/**
 * @file hal_audio.cpp
 * @author Forairaaaaa
 * @brief HAL层音频系统实现
 * @version 0.1
 * @date 2023-11-20
 *
 * @copyright Copyright (c) 2023
 *
 */
#include <mooncake.h>
#include "../hal_rachel.h"
#include <Arduino.h>
#include "../hal_config.h"
#include <FS.h>
#include <SD.h>
#include "driver/i2s.h"
#include <LittleFS.h>
#include <FS.h>

// 静态成员，用于任务访问HAL实例
static HAL_Rachel* g_hal_instance = nullptr;

// 非阻塞录音内部状态
static bool s_is_recording = false;
static size_t s_rec_written = 0;
static File s_rec_file;

void HAL_Rachel::_audio_init()
{
    spdlog::info("音频系统初始化开始...");
    
    // 设置全局HAL实例指针，供静态任务函数使用
    g_hal_instance = this;
    
    // 创建M5EchoBase实例
    _echobase = new M5EchoBase(I2S_NUM_0);
    if (_echobase == nullptr) {
        spdlog::error("无法创建M5EchoBase实例!");
        return;
    }
    
    
    // 初始化音频硬件 - 使用设备引脚配置和I2C总线
    const int SAMPLE_RATE = 48000;
    _audio_sample_rate = SAMPLE_RATE;
    bool init_success = _echobase->init(
        SAMPLE_RATE,    // 采样率
        14,             // I2C SDA
        13,             // I2C SCL  
        36,             // I2S DIN
        37,             // I2S WS
        38,             // I2S DOUT
        35,             // I2S BCK
        _i2c_bus        // I2C总线实例
    );
    
    if (!init_success) {
        spdlog::error("M5EchoBase初始化失败!");
        delete _echobase;
        _echobase = nullptr;
        return;
    }

    // 设置音频参数
    _echobase->setSpeakerVolume(_audio_volume);        // 音量设置
    _echobase->setMicGain(ES8311_MIC_GAIN_0DB);       // 设置麦克风增益（降低以避免炸麦）
    _echobase->setMute(_audio_muted);                 // 设置静音状态
    
    // 创建FreeRTOS音频队列
    _audio_queue = xQueueCreate(5, sizeof(AudioCommand_t));
    if (_audio_queue == nullptr) {
        spdlog::error("无法创建音频命令队列");
        delete _echobase;
        _echobase = nullptr;
        return;
    }
    
    // 创建音频播放任务
    BaseType_t result = xTaskCreatePinnedToCore(
        _audioPlaybackTask,        // 任务函数
        "HAL_AudioTask",           // 任务名称
        12288,                     // 栈大小12KB
        this,                      // 参数（传递HAL实例）
        configMAX_PRIORITIES - 4,  // 优先级
        &_audio_task_handle,       // 任务句柄
        0                          // 分配到core0
    );
    
    if (result != pdPASS) {
        spdlog::error("无法创建音频播放任务");
        vQueueDelete(_audio_queue);
        _audio_queue = nullptr;
        delete _echobase;
        _echobase = nullptr;
        return;
    }
    
    spdlog::info("音频系统初始化完成！");
    i2s_zero_dma_buffer(I2S_NUM_0);
}

// FreeRTOS音频播放任务（静态函数）
void HAL_Rachel::_audioPlaybackTask(void* parameter)
{
    HAL_Rachel* hal = static_cast<HAL_Rachel*>(parameter);
    AudioCommand_t command;
    
    spdlog::info("HAL音频播放任务已启动");
    
    while (true) {
        // 等待音频播放命令
        if (xQueueReceive(hal->_audio_queue, &command, portMAX_DELAY) == pdTRUE) {
            // 设置播放状态
            hal->_is_audio_playing = true;
            hal->_should_stop_audio = false;
            
            spdlog::info("开始播放音频: {}", command.filename);
            
            // 播放前取消静音
            //if (hal->_echobase != nullptr) {
            //    hal->_echobase->setMute(false);
            //}

            //vTaskDelay(pdMS_TO_TICKS(20));
            // 播放音频文件
            bool success = _playWavFileInTask(*command.fs_ptr, command.filename);
            
            // 播放结束后恢复静音
            //if (hal->_echobase != nullptr) {
            //    hal->_echobase->setMute(hal->_audio_muted);
            //}
            
            if (success) {
                spdlog::info("音频播放完成");
            } else {
                spdlog::error("音频播放失败");
            }
            
            // 清除播放状态
            hal->_is_audio_playing = false;
            hal->_should_stop_audio = false;
        }
    }
}

// 在任务中播放WAV文件（支持中断）
bool HAL_Rachel::_playWavFileInTask(FS& fs, const char* filename)
{
    if (g_hal_instance == nullptr || g_hal_instance->_echobase == nullptr) {
        spdlog::error("音频系统未正确初始化");
        return false;
    }
    
    // 打开文件
    File file = fs.open(filename, FILE_READ);
    if (!file) {
        spdlog::error("无法打开音频文件: {}", filename);
        return false;
    }

    // 读取WAV头部
    WavHeader_t wav_header;
    if (file.read((uint8_t*)&wav_header, sizeof(wav_header)) != sizeof(wav_header)) {
        spdlog::error("无法读取WAV文件头");
        file.close();
        return false;
    }

    // 复制packed字段到局部变量避免编译错误
    uint16_t audiofmt = wav_header.audiofmt;
    uint16_t channels = wav_header.channel;
    uint32_t sample_rate = wav_header.sample_rate;
    uint16_t bits_per_sample = wav_header.bit_per_sample;

    // 验证WAV文件格式
    if (memcmp(wav_header.RIFF, "RIFF", 4) != 0 || 
        memcmp(wav_header.WAVEfmt, "WAVEfmt ", 8) != 0 ||
        audiofmt != 1) {
        spdlog::error("不支持的WAV文件格式");
        file.close();
        return false;
    }

    // 打印WAV文件信息
    spdlog::info("WAV文件信息:");
    spdlog::info("  声道数: {}", channels);
    spdlog::info("  采样率: {} Hz", sample_rate);
    spdlog::info("  位深度: {} bit", bits_per_sample);

    // 验证音频格式兼容性
    if (bits_per_sample != 16) {
        spdlog::warn("警告: 音频位深度为{}位，可能影响播放质量", bits_per_sample);
    }
    if (channels > 2) {
        spdlog::error("不支持超过2声道的音频");
        file.close();
        return false;
    }

    // 寻找data chunk
    SubChunk_t sub_chunk;
    bool found_data = false;
    
    file.seek(sizeof(WavHeader_t));
    
    while (file.available()) {
        if (file.read((uint8_t*)&sub_chunk, 8) != 8) {
            break;
        }
        
        if (memcmp(sub_chunk.identifier, "data", 4) == 0) {
            found_data = true;
            break;
        } else {
            // 跳过这个chunk
            uint32_t skip_size = sub_chunk.chunk_size;
            file.seek(file.position() + skip_size);
        }
    }

    if (!found_data) {
        spdlog::error("未找到音频数据部分");
        file.close();
        return false;
    }

    uint32_t data_size = sub_chunk.chunk_size;
    spdlog::info("找到音频数据，大小: {} 字节", data_size);

    // 播放PCM数据
    const size_t CHUNK_SIZE = 8192;  // 8KB缓冲区
    uint8_t* buffer = (uint8_t*)malloc(CHUNK_SIZE);
    if (!buffer) {
        spdlog::error("无法分配音频缓冲区");
        file.close();
        return false;
    }
    
    size_t bytes_remaining = data_size;
    uint32_t total_chunks = (data_size + CHUNK_SIZE - 1) / CHUNK_SIZE;
    uint32_t chunks_processed = 0;

    while (bytes_remaining > 0 && file.available() && !g_hal_instance->_should_stop_audio) {
        size_t bytes_to_read = min(CHUNK_SIZE, bytes_remaining);
        size_t bytes_read = file.read(buffer, bytes_to_read);
        
        if (bytes_read > 0) {
            size_t bytes_written = 0;
            esp_err_t err = i2s_write(I2S_NUM_0, buffer, bytes_read, &bytes_written, portMAX_DELAY);
            if (err != ESP_OK) {
                spdlog::error("I2S写入失败");
                break;
            }
            bytes_remaining -= bytes_read;
        } else {
            break;
        }
        
        chunks_processed++;
        
        // 定期让出CPU
        if ((chunks_processed * 4) % total_chunks == 0) {
            taskYIELD();
        }
    }
    
    free(buffer);
    file.close();
    
    // 检查是否是被停止的
    if (g_hal_instance->_should_stop_audio) {
        spdlog::info("音频播放被用户停止");
    } else {
        spdlog::info("音频播放正常完成");
    }
    
    // 清零DMA缓冲区以消除播放结束后的噪声
    i2s_zero_dma_buffer(I2S_NUM_0);
    
    return true;
}

// HAL接口实现
bool HAL_Rachel::playWavFile(const char* filename)
{
    if (_audio_queue == nullptr || _echobase == nullptr) {
        spdlog::error("音频系统未初始化");
        return false;
    }
    
    // 选择文件系统：优先 SD 卡；不可用或文件不存在则回退 LittleFS
    FS* selected_fs = nullptr;
    if (LittleFS.exists(filename)) {
        selected_fs = &LittleFS;
    } else if (SD.cardSize() && SD.totalBytes() && SD.exists(filename)) {
        selected_fs = &SD;
    } else {
        spdlog::warn("找不到音频文件: {} (LittleFS与SD均不存在)", filename);
        return false;
    }
    // 记录将要使用的文件系统
    const char* fs_name = (selected_fs == &SD) ? "SD" : "LittleFS";
    spdlog::info("音频来自: {}", fs_name);
    
    // 检查是否正在播放
    if (isAudioPlaying()) {
        spdlog::warn("已有音频在播放中，请先停止当前播放");
        return false;
    }
    
    // 创建播放命令
    AudioCommand_t command;
    strncpy(command.filename, filename, sizeof(command.filename) - 1);
    command.filename[sizeof(command.filename) - 1] = '\0';
    command.fs_ptr = selected_fs;  // 使用已选择的文件系统
    
    // 发送命令到队列
    if (xQueueSend(_audio_queue, &command, pdMS_TO_TICKS(100)) != pdTRUE) {
        spdlog::error("无法发送音频播放命令");
        return false;
    }
    
    spdlog::info("已发送音频播放命令: {}", filename);
    return true;
}

bool HAL_Rachel::isAudioPlaying()
{
    return _is_audio_playing;
}

void HAL_Rachel::stopAudioPlayback()
{
    _should_stop_audio = true;
    spdlog::info("已发送停止播放信号");
}

void HAL_Rachel::setAudioVolume(uint8_t volume)
{
    _audio_volume = volume;
    if (_echobase != nullptr) {
        _echobase->setSpeakerVolume(volume);
    }
}

uint8_t HAL_Rachel::getAudioVolume()
{
    return _audio_volume;
}

void HAL_Rachel::setAudioMute(bool mute)
{
    _audio_muted = mute;
    if (_echobase != nullptr) {
        _echobase->setMute(mute);
    }
} 

// 录音到SD卡原始PCM文件（无WAV头）
bool HAL_Rachel::recordPcmToSd(const char* filename, int durationSeconds)
{
    if (_echobase == nullptr) {
        spdlog::error("EchoBase 未初始化，无法录音");
        return false;
    }
    if (!SD.cardSize() || !SD.totalBytes()) {
        spdlog::error("SD卡不可用");
        return false;
    }

    int size_bytes = _echobase->getBufferSize(durationSeconds);
    spdlog::info("开始录音: {} 秒, 预计 {} 字节 -> {}", durationSeconds, size_bytes, filename);

    // 静音扬声器，避免录音串入回放
    _echobase->setMute(true);
    bool ok = _echobase->record(SD, filename, size_bytes);
    _echobase->setMute(_audio_muted);

    if (!ok) {
        spdlog::error("录音失败");
    } else {
        spdlog::info("录音完成: {}", filename);
    }
    return ok;
}

int HAL_Rachel::getRecordBufferSize(int durationSeconds)
{
    if (_echobase == nullptr) return 0;
    return _echobase->getBufferSize(durationSeconds);
}

// 写WAV头到文件开头
static bool _write_wav_header(File& file, uint32_t sample_rate, uint16_t channels, uint16_t bits_per_sample, uint32_t data_size)
{
    // RIFF chunk descriptor
    file.seek(0);
    // RIFF
    file.write((const uint8_t*)"RIFF", 4);
    uint32_t chunk_size = 36 + data_size; // 4 + (8 + SubChunk1Size) + (8 + SubChunk2Size)
    file.write((uint8_t*)&chunk_size, 4);
    file.write((const uint8_t*)"WAVE", 4);

    // fmt subchunk
    file.write((const uint8_t*)"fmt ", 4);
    uint32_t subchunk1_size = 16; // PCM
    uint16_t audio_format = 1; // PCM
    uint32_t byte_rate = sample_rate * channels * (bits_per_sample / 8);
    uint16_t block_align = channels * (bits_per_sample / 8);
    file.write((uint8_t*)&subchunk1_size, 4);
    file.write((uint8_t*)&audio_format, 2);
    file.write((uint8_t*)&channels, 2);
    file.write((uint8_t*)&sample_rate, 4);
    file.write((uint8_t*)&byte_rate, 4);
    file.write((uint8_t*)&block_align, 2);
    file.write((uint8_t*)&bits_per_sample, 2);

    // data subchunk
    file.write((const uint8_t*)"data", 4);
    file.write((uint8_t*)&data_size, 4);
    return true;
}

bool HAL_Rachel::recordWavToSd(const char* filename, int durationSeconds)
{
    if (_echobase == nullptr) {
        spdlog::error("EchoBase 未初始化，无法录音");
        return false;
    }
    if (!SD.cardSize() || !SD.totalBytes()) {
        spdlog::error("SD卡不可用");
        return false;
    }

    const uint16_t channels = 2;
    const uint16_t bits_per_sample = 16;
    int pcm_size = _echobase->getBufferSize(durationSeconds, _audio_sample_rate);
    if (pcm_size <= 0) {
        spdlog::error("计算录音大小失败");
        return false;
    }

    // 先创建文件并写占位头
    File file = SD.open(filename, FILE_WRITE);
    if (!file) {
        spdlog::error("无法创建WAV文件: {}", filename);
        return false;
    }
    // 预写头（数据大小暂为0）
    _write_wav_header(file, _audio_sample_rate, channels, bits_per_sample, 0);

    // 从当前偏移开始写入PCM数据
    size_t header_size = 44; // 标准WAV头
    file.seek(header_size);

    // 录音：使用内部API录到临时缓冲区分片写入以避免超大内存
    const size_t CHUNK = 8192;
    uint8_t* buf = (uint8_t*)malloc(CHUNK);
    if (!buf) {
        spdlog::error("内存不足，无法分配缓冲区");
        file.close();
        SD.remove(filename);
        return false;
    }

    // 静音扬声器，避免录音串音
    _echobase->setMute(true);

    size_t bytes_remaining = pcm_size;
    size_t total_written = 0;
    while (bytes_remaining > 0) {
        size_t to_capture = bytes_remaining > CHUNK ? CHUNK : bytes_remaining;
        size_t bytes_read = 0;
        // 直接从I2S读取一片
        esp_err_t err = i2s_read(I2S_NUM_0, buf, to_capture, &bytes_read, portMAX_DELAY);
        if (err != ESP_OK) {
            spdlog::error("录音I2S读取失败");
            free(buf);
            file.close();
            SD.remove(filename);
            _echobase->setMute(_audio_muted);
            return false;
        }
        if (bytes_read > 0) {
            file.write(buf, bytes_read);
            total_written += bytes_read;
            bytes_remaining -= bytes_read;
        }
    }

    _echobase->setMute(_audio_muted);

    // 回写正确的WAV头（包含正确data大小）
    _write_wav_header(file, _audio_sample_rate, channels, bits_per_sample, (uint32_t)total_written);
    file.close();

    spdlog::info("WAV录音完成: {} ({} bytes)", filename, (int)total_written);
    return true;
}

// 非阻塞WAV录音：开始/步进/停止
bool HAL_Rachel::startWavRecording(const char* filename)
{
    if (_echobase == nullptr) {
        spdlog::error("EchoBase 未初始化，无法录音");
        return false;
    }
    if (!SD.cardSize() || !SD.totalBytes()) {
        spdlog::error("SD卡不可用");
        return false;
    }
    if (s_is_recording) {
        spdlog::warn("录音已在进行中");
        return false;
    }

    s_rec_file = SD.open(filename, FILE_WRITE);
    if (!s_rec_file) {
        spdlog::error("无法创建WAV文件: {}", filename);
        return false;
    }

    // 预写WAV头（data_size=0），随后从偏移44开始写PCM
    _write_wav_header(s_rec_file, _audio_sample_rate, 2, 16, 0);
    s_rec_file.seek(44);

    s_rec_written = 0;
    s_is_recording = true;

    _echobase->setMute(true);  // 录音期间静音扬声器，避免串音
    spdlog::info("开始录音: {}", filename);
    return true;
}

bool HAL_Rachel::recordWavStep(size_t chunkBytes)
{
    if (!s_is_recording) return false;
    if (chunkBytes == 0) return true;

    const size_t CHUNK = chunkBytes;
    uint8_t* buf = (uint8_t*)malloc(CHUNK);
    if (!buf) {
        spdlog::error("recordWavStep: 内存不足");
        return false;
    }

    size_t bytes_read = 0;
    esp_err_t err = i2s_read(I2S_NUM_0, buf, CHUNK, &bytes_read, pdMS_TO_TICKS(50));
    if (err == ESP_OK && bytes_read > 0) {
        s_rec_file.write(buf, bytes_read);
        s_rec_written += bytes_read;
    }
    free(buf);
    return true;
}

bool HAL_Rachel::stopWavRecording()
{
    if (!s_is_recording) return false;

    _echobase->setMute(_audio_muted);

    // 回写正确WAV头，包含实际data大小
    _write_wav_header(s_rec_file, _audio_sample_rate, 2, 16, (uint32_t)s_rec_written);
    s_rec_file.close();

    s_is_recording = false;
    spdlog::info("录音结束，写入 {} 字节", (int)s_rec_written);
    return true;
}

bool HAL_Rachel::isWavRecording()
{
    return s_is_recording;
}