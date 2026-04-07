/**
 * @file lgfx_fx.cpp
 * @author Forairaaaaa
 * @brief
 * @version 0.1
 * @date 2023-11-11
 *
 * @copyright Copyright (c) 2023
 *
 */
#include "lgfx_fx.h"
#include <algorithm>
#include <cmath>

void LGFX_SpriteFx::drawFastHLineInDifference(int32_t x, int32_t y, int32_t w)
{
    if (!_panel->isReadable())
        drawFastHLine(x, y, w);

    startWrite();

    // Get fill color
    lgfx::rgb888_t fill_color;
    fill_color.set(_write_conv.revert_rgb888(_color.raw));

    lgfx::bgr888_t last_color_buffer;
    lgfx::bgr888_t color_buffer;
    uint32_t draw_line_width = 0;

    readRectRGB(x, y, 1, 1, &color_buffer);
    last_color_buffer.R8(abs((int16_t)fill_color.R8() - color_buffer.R8()));
    last_color_buffer.G8(abs((int16_t)fill_color.G8() - color_buffer.G8()));
    last_color_buffer.B8(abs((int16_t)fill_color.B8() - color_buffer.B8()));

    for (int j = 0; j < w; j++)
    {
        // Get difference
        readRectRGB(x + j, y, 1, 1, &color_buffer);
        color_buffer.R8(abs((int16_t)fill_color.R8() - color_buffer.R8()));
        color_buffer.G8(abs((int16_t)fill_color.G8() - color_buffer.G8()));
        color_buffer.B8(abs((int16_t)fill_color.B8() - color_buffer.B8()));

        // If comes new color
        if (color_buffer.get() != last_color_buffer.get())
        {
            setColor(last_color_buffer);
            drawFastHLine(x + j - draw_line_width, y, draw_line_width);
            last_color_buffer = color_buffer;
            // If also reach the end of line
            if (j == w - 1)
            {
                draw_line_width = 0;
            }
            else
            {
                draw_line_width = 1;
                continue;
            }
        }
        // If reach the end of line
        if (j == w - 1)
        {
            setColor(last_color_buffer);
            drawFastHLine(x + j - draw_line_width, y, draw_line_width + 1);
            draw_line_width = 0;
            last_color_buffer = color_buffer;
            break;
        }
        draw_line_width++;
    }

    setColor(fill_color);
    endWrite();
}

void LGFX_SpriteFx::fillRectInDifference(int32_t x, int32_t y, int32_t w, int32_t h)
{
    if (!_panel->isReadable())
        fillRect(x, y, w, h);

    _adjust_abs(x, w);
    _adjust_abs(y, h);

    startWrite();
    for (int i = 0; i < h; i++)
    {
        drawFastHLineInDifference(x, y + i, w);
    }
    endWrite();
}

constexpr float LoAlphaTheshold = 1.0f / 32.0f;
constexpr float HiAlphaTheshold = 1.0f - LoAlphaTheshold;

