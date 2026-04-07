/**
 * @file app_genshin.h
 * @author Forairaaaaa
 * @brief
 * @version 0.1
 * @date 2023-11-04
 *
 * @copyright Copyright (c) 2023
 *
 */
#include <mooncake.h>
#include <math.h>
#include "assets/icon_app_genshin.hpp"
#include "gif_player.h"

namespace MOONCAKE::APPS
{
    /**
     * @brief Genshin
     *
     */
    class AppGenshin : public APP_BASE
    {
    private:
        // 应用状态变量
        bool _audio_started;
        // GIF 播放器
        GifPlayer _gifPlayer;
        // GIF 文件路径数组
        const char* _gifPaths[3] = {"/gif/ASES2.gif", "/gif/ASES3.gif", "/gif/ASES4.gif"};
        // 当前播放的GIF索引
        int _currentGifIndex;
        // START键按下状态（防止重复触发）
        bool _startKeyPressed;
        
    public:
        AppGenshin() : _audio_started(false), _currentGifIndex(0), _startKeyPressed(false) {}
        
        void onCreate() override;
        void onResume() override;
        void onRunning() override;
        void onDestroy() override;
    };

    class AppGenshin_Packer : public APP_PACKER_BASE
    {
        std::string getAppName() override { return "gifplayer"; }
        void* getAppIcon() override { return (void*)image_data_icon_app_genshin; }
        void* newApp() override { return new AppGenshin; }
        void deleteApp(void* app) override { delete (AppGenshin*)app; }
    };
} // namespace MOONCAKE::APPS