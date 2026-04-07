#pragma once

#include <AnimatedGIF.h>
#include <FS.h>
#include <SD.h>

namespace MOONCAKE::APPS {

class GifPlayer {
public:
    GifPlayer();

    bool open(const char* path, int x, int y);
    void setPosition(int x, int y);
    void update();
    void reset();
    void close();
    bool isOpened() const { return _opened; }

private:
    AnimatedGIF _gif;
    bool _opened;
    int _x;
    int _y;
    unsigned long _lastFrameTime;
};

} // namespace MOONCAKE::APPS


