/**
 * @file app_imutest.cpp
 * @author Forairaaaaa
 * @brief
 * @version 0.1
 * @date 2023-11-04
 *
 * @copyright Copyright (c) 2023
 *
 */
#include "app_imutest.h"
#include "spdlog/spdlog.h"
#include "../../hal/hal.h"
#include "../assets/theme/theme.h"
#include <cmath>
#include <cstdlib>

using namespace MOONCAKE::APPS;

static const auto BANGBOO_EYE_COLOR = THEME_COLOR_LawnGreen;

static void drawEyes()
{
    HAL::GetCanvas()->fillScreen(THEME_COLOR_BLACK);
    HAL::GetCanvas()->fillSmoothCircle(60, 91, 32, BANGBOO_EYE_COLOR);
    HAL::GetCanvas()->fillSmoothCircle(60, 91, 21, THEME_COLOR_BLACK);
    HAL::GetCanvas()->fillSmoothCircle(180, 91, 32, BANGBOO_EYE_COLOR);
    HAL::GetCanvas()->fillSmoothCircle(180, 91, 21, THEME_COLOR_BLACK);
    HAL::CanvasUpdate();
}

static void drawWince(){
    HAL::GetCanvas()->fillScreen(THEME_COLOR_BLACK);
    HAL::GetCanvas()->setColor(BANGBOO_EYE_COLOR);

    HAL::GetCanvas()->fillSmoothTriangle(95, 91, 33, 55, 33, 127, BANGBOO_EYE_COLOR);
    HAL::GetCanvas()->fillSmoothTriangle(73, 91, 33, 67.72, 33, 114.28, THEME_COLOR_BLACK);
    HAL::GetCanvas()->fillSmoothTriangle(145, 91, 207, 55, 207, 127, BANGBOO_EYE_COLOR);
    HAL::GetCanvas()->fillSmoothTriangle(167, 91, 207, 67.72, 207, 114.28, THEME_COLOR_BLACK);

    HAL::GetCanvas()->fillSmoothTriangle(33, 73, 33, 55, 53, 55, THEME_COLOR_BLACK);
    HAL::GetCanvas()->fillSmoothTriangle(33, 109, 33, 127, 53, 127, THEME_COLOR_BLACK);

    HAL::GetCanvas()->fillSmoothTriangle(207, 73, 207, 55, 187, 55, THEME_COLOR_BLACK);
    HAL::GetCanvas()->fillSmoothTriangle(207, 109, 207, 127, 187, 127, THEME_COLOR_BLACK);

    HAL::GetCanvas()->fillTriangle(90, 55, 90, 127, 120, 91, THEME_COLOR_BLACK);
    HAL::GetCanvas()->fillTriangle(150, 55, 150, 127, 120, 91, THEME_COLOR_BLACK);
    HAL::CanvasUpdate();
}

static void drawBlink()
{
    HAL::GetCanvas()->fillScreen(THEME_COLOR_BLACK);
    HAL::GetCanvas()->fillSmoothRoundRect(29, 86, 64, 11, 2, BANGBOO_EYE_COLOR);
    HAL::GetCanvas()->fillSmoothRoundRect(149, 86, 64, 11, 2, BANGBOO_EYE_COLOR);
    
    HAL::CanvasUpdate();
}

void AppImutest::onCreate() { 
    spdlog::info("{} onCreate", getAppName()); 
}

// Like setup()...
void AppImutest::onResume() { spdlog::info("{} onResume", getAppName()); }

