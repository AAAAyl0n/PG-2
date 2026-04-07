/**
 * @file hal_touch.cpp
 * @brief MPR121 capacitive touch driver via m5::I2C_Class
 */
#include <mooncake.h>
#include "../hal_rachel.h"
#include "../hal_config.h"

#define MPR121_ADDR       0x5A
#define MPR121_I2C_FREQ   400000

// MPR121 寄存器
#define MPR121_TOUCHSTATUS_L  0x00
#define MPR121_TOUCHSTATUS_H  0x01
#define MPR121_FILTDATA_0L    0x04
#define MPR121_BASELINE_0     0x1E
#define MPR121_MHDR           0x2B
#define MPR121_NHDR           0x2C
#define MPR121_NCLR           0x2D
#define MPR121_FDLR           0x2E
#define MPR121_MHDF           0x2F
#define MPR121_NHDF           0x30
#define MPR121_NCLF           0x31
#define MPR121_FDLF           0x32
#define MPR121_NHDT           0x33
#define MPR121_NCLT           0x34
#define MPR121_FDLT           0x35
#define MPR121_TOUCHTH_0      0x41
#define MPR121_RELEASETH_0    0x42
#define MPR121_DEBOUNCE       0x5B
#define MPR121_CONFIG1        0x5C
#define MPR121_CONFIG2        0x5D
#define MPR121_ECR            0x5E
#define MPR121_SOFTRESET      0x80

static void _mpr121_write(m5::I2C_Class* bus, uint8_t reg, uint8_t val)
{
    bus->writeRegister8(MPR121_ADDR, reg, val, MPR121_I2C_FREQ);
}

static uint8_t _mpr121_read8(m5::I2C_Class* bus, uint8_t reg)
{
    return bus->readRegister8(MPR121_ADDR, reg, MPR121_I2C_FREQ);
}

static uint16_t _mpr121_read16(m5::I2C_Class* bus, uint8_t reg)
{
    uint8_t buf[2] = {0};
    bus->readRegister(MPR121_ADDR, reg, buf, 2, MPR121_I2C_FREQ);
    return (uint16_t)buf[0] | ((uint16_t)buf[1] << 8);
}

void HAL_Rachel::_touch_init()
{
    spdlog::info("touch init (MPR121)");
    HAL_LOG_INFO("touch init (MPR121)");

    if (_i2c_bus == nullptr) {
        _touch_ready = false;
        HAL_LOG_ERROR("MPR121: no i2c bus");
        return;
    }

    // Soft reset
    _mpr121_write(_i2c_bus, MPR121_SOFTRESET, 0x63);
    delay(1);

    // 验证芯片 - 读 CONFIG2 默认值应为 0x24
    uint8_t cfg2 = _mpr121_read8(_i2c_bus, MPR121_CONFIG2);
    if (cfg2 != 0x24) {
        _touch_ready = false;
        spdlog::error("MPR121 not found (cfg2=0x{:02X})", cfg2);
        HAL_LOG_ERROR("MPR121 not found");
        return;
    }

    // 停止模式下配置 (ECR=0)
    _mpr121_write(_i2c_bus, MPR121_ECR, 0x00);

    // Baseline filter 参数
    _mpr121_write(_i2c_bus, MPR121_MHDR, 0x01);
    _mpr121_write(_i2c_bus, MPR121_NHDR, 0x01);
    _mpr121_write(_i2c_bus, MPR121_NCLR, 0x0E);
    _mpr121_write(_i2c_bus, MPR121_FDLR, 0x00);
    _mpr121_write(_i2c_bus, MPR121_MHDF, 0x01);
    _mpr121_write(_i2c_bus, MPR121_NHDF, 0x05);
    _mpr121_write(_i2c_bus, MPR121_NCLF, 0x01);
    _mpr121_write(_i2c_bus, MPR121_FDLF, 0x00);
    _mpr121_write(_i2c_bus, MPR121_NHDT, 0x00);
    _mpr121_write(_i2c_bus, MPR121_NCLT, 0x00);
    _mpr121_write(_i2c_bus, MPR121_FDLT, 0x00);

    // 触摸/释放阈值 (12 个通道)
    for (uint8_t i = 0; i < 12; i++) {
        _mpr121_write(_i2c_bus, MPR121_TOUCHTH_0 + i * 2, 12);   // 触摸阈值
        _mpr121_write(_i2c_bus, MPR121_RELEASETH_0 + i * 2, 6);  // 释放阈值
    }

    // Debounce
    _mpr121_write(_i2c_bus, MPR121_DEBOUNCE, 0x11);  // touch=1, release=1

    // AFE 配置
    _mpr121_write(_i2c_bus, MPR121_CONFIG1, 0x10);  // FFI=00, CDC=16uA
    _mpr121_write(_i2c_bus, MPR121_CONFIG2, 0x20);  // CDT=0.5us, SFI=00, ESI=16ms

    // 启用 12 个电极
    _mpr121_write(_i2c_bus, MPR121_ECR, 0x8C);  // CL=10 (baseline tracking), ELE=12

    _touch_ready = true;
    spdlog::info("MPR121 ok");
    HAL_LOG_INFO("MPR121 ok");
}

uint16_t HAL_Rachel::getTouchStatus()
{
    if (!_touch_ready) return 0;
    return _mpr121_read16(_i2c_bus, MPR121_TOUCHSTATUS_L) & 0x0FFF;
}

bool HAL_Rachel::isTouched(uint8_t channel)
{
    if (!_touch_ready || channel > 11) return false;
    return (getTouchStatus() >> channel) & 1;
}

uint16_t HAL_Rachel::getTouchFiltered(uint8_t channel)
{
    if (!_touch_ready || channel > 11) return 0;
    return _mpr121_read16(_i2c_bus, MPR121_FILTDATA_0L + channel * 2);
}

uint16_t HAL_Rachel::getTouchBaseline(uint8_t channel)
{
    if (!_touch_ready || channel > 11) return 0;
    // baseline 寄存器只有高 8 位，左移 2 位得到 10-bit 值
    return (uint16_t)_mpr121_read8(_i2c_bus, MPR121_BASELINE_0 + channel) << 2;
}

bool HAL_Rachel::checkTouch()
{
    return _touch_ready;
}
