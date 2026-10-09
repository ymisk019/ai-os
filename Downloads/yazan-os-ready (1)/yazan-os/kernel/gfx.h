#pragma once
#include <stdint.h>
#include <stddef.h>

typedef uint32_t rgb_t;   /* 0xRRGGBB */

typedef struct {
    uint8_t *base;
    uint32_t pitch, width, height;
    uint8_t  bpp;
    uint8_t  rpos, rsize, gpos, gsize, bpos, bsize;
} gfx_fb_t;

enum {
    ICON_TERMINAL = 1,
    ICON_CALCULATOR,
    ICON_EXPLORER,
    ICON_NOTEPAD,
    ICON_SETTINGS,
    ICON_TASKMGR,
    ICON_POWER,
    ICON_YAZAN_LOGO,
    ICON_BRICKS
};

void     gfx_init(const gfx_fb_t *fb);
int      gfx_ready(void);
uint32_t gfx_width(void);
uint32_t gfx_height(void);
void     gfx_enable_double_buffer(void);
void     gfx_flip(void);
uint8_t *gfx_get_back_buffer(void);

rgb_t    gfx_mix(rgb_t a, rgb_t b, int t256);
void     gfx_pixel(int x, int y, rgb_t c);
void     gfx_fill_rect(int x, int y, int w, int h, rgb_t c);
void     gfx_rect_outline(int x, int y, int w, int h, rgb_t c);
void     gfx_fill_rrect(int x, int y, int w, int h, int r, rgb_t c);
void     gfx_vgradient(int x, int y, int w, int h, rgb_t top, rgb_t bottom);
void     gfx_blend_rect(int x, int y, int w, int h, rgb_t c, int alpha256);
void     gfx_clear(rgb_t c);

void     gfx_text(int x, int y, const char *s, int scale, rgb_t c);
int      gfx_text_width(const char *s, int scale);
void     gfx_text_center(int cx, int y, const char *s, int scale, rgb_t c);
void     gfx_char(int x, int y, char ch, int scale, rgb_t fg, rgb_t bg);
void     gfx_scroll_up(int x, int y, int w, int h, int dy, rgb_t fill);

void     gfx_draw_cursor(int x, int y);
void     gfx_draw_icon(int x, int y, int icon_type, int size);