// Like loop()...
void AppImutest::onRunning()
{
    // 初始显示正常眼睛
    drawEyes();
    
    // 初始化随机种子
    srand(HAL::Millis());
    
    while (true)
    {
        // 检查按键SELECT退出
        if (HAL::GetButton(GAMEPAD::BTN_SELECT)) {
            destroyApp();
            return;
        }
        
        // 更新IMU数据
        HAL::UpdateImuData();
        
        // 获取IMU数据
        auto& imuData = HAL::GetImuData();
        
        // 获取当前时间
        unsigned long currentTime = HAL::Millis();
        
        // 计算加速度变化量 (摇晃强度)
        if (_data.lastTime > 0) {
            float deltaX = imuData.accelX - _data.lastAccelX;
            float deltaY = imuData.accelY - _data.lastAccelY;
            float deltaZ = imuData.accelZ - _data.lastAccelZ;
            float deltaAccel = sqrt(deltaX * deltaX + deltaY * deltaY + deltaZ * deltaZ);
            
            // 更新摇晃强度（带衰减）
            _data.shakeIntensity = _data.shakeIntensity * _data.SHAKE_DECAY + deltaAccel;
        }
        
        // 摇晃检测状态机
        switch (_data.shakeState) {
            case SHAKE_IDLE:
                // 眨眼逻辑
                if (!_data.isBlinking) {
                    // 检查是否到了眨眼时间
                    if (currentTime - _data.lastBlinkTime >= _data.nextBlinkDelay) {
                        _data.isBlinking = true;
                        _data.blinkStartTime = currentTime;
                        drawBlink();
                        
                        // 设置下一次眨眼的随机间隔（6-8秒）
                        _data.nextBlinkDelay = _data.BLINK_MIN_INTERVAL + 
                                              (rand() % (_data.BLINK_MAX_INTERVAL - _data.BLINK_MIN_INTERVAL));
                    }
                } else {
                    // 检查眨眼是否结束
                    if (currentTime - _data.blinkStartTime >= _data.BLINK_DURATION) {
                        _data.isBlinking = false;
                        _data.lastBlinkTime = currentTime;
                        drawEyes();
                    }
                }
                
                // 检测是否开始摇晃
                if (_data.shakeIntensity > _data.SHAKE_THRESHOLD) {
                    _data.shakeState = SHAKE_DETECTING;
                    _data.shakeStartTime = currentTime;
                }
                break;
                
            case SHAKE_DETECTING:
                // 检测摇晃是否持续足够长时间
                if (currentTime - _data.shakeStartTime >= _data.SHAKE_DETECT_TIME) {
                    // 摇晃持续足够时间，触发显示
                    _data.shakeState = SHAKE_TRIGGERED;
                    _data.triggerStartTime = currentTime;
                    // 随机播放1s_1、1s_3、1s_4、1s_5中的一个音频
                    {
                        int idx = rand() % 3;
                        const char* audio_files[] = {
                            "/bangboo_audio/1s_1.wav",
                            "/bangboo_audio/1s_4.wav",
                            "/bangboo_audio/1s_5.wav"
                        };
                        HAL::PlayWavFile(audio_files[idx]);
                    }
                    drawWince();  // 显示眯眼状态
                } else if (_data.shakeIntensity < _data.SHAKE_THRESHOLD * 0.3f) {
                    // 摇晃强度太低，回到空闲状态
                    _data.shakeState = SHAKE_IDLE;
                    _data.shakeIntensity = 0.0f;
                    drawEyes();  // 恢复正常眼睛
                }
                break;
                
            case SHAKE_TRIGGERED:
                // 显示眯眼状态2秒
                if (currentTime - _data.triggerStartTime >= _data.SHAKE_DISPLAY_TIME) {
                    _data.shakeState = SHAKE_IDLE;
                    _data.shakeIntensity = 0.0f;
                    drawEyes();  // 恢复正常眼睛
                }
                break;
        }
        
        // 更新上一次的加速度值
        _data.lastAccelX = imuData.accelX;
        _data.lastAccelY = imuData.accelY;
        _data.lastAccelZ = imuData.accelZ;
        _data.lastTime = currentTime;
        
        // 注释掉所有的显示输出
        /*
        // 清屏
        HAL::GetCanvas()->fillScreen(THEME_COLOR_BLACK);
        
        // 设置文本颜色和大小
        HAL::GetCanvas()->setTextColor(THEME_COLOR_LIGHT, THEME_COLOR_BLACK);
        HAL::GetCanvas()->setTextSize(1);
        
        // 显示加速度计数据
        HAL::GetCanvas()->setCursor(20, 40);
        HAL::GetCanvas()->printf("Accel (m/s^2):");
        
        HAL::GetCanvas()->setCursor(20, 60);
        HAL::GetCanvas()->printf("X轴: %.3f", imuData.accelX);
        
        HAL::GetCanvas()->setCursor(20, 80);
        HAL::GetCanvas()->printf("Y轴: %.3f", imuData.accelY);
        
        HAL::GetCanvas()->setCursor(20, 100);
        HAL::GetCanvas()->printf("Z轴: %.3f", imuData.accelZ);
        
        // 显示摇晃状态符号
        if (_data.shakeState == SHAKE_TRIGGERED) {
            HAL::GetCanvas()->drawCenterString("> <", 160, 60);
        } else {
            HAL::GetCanvas()->drawCenterString("- -", 160, 60);
        }
        
        // 显示调试信息
        HAL::GetCanvas()->setCursor(20, 120);
        HAL::GetCanvas()->printf("摇晃强度: %.3f", _data.shakeIntensity);
        
        HAL::GetCanvas()->setCursor(20, 140);
        const char* stateStr[] = {"IDLE", "DETECTING", "TRIGGERED"};
        HAL::GetCanvas()->printf("状态: %s", stateStr[_data.shakeState]);
        
        // 显示数据更新计数
        HAL::GetCanvas()->setCursor(20, 160);
        HAL::GetCanvas()->printf("更新次数: %lu", _data.count);
        
        // 更新屏幕显示
        HAL::CanvasUpdate();
        */
        
        // 增加计数器
        _data.count++;
        
        // 延时一下，避免刷新太快
        HAL::Delay(10);
    }
}

void AppImutest::onDestroy() { spdlog::info("{} onDestroy", getAppName()); }
