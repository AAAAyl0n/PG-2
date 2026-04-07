# Bangboo！

更新目标：实现Sprte Sheet状态机。插卡更换表情和状态（sheet存在SD卡里面）





## 工程概览

项目核心采用 **Mooncake** 应用框架（App 生命周期/管理器），通过自研的 **HAL（Hardware Abstraction Layer）** 统一屏幕、按键、文件系统、音频、IMU、RTC 等硬件能力，然后在其上实现多个可安装的 `App`（含启动器/设置/录音/音频播放器等）。

## 技术栈与关键依赖

- **构建系统**：PlatformIO（见 `platformio.ini`，环境 `env:rachel-sdk`）
- **目标平台**：`espressif32` / `esp32-s3-devkitc-1` / `Arduino framework`
- **主要库（platformio.ini/lib_deps）**：
  - `forairaaaaa/Mooncake`：App 框架与管理
  - `lovyan03/LovyanGFX`：图形渲染
  - `bblanchon/ArduinoJson`：JSON
  - `h2zero/NimBLE-Arduino` / `ESP32-BLE-Gamepad`：BLE/手柄相关
  - `bitbank2/AnimatedGIF`：GIF 播放

## 目录结构（重点）

- **`platformio.ini`**：PlatformIO 配置（CPU/分区表/LittleFS/依赖库等）
- **`src/`**：固件源代码
  - **`src/main.cpp`**：Arduino `setup()/loop()` 入口，仅转发到 `RACHEL::Setup/Loop()`
  - **`src/rachel/`**：系统核心
    - **`rachel.cpp/.h`**：系统入口；负责 HAL 注入、初始化 Mooncake、安装 Launcher 和 Apps
    - **`hal/`**：硬件抽象层
      - **`hal.h / hal.cpp`**：HAL 抽象基类 + 单例注入（`HAL::Inject()`）
      - **`hal_rachel/`**：真机 HAL 实现（屏幕/按键/FS/RTC/IMU/音频/SD 等）
      - **`hal_simulator/`**：PC/非 ESP 平台的 HAL 实现（基于 LovyanGFX SDL）
    - **`apps/`**：应用层（可安装 App + 启动器 + UI 组件）
      - **`apps.h`**：统一注册入口（`rachel_app_install_callback()`）
      - **`launcher/`**：启动器（菜单/动画/自动启动等）
      - **`app_*/`**：具体 App（例如 `app_settings` / `app_bangboo` / `app_recorder` / `app_music`）
      - **`utils/`**：系统与 UI 工具
        - `system/ui/*`：`SelectMenu`、`ProgressWindow` 等
        - `smooth_menu/*`：菜单/选择器/相机（camera）/动画路径（lv_anim）等
      - **`assets/`**：通用主题/字体/默认图标
- **`data/`**：将被打包到 **LittleFS（Flash 文件系统）** 的资源（例如 `/system_audio/*`、`/bangboo_audio/*`、`/fonts/*`）
- **`scriptscodes/`**：脚本/示例代码（非固件主路径）

## 运行时架构（分层与调用链）

### 分层

- **入口层（Arduino）**：`src/main.cpp`
- **系统层（RACHEL）**：`src/rachel/rachel.*`
- **框架层（Mooncake）**：App 管理、生命周期调度
- **抽象层（HAL）**：统一屏幕/输入/存储/音频/传感器 API
- **应用层（Apps）**：Launcher + 若干 `app_*` 业务应用
- **资源层（Assets/FS）**：
  - LittleFS：`data/` → `/system_audio/*`、`/bangboo_audio/*`、`/fonts/*`…
  - SD：运行期读取（例如 `/audio/*.wav`）

### 启动流程（核心链路）

1. **`setup()`** 调用 `RACHEL::Setup()`
2. **HAL 注入**：
   - 真机：`HAL::Inject(new HAL_Rachel)`
   - 非 ESP：`HAL::Inject(new HAL_Simulator)`
3. 初始化 **Mooncake**：`Mooncake::init()`
4. 安装启动器：`installApp(new Launcher_Packer)` 并 `createApp(launcher)`
5. 安装其它 Apps：调用 `rachel_app_install_callback(mooncake)`（见 `src/rachel/apps/apps.h`）
6. `loop()` 中不断 `RACHEL::Loop()` → `_mooncake->update()`，由框架驱动各 App 生命周期

用一个简化图表示：

