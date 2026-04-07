#include "gif_player.h"
#include "../../hal/hal.h"
#include <FS.h>
#include <LittleFS.h>

using namespace MOONCAKE::APPS;

// Static state for GIF callbacks
static int s_gif_x = 0;
static int s_gif_y = 0;

// AnimatedGIF file callbacks
static void* GIFOpenFile(const char* szFilename, int32_t* pFileSize)
{
    File f = SD.open(szFilename, FILE_READ);
    //File f = LittleFS.open(szFilename, FILE_READ);
    if (!f) return nullptr;
    File* handle = new File(f);
    *pFileSize = f.size();
    return (void*)handle;
}

static void GIFCloseFile(void* pHandle)
{
    if (!pHandle) return;
    File* pf = (File*)pHandle;
    pf->close();
    delete pf;
}

static int32_t GIFReadFile(GIFFILE* pFile, uint8_t* pBuf, int32_t iLen)
{
    if (!pFile || !pFile->fHandle) return 0;
    File* pf = (File*)pFile->fHandle;
    int32_t bytes = pf->read(pBuf, iLen);
    if (bytes > 0) pFile->iPos += bytes;
    return bytes;
}

static int32_t GIFSeekFile(GIFFILE* pFile, int32_t iPosition)
{
    if (!pFile || !pFile->fHandle) return -1;
    File* pf = (File*)pFile->fHandle;
    bool ok = pf->seek(iPosition, SeekSet);
    if (ok) pFile->iPos = iPosition;
    return ok ? iPosition : -1;
}

// Draw callback
static void GIFDraw(GIFDRAW* pDraw)
{
    auto canvas = HAL::GetCanvas();
    static uint16_t line[240];
    const int width = pDraw->iWidth;
    const uint8_t* src = pDraw->pPixels;
    const uint16_t* pal565 = (const uint16_t*)pDraw->pPalette;
    const bool hasTrans = pDraw->ucHasTransparency != 0;
    const uint8_t transparent = pDraw->ucTransparent;

    const int16_t dstX = s_gif_x + pDraw->iX;
    const int16_t dstY = s_gif_y + pDraw->iY + pDraw->y;

    //canvas->startWrite();

    if (!hasTrans) {
        for (int x = 0; x < width; x++) {
            line[x] = pal565[src[x]];
        }
        canvas->pushImage(dstX, dstY, width, 1, line);
    } else {
        canvas->readRect(dstX, dstY, width, 1, line);
        for (int x = 0; x < width; x++) {
            const uint8_t idx = src[x];
            if (idx != transparent) {
                line[x] = pal565[idx];
            }
        }
        canvas->pushImage(dstX, dstY, width, 1, line);
    }

    //canvas->endWrite();
}

GifPlayer::GifPlayer()
    : _opened(false), _x(0), _y(0), _lastFrameTime(0) {}

bool GifPlayer::open(const char* path, int x, int y)
{
    setPosition(x, y);
    _gif.begin(BIG_ENDIAN_PIXELS);
    if (_gif.open(path, GIFOpenFile, GIFCloseFile, GIFReadFile, GIFSeekFile, GIFDraw)) {
        _opened = true;
        return true;
    }
    return false;
}

void GifPlayer::setPosition(int x, int y)
{
    _x = x;
    _y = y;
    s_gif_x = x;
    s_gif_y = y;
}

void GifPlayer::update()
{
    if (!_opened) return;
    int delay_ms = 0;
    int rc = _gif.playFrame(true, &delay_ms);
    if (rc == 0) {
        _gif.reset();
    }
    HAL::CanvasUpdate();

    unsigned long currentTime = millis();
    int actualDelay = delay_ms;
    if (delay_ms < 5) actualDelay = 5;
    if (delay_ms > 200) actualDelay = 200;
    unsigned long elapsed = currentTime - _lastFrameTime;
    if (elapsed < actualDelay) {
        delay(actualDelay - elapsed);
    }
    _lastFrameTime = millis();
}

void GifPlayer::reset()
{
    if (_opened) _gif.reset();
}

void GifPlayer::close()
{
    if (_opened) {
        _gif.close();
        _opened = false;
    }
}