void LGFX_SpriteFx::fillSmoothRoundRectInDifference(int32_t x, int32_t y, int32_t w, int32_t h, int32_t r)
{
    if (!_panel->isReadable())
        fillSmoothRoundRect(x, y, w, h, r);

    startWrite();
    int32_t xs = 0;
    int32_t cx = 0;
    uint32_t rgb888 = _write_conv.revert_rgb888(_color.raw);
    // Limit radius to half width or height
    if (r > w / 2)
        r = w / 2;
    if (r > h / 2)
        r = h / 2;

    y += r;
    h -= 2 * r;
    fillRectInDifference(x, y, w, h);
    h--;
    x += r;
    w -= 2 * r + 1;
    int32_t r1 = r * r;
    r++;
    int32_t r2 = r * r;

    for (int32_t cy = r - 1; cy > 0; cy--)
    {
        int32_t dy2 = (r - cy) * (r - cy);
        for (cx = xs; cx < r; cx++)
        {
            int32_t hyp2 = (r - cx) * (r - cx) + dy2;
            if (hyp2 <= r1)
                break;
            if (hyp2 >= r2)
                continue;
            float alphaf = (float)r - sqrtf(hyp2);
            if (alphaf > HiAlphaTheshold)
                break;
            xs = cx;
            if (alphaf < LoAlphaTheshold)
                continue;
            uint8_t alpha = alphaf * 255;
            fillRectAlpha(x + cx - r, y + cy - r, 1, 1, alpha, rgb888);
            fillRectAlpha(x - cx + r + w, y + cy - r, 1, 1, alpha, rgb888);
            fillRectAlpha(x - cx + r + w, y - cy + r + h, 1, 1, alpha, rgb888);
            fillRectAlpha(x + cx - r, y - cy + r + h, 1, 1, alpha, rgb888);
        }
        drawFastHLineInDifference(x + cx - r, y + cy - r, 2 * (r - cx) + 1 + w);
        drawFastHLineInDifference(x + cx - r, y - cy + r + h, 2 * (r - cx) + 1 + w);
    }
    endWrite();
}

// 辅助函数：计算点到直线的距离
float LGFX_SpriteFx::pointToLineDistance(int32_t px, int32_t py, int32_t x1, int32_t y1, int32_t x2, int32_t y2)
{
    // 计算直线方程 Ax + By + C = 0
    float A = y2 - y1;
    float B = x1 - x2;
    float C = x2 * y1 - x1 * y2;
    
    // 避免除零错误
    float denominator = sqrt(A * A + B * B);
    if (denominator < 0.001f) return 0.0f;
    
    // 点到直线距离公式: |Ax + By + C| / sqrt(A² + B²)
    float distance = abs(A * px + B * py + C) / denominator;
    return distance;
}

// 辅助函数：判断点是否在三角形内部（使用重心坐标法）
bool LGFX_SpriteFx::isPointInTriangle(int32_t px, int32_t py, int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t x2, int32_t y2)
{
    return isPointInTriangle((float)px, (float)py, (float)x0, (float)y0, (float)x1, (float)y1, (float)x2, (float)y2);
}

// 支持浮点数的版本
bool LGFX_SpriteFx::isPointInTriangle(float px, float py, float x0, float y0, float x1, float y1, float x2, float y2)
{
    // 使用叉积方法判断点是否在三角形内部
    auto sign = [](float p1x, float p1y, float p2x, float p2y, float p3x, float p3y) -> float {
        return (p1x - p3x) * (p2y - p3y) - (p2x - p3x) * (p1y - p3y);
    };

    float d1 = sign(px, py, x0, y0, x1, y1);
    float d2 = sign(px, py, x1, y1, x2, y2);
    float d3 = sign(px, py, x2, y2, x0, y0);

    bool has_neg = (d1 < 0) || (d2 < 0) || (d3 < 0);
    bool has_pos = (d1 > 0) || (d2 > 0) || (d3 > 0);

    return !(has_neg && has_pos);
}

// 更精确的抗锯齿计算：使用超采样方法
float LGFX_SpriteFx::calculateTriangleAlpha(int32_t px, int32_t py, int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t x2, int32_t y2)
{
    // 默认使用4x4采样获得高质量抗锯齿效果
    return calculateTriangleAlpha(px, py, x0, y0, x1, y1, x2, y2, 4);
}

