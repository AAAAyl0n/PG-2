/**
 * @file lgfx_fx.hpp
 * @author Forairaaaaa
 * @brief
 * @version 0.1
 * @date 2023-11-11
 *
 * @copyright Copyright (c) 2023
 *
 */
#pragma once
#include <LovyanGFX.hpp>

/**
 * @brief Add apis to fill shape layer in difference mode 添加api以差分模式填充形状层
 *
 */
class LGFX_SpriteFx : public LGFX_Sprite
{
public:
    LGFX_SpriteFx(LovyanGFX* parent) : LGFX_Sprite(parent) {}

    void drawFastHLineInDifference(int32_t x, int32_t y, int32_t w);
    void fillRectInDifference(int32_t x, int32_t y, int32_t w, int32_t h);
    void fillSmoothRoundRectInDifference(int32_t x, int32_t y, int32_t w, int32_t h, int32_t r);
    inline void fillSmoothCircleInDifference(int32_t x, int32_t y, int32_t r)
    {
        fillSmoothRoundRectInDifference(x - r, y - r, r * 2 + 1, r * 2 + 1, r);
    }
    
    // 平滑三角形绘制函数
    void fillSmoothTriangle(int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t x2, int32_t y2);
    void fillSmoothTriangle(int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint32_t color);
    
    // 高性能版本（2x2采样）
    void fillSmoothTriangleFast(int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t x2, int32_t y2);
    void fillSmoothTriangleFast(int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint32_t color);

     // 高性能平滑圆弧绘制函数
    void fillSmoothArc(int32_t x, int32_t y, int32_t r0, int32_t r1, float angle0, float angle1);
    void fillSmoothArc(int32_t x, int32_t y, int32_t r0, int32_t r1, float angle0, float angle1, uint32_t color);

private:
    // 辅助函数：计算点到直线的距离 Helper function: calculate distance from point to line
    float pointToLineDistance(int32_t px, int32_t py, int32_t x1, int32_t y1, int32_t x2, int32_t y2);
    // 辅助函数：判断点是否在三角形内部 Helper function: check if point is inside triangle
    bool isPointInTriangle(int32_t px, int32_t py, int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t x2, int32_t y2);
    bool isPointInTriangle(float px, float py, float x0, float y0, float x1, float y1, float x2, float y2);
    // 辅助函数：计算三角形的边缘平滑度 Helper function: calculate triangle edge smoothness
    float calculateTriangleAlpha(int32_t px, int32_t py, int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t x2, int32_t y2);
    // 可调采样精度的alpha计算 Adjustable sampling precision alpha calculation
    float calculateTriangleAlpha(int32_t px, int32_t py, int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t x2, int32_t y2, int samples);
};
