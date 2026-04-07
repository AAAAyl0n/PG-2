# PocketGuitar

通过电容触摸弦、模拟摇杆和旋转编码器实现和弦演奏。

## 交互方式

- **六根触摸弦** — MPR121 电容触摸（ELE0~ELE5），触弦发声
- **摇杆** — 控制和弦配置（选择具体和弦）
- **滚轮（旋转编码器）** — 切换和弦组
- **BOOT 键（GPIO0）** — 返回/退出
- **START 键（GPIO8）** — 确认/功能键

## 硬件

- **MCU**: ESP32-S3, 240MHz, 16MB Flash
- **屏幕**: 240x240 SPI LCD (LovyanGFX)
- **音频**: ES8311 codec, I2S 输出
- **触摸**: MPR121QR2, 12 通道电容触摸 (I2C 0x5A)
- **IMU**: BMI270 (I2C)
- **RTC**: RTC8563 (I2C)
- **LED**: 11 颗 WS2812 (GPIO21)
- **SD 卡**: SPI 接口
- **MIDI**: UART TX (GPIO9, 预留 SAM2695)

## 构建

基于 PlatformIO + Arduino framework。

```bash
# 编译
pio run -e rachel-sdk

# 烧录
pio run -e rachel-sdk -t upload

# 上传文件系统 (LittleFS)
pio run -e rachel-sdk -t uploadfs
```

## 架构

```
Arduino setup/loop
  → RACHEL::Setup/Loop
    → HAL::Inject(HAL_Rachel)    // 硬件抽象层
    → Mooncake::init()           // App 框架
    → install Apps               // 注册应用
```

- **HAL 层**: 统一屏幕、输入、音频、触摸、LED 等硬件接口
- **App 层**: Mooncake 生命周期管理 (onCreate/onResume/onRunning/onDestroy)
- **输入映射**: 编码器左右 → BTN_SELECT/BTN_RIGHT, 摇杆按键 → BTN_JOYSTICK, BOOT → BTN_BACK