// 填充平滑三角形函数
void LGFX_SpriteFx::fillSmoothTriangle(int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t x2, int32_t y2)
{
    // 找到三角形的边界矩形
    int32_t minX = std::min({x0, x1, x2}) - 1;
    int32_t maxX = std::max({x0, x1, x2}) + 1;
    int32_t minY = std::min({y0, y1, y2}) - 1;
    int32_t maxY = std::max({y0, y1, y2}) + 1;
    
    // 限制在屏幕范围内
    minX = std::max(minX, _clip_l);
    maxX = std::min(maxX, _clip_r);
    minY = std::max(minY, _clip_t);
    maxY = std::min(maxY, _clip_b);
    
    uint32_t rgb888 = _write_conv.revert_rgb888(_color.raw);
    
    startWrite();
    
    // 对每个像素进行精确的覆盖度计算
    for (int32_t y = minY; y <= maxY; y++) {
        for (int32_t x = minX; x <= maxX; x++) {
            float alpha = calculateTriangleAlpha(x, y, x0, y0, x1, y1, x2, y2);
            
            if (alpha > 0.001f) {  // 有覆盖
                if (alpha >= 0.999f) {
                    // 完全覆盖，直接绘制
                    writePixel(x, y);
                } else {
                    // 部分覆盖，使用alpha混合
                    uint8_t alphaValue = (uint8_t)(alpha * 255);
                    fillRectAlpha(x, y, 1, 1, alphaValue, rgb888);
                }
            }
        }
    }
    
    endWrite();
}

// 带颜色参数的重载版本：填充平滑三角形
void LGFX_SpriteFx::fillSmoothTriangle(int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint32_t color)
{
    setColor(color);
    fillSmoothTriangle(x0, y0, x1, y1, x2, y2);
}

// 可调采样精度的alpha计算函数
float LGFX_SpriteFx::calculateTriangleAlpha(int32_t px, int32_t py, int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t x2, int32_t y2, int samples)
{
    const float step = 1.0f / samples;
    const float offset = step * 0.5f;
    
    int inside_count = 0;
    int total_samples = samples * samples;
    
    // 将整数坐标转换为浮点数
    float fx0 = (float)x0, fy0 = (float)y0;
    float fx1 = (float)x1, fy1 = (float)y1;
    float fx2 = (float)x2, fy2 = (float)y2;
    
    // 对像素内的多个采样点进行检测
    for (int sy = 0; sy < samples; sy++) {
        for (int sx = 0; sx < samples; sx++) {
            float sample_x = px + offset + sx * step;
            float sample_y = py + offset + sy * step;
            
            // 检查采样点是否在三角形内部
            if (isPointInTriangle(sample_x, sample_y, fx0, fy0, fx1, fy1, fx2, fy2)) {
                inside_count++;
            }
        }
    }
    
    return (float)inside_count / (float)total_samples;
}

// 高性能版本：使用2x2采样的平滑三角形
void LGFX_SpriteFx::fillSmoothTriangleFast(int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t x2, int32_t y2)
{
    // 找到三角形的边界矩形
    int32_t minX = std::min({x0, x1, x2}) - 1;
    int32_t maxX = std::max({x0, x1, x2}) + 1;
    int32_t minY = std::min({y0, y1, y2}) - 1;
    int32_t maxY = std::max({y0, y1, y2}) + 1;
    
    // 限制在屏幕范围内
    minX = std::max(minX, _clip_l);
    maxX = std::min(maxX, _clip_r);
    minY = std::max(minY, _clip_t);
    maxY = std::min(maxY, _clip_b);
    
    uint32_t rgb888 = _write_conv.revert_rgb888(_color.raw);
    
    startWrite();
    
    // 对每个像素进行2x2采样（更快但质量稍低）
    for (int32_t y = minY; y <= maxY; y++) {
        for (int32_t x = minX; x <= maxX; x++) {
            float alpha = calculateTriangleAlpha(x, y, x0, y0, x1, y1, x2, y2, 2);  // 2x2采样
            
            if (alpha > 0.001f) {  // 有覆盖
                if (alpha >= 0.999f) {
                    // 完全覆盖，直接绘制
                    writePixel(x, y);
                } else {
                    // 部分覆盖，使用alpha混合
                    uint8_t alphaValue = (uint8_t)(alpha * 255);
                    fillRectAlpha(x, y, 1, 1, alphaValue, rgb888);
                }
            }
        }
    }
    
    endWrite();
}

