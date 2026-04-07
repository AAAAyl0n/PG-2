#include "app_genshin.h"
#include "spdlog/spdlog.h"
#include "../../hal/hal.h"
#include "../assets/theme/theme.h"
#include "../utils/system/ui/ui.h"

using namespace MOONCAKE::APPS;

// GIF 相关已抽离到 GifPlayer

void AppGenshin::onCreate() { 
    spdlog::info("{} onCreate", getAppName()); 
}

void AppGenshin::onResume() { 
    spdlog::info("{} onResume", getAppName()); 

    // 居中并打开 GIF
    int gif_x = (HAL::GetDisplay()->width() - 240) / 2;
    int gif_y = (HAL::GetDisplay()->height() - 240) / 2;

    auto canvas = HAL::GetCanvas();
    canvas->fillScreen(TFT_BLACK);
    HAL::CanvasUpdate();

    if (!_gifPlayer.open(_gifPaths[_currentGifIndex], gif_x, gif_y)) {
        spdlog::error("GIF open failed: {}", _gifPaths[_currentGifIndex]);
    }
}

using namespace SYSTEM::UI;

void AppGenshin::onRunning()
{
    // 检查按键退出
    if (HAL::GetButton(GAMEPAD::BTN_SELECT)) {
        // 退出前停止播放
        destroyApp();
    }

    // 检查START键切换GIF
    bool startPressed = HAL::GetButton(GAMEPAD::BTN_START);
    if (startPressed && !_startKeyPressed) {
        // 切换到下一个GIF
        _currentGifIndex = (_currentGifIndex + 1) % 3;
        
        // 关闭当前GIF
        _gifPlayer.close();
        
        // 计算居中位置
        int gif_x = (HAL::GetDisplay()->width() - 240) / 2;
        int gif_y = (HAL::GetDisplay()->height() - 240) / 2;
        
        // 清屏
        auto canvas = HAL::GetCanvas();
        canvas->fillScreen(TFT_BLACK);
        HAL::CanvasUpdate();
        
        // 打开新的GIF
        if (!_gifPlayer.open(_gifPaths[_currentGifIndex], gif_x, gif_y)) {
            spdlog::error("GIF open failed: {}", _gifPaths[_currentGifIndex]);
        } else {
            spdlog::info("切换到GIF: {}", _gifPaths[_currentGifIndex]);
        }
    }
    _startKeyPressed = startPressed;

    // 驱动 GIF 帧推进
    if (_gifPlayer.isOpened()) {
        _gifPlayer.update();
    } else {
        delay(30);
    }
}

void AppGenshin::onDestroy() { 
    spdlog::info("{} onDestroy", getAppName()); 
    _gifPlayer.close();
}
