#pragma once

#include <kot/heap.h>
#include <kot/math.h>

typedef struct
{
    uint64_t fb_addr;
    size_t fb_size;
    uint32_t width;
    uint32_t height;
    uint32_t pitch;
    uint32_t bpp;
    uint32_t btpp;
} framebuffer_t;

enum WindowStyle
{
    None,
    NoResize,
    Default
};

class Context
{
private:
    framebuffer_t *framebuffer;

public:
    Context(framebuffer_t *framebuffer);

    void putPixel(uint32_t x, uint32_t y, uint32_t colour);
    void fillRect(uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t colour);
    void fillTri(uint32_t x1, uint32_t y1, uint32_t x2, uint32_t y2, uint32_t x3, uint32_t y3, uint32_t colour);
    void drawRect(uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t colour);
    void drawTri(uint32_t x1, uint32_t y1, uint32_t x2, uint32_t y2, uint32_t x3, uint32_t y3, uint32_t colour);
    void drawLine(uint32_t x1, uint32_t y1, uint32_t x2, uint32_t y2, uint32_t colour);

    void swapTo(framebuffer_t *to);
    void swapFrom(framebuffer_t *from);
    void swapTo(Context *to);
    void swapFrom(Context *from);
    void blitTo(framebuffer_t *to, uint32_t x, uint32_t y);
    void blitFrom(framebuffer_t *from, uint32_t x, uint32_t y);
    void blitTo(Context *to, uint32_t x, uint32_t y);
    void blitFrom(Context *from, uint32_t x, uint32_t y);
    void clear();
    void fill(uint32_t colour);

    framebuffer_t *getFramebuffer();
};