// 带颜色参数的高性能版本
void LGFX_SpriteFx::fillSmoothTriangleFast(int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint32_t color)
{
    setColor(color);
    fillSmoothTriangleFast(x0, y0, x1, y1, x2, y2);
}

// 辅助函数：判断点是否在圆弧内
bool isPointInArc(float px, float py, float cx, float cy, float r0, float r1, float angle0, float angle1)
{
    // 计算点到中心的距离和角度
    float dx = px - cx;
    float dy = py - cy;
    float distance = sqrt(dx * dx + dy * dy);
    
    // 检查是否在半径范围内
    if (distance < r0 || distance > r1) {
        return false;
    }
    
    // 计算角度 (0-360度)
    float angle = atan2(dy, dx) * 180.0f / 3.14159265359f;
    if (angle < 0) angle += 360.0f;
    
    // 处理角度跨越0度的情况
    if (angle0 <= angle1) {
        return (angle >= angle0 && angle <= angle1);
    } else {
        return (angle >= angle0 || angle <= angle1);
    }
}

// 计算像素在圆弧中的覆盖度（高性能2x2采样）
float calculateArcAlpha(int32_t px, int32_t py, float cx, float cy, float r0, float r1, float angle0, float angle1)
{
    const float step = 0.5f;  // 2x2采样
    int inside_count = 0;
    
    // 2x2采样点
    for (int sy = 0; sy < 2; sy++) {
        for (int sx = 0; sx < 2; sx++) {
            float sample_x = px + sx * step + 0.25f;
            float sample_y = py + sy * step + 0.25f;
            
            if (isPointInArc(sample_x, sample_y, cx, cy, r0, r1, angle0, angle1)) {
                inside_count++;
            }
        }
    }
    
    return inside_count / 4.0f;  // 返回覆盖度 (0.0 到 1.0)
}

// 高性能平滑圆弧绘制函数
void LGFX_SpriteFx::fillSmoothArc(int32_t x, int32_t y, int32_t r0, int32_t r1, float angle0, float angle1)
{
    // 确保参数正确
    if (r0 > r1) {
        std::swap(r0, r1);
    }
    
    // 计算边界框
    int32_t minX = x - r1 - 1;
    int32_t maxX = x + r1 + 1;
    int32_t minY = y - r1 - 1;
    int32_t maxY = y + r1 + 1;
    
    // 限制在屏幕范围内
    minX = std::max(minX, _clip_l);
    maxX = std::min(maxX, _clip_r);
    minY = std::max(minY, _clip_t);
    maxY = std::min(maxY, _clip_b);
    
    uint32_t rgb888 = _write_conv.revert_rgb888(_color.raw);
    
    startWrite();
    
    // 对每个像素进行覆盖度计算
    for (int32_t py = minY; py <= maxY; py++) {
        for (int32_t px = minX; px <= maxX; px++) {
            float alpha = calculateArcAlpha(px, py, (float)x, (float)y, (float)r0, (float)r1, angle0, angle1);
            
            if (alpha > 0.001f) {  // 有覆盖
                if (alpha >= 0.999f) {
                    // 完全覆盖，直接绘制
                    writePixel(px, py);
                } else {
                    // 部分覆盖，使用alpha混合
                    uint8_t alphaValue = (uint8_t)(alpha * 255);
                    fillRectAlpha(px, py, 1, 1, alphaValue, rgb888);
                }
            }
        }
    }
    
    endWrite();
}

// 带颜色参数的重载版本
void LGFX_SpriteFx::fillSmoothArc(int32_t x, int32_t y, int32_t r0, int32_t r1, float angle0, float angle1, uint32_t color)
{
    setColor(color);
    fillSmoothArc(x, y, r0, r1, angle0, angle1);
}
