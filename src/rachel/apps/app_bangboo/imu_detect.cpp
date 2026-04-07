#include "app_bangboo.h"
#include "spdlog/spdlog.h"
#include "../../hal/hal.h"
#include "../assets/theme/theme.h"

using namespace MOONCAKE::APPS;

void AppBangboo::imu_detection()
{
    HAL::UpdateImuData();
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

    // 触发后的冷却期：避免连续触发表情
    if (currentTime < _data.shakeCooldownUntil) {
        _data.shakeState = SHAKE_IDLE;
        if (_data.shakeIntensity < _data.SHAKE_THRESHOLD * 0.1f) {
            _data.shakeIntensity = 0.0f;
        }
        // 更新上一次的加速度值
        _data.lastAccelX = imuData.accelX;
        _data.lastAccelY = imuData.accelY;
        _data.lastAccelZ = imuData.accelZ;
        _data.lastTime = currentTime;
        return;
    }

    // 摇晃检测状态机
    switch (_data.shakeState) {
        case SHAKE_IDLE:
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
                _data.shakeCooldownUntil = currentTime + _data.SHAKE_COOLDOWN_TIME;
                _data.shakeIntensity = 0.0f;
            } else if (_data.shakeIntensity < _data.SHAKE_THRESHOLD * 0.3f) {
                // 摇晃强度太低，回到空闲状态
                _data.shakeState = SHAKE_IDLE;
                _data.shakeIntensity = 0.0f;
            }
            break;
            
        case SHAKE_TRIGGERED:
            if (_data.currentState ==  STATE_SHAKED) {
                _data.shakeState = SHAKE_IDLE;
                _data.shakeIntensity = 0.0f;
            }
            break;
    }
    
    // 更新上一次的加速度值
    _data.lastAccelX = imuData.accelX;
    _data.lastAccelY = imuData.accelY;
    _data.lastAccelZ = imuData.accelZ;
    _data.lastTime = currentTime;
}