```text
Arduino setup/loop
   │
   ▼
RACHEL::Setup/Loop
   │
   ├─ HAL::Inject(HAL_Rachel or HAL_Simulator)
   │
   ├─ Mooncake::init()
   │
   ├─ install Launcher + Apps
   │
   └─ createApp(Launcher)

RACHEL::Loop -> Mooncake::update -> App lifecycle callbacks
```

## HAL（硬件抽象层）设计要点

- **单例注入**：`HAL::Inject()` 在系统启动时注入一次，之后全局通过 `HAL::Get*()`/静态 API 获取硬件能力。
- **两套实现**：
  - `HAL_Rachel`：面向真机（GPIO/I2C/RTC/IMU/SD/音频任务等）
  - `HAL_Simulator`：面向 PC/非 ESP（SDL 窗口模拟显示与按键）
- **典型能力**：显示（`GetDisplay/GetCanvas/CanvasUpdate`）、按键（`GetButton/GetAnyButton`）、时间（`GetLocalTime/SetSystemTime`）、配置（`LoadSystemConfig/SaveSystemConfig/UpdateSystemFromConfig`）、音频（`PlayWavFile/StopAudioPlayback/...`）、IMU（`UpdateImuData/GetImuData`）等。

## Apps（应用层）组织方式

### App 生命周期

所有 App 继承 Mooncake 的 `APP_BASE`，常见回调：
- `onCreate()`：创建期（资源/对象初始化）
- `onResume()`：切到前台（加载字体/准备渲染）
- `onRunning()`：前台循环（处理输入/更新状态/渲染）
- `onRunningBG()`：后台运行（如果允许）
- `onPause()`：进入后台
- `onDestroy()`：销毁（释放资源）

### Packer 模式（App 注册单元）

每个 App 通常配套一个 `APP_PACKER_BASE` 派生类：
- `getAppName()`：启动器显示名称
- `getAppIcon()`：启动器显示图标（一般是 `assets/icon_*.hpp` 的图片数据）
- `newApp()/deleteApp()`：创建/释放 App 实例

### 全量注册点（`apps.h`）

`src/rachel/apps/apps.h` 中的 `rachel_app_install_callback(Mooncake*)` 是项目的 **统一安装入口**：
- 启动器创建前安装（`installApp()`）
- 注释/取消注释对应行即可控制某 App 是否在启动器中可见

## UI / 菜单体系

- **Launcher 菜单**：`src/rachel/apps/launcher/`，内部使用 `utils/smooth_menu/simple_menu` 进行选择器/相机/动画驱动。
- **通用 UI**：`src/rachel/apps/utils/system/ui/`
  - `SelectMenu`：阻塞式选择菜单（常用于设置页/确认弹窗）
  - `ProgressWindow`：进度窗口
- **主题与字体**：`src/rachel/apps/assets/theme/theme.h`、`src/rachel/apps/assets/fonts/`

## 资源与文件系统约定

- **LittleFS（Flash 内置）**：来自根目录 `data/`
  - 例如：`/system_audio/Home.wav`、`/bangboo_audio/bada1.wav`、`/fonts/font_text_24.vlw`
- **SD 卡**：运行期读写
  - 例如：`AppRecorder` 写入 `/audio/YYMMDDHHMMSS.wav`
  - `AppMusic` 从 `/audio/` 扫描并播放 `.wav`

## 构建、烧录与资源上传（PlatformIO）

工程环境名：`rachel-sdk`（见 `platformio.ini`）。

- **编译固件**：

```bash
pio run -e rachel-sdk
```

- **上传固件**：

```bash
pio run -e rachel-sdk -t upload
```

- **构建 LittleFS 镜像（打包 data/）**：

```bash
pio run -e rachel-sdk -t buildfs
```

- **上传 LittleFS 到设备**：

```bash
pio run -e rachel-sdk -t uploadfs
```

- **一次性上传（FS + 固件）**：

```bash
pio run -e rachel-sdk -t uploadfs -t upload
```

（上述命令也记录在 `problemcache.md`）

## 如何新增一个 App

项目提供了 App 脚手架：`src/rachel/apps/tools/app_generator.py`。

- **用法**（注意该脚本使用相对路径，建议在其目录下执行）：

```bash
cd src/rachel/apps/tools
python app_generator.py
```

它会：
- 从 `app_template/` 复制并生成 `app_<name>/`
- 自动把头文件 include 和 `installApp(...)` 注入到 `src/rachel/apps/apps.h`

## 备注

- `include/` 与 `lib/` 为 PlatformIO 标准目录：用于放置额外头文件与私有库（目前基本为占位说明）。
