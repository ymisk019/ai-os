/* Yazan OS 3.0 Graphics Engine
 * Double-buffering, rounded rects, gradients, alpha glass effects, icon rendering. */
#include "gfx.h"
#include "font8x8.h"
#include "kstring.h"
#include "heap.h"
#include "theme.h"

static gfx_fb_t fb;
static int ready = 0;
static uint8_t *back_buffer = NULL;
static uint8_t *render_base = NULL;

void gfx_init(const gfx_fb_t *f) {
    fb = *f;
    render_base = fb.base;
    ready = 1;
}

int      gfx_ready(void)  { return ready; }
uint32_t gfx_width(void)  { return fb.width; }
uint32_t gfx_height(void) { return fb.height; }
uint8_t *gfx_get_back_buffer(void) { return back_buffer; }

void gfx_enable_double_buffer(void) {
    if (!ready) return;
    size_t buf_size = (size_t)fb.height * fb.pitch;
    back_buffer = (uint8_t *)kmalloc(buf_size);
    if (back_buffer) {
        memcpy(back_buffer, fb.base, buf_size);
        render_base = back_buffer;
    }
}

void gfx_flip(void) {
    if (!ready || !back_buffer) return;
    size_t total_bytes = (size_t)fb.height * fb.pitch;
    /* 64-bit accelerated blit */
    uint64_t *dst = (uint64_t *)fb.base;
    const uint64_t *src = (const uint64_t *)back_buffer;
    size_t words = total_bytes / 8;
    for (size_t i = 0; i < words; i++) {
        dst[i] = src[i];
    }
}

static inline uint32_t pack(rgb_t c) {
    uint32_t r = (c >> 16) & 0xFF, g = (c >> 8) & 0xFF, b = c & 0xFF;
    return ((r >> (8 - fb.rsize)) << fb.rpos) |
           ((g >> (8 - fb.gsize)) << fb.gpos) |
           ((b >> (8 - fb.bsize)) << fb.bpos);
}

static inline rgb_t unpack(uint32_t v) {
    uint32_t r = ((v >> fb.rpos) & ((1 << fb.rsize) - 1)) << (8 - fb.rsize);
    uint32_t g = ((v >> fb.gpos) & ((1 << fb.gsize) - 1)) << (8 - fb.gsize);
    uint32_t b = ((v >> fb.bpos) & ((1 << fb.bsize) - 1)) << (8 - fb.bsize);
    return (r << 16) | (g << 8) | b;
}

static inline void put_packed(int x, int y, uint32_t v) {
    uint8_t *p = render_base + (size_t)y * fb.pitch;
    switch (fb.bpp) {
    case 32: ((uint32_t *)p)[x] = v; break;
    case 24: p += x * 3; p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8); p[2] = (uint8_t)(v >> 16); break;
    case 16: ((uint16_t *)p)[x] = (uint16_t)v; break;
    default: break;
    }
}

static inline uint32_t get_packed(int x, int y) {
    uint8_t *p = render_base + (size_t)y * fb.pitch;
    switch (fb.bpp) {
    case 32: return ((uint32_t *)p)[x];
    case 24: {
        p += x * 3;
        return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16);
    }
    case 16: return (uint32_t)((uint16_t *)p)[x];
    default: return 0;
    }
}

rgb_t gfx_mix(rgb_t a, rgb_t b, int t) {
    if (t < 0) t = 0;
    if (t > 256) t = 256;
    uint32_t ar = (a >> 16) & 0xFF, ag = (a >> 8) & 0xFF, ab = a & 0xFF;
    uint32_t br = (b >> 16) & 0xFF, bg = (b >> 8) & 0xFF, bb = b & 0xFF;
    uint32_t r = (ar * (256 - t) + br * t) >> 8;
    uint32_t g = (ag * (256 - t) + bg * t) >> 8;
    uint32_t bl = (ab * (256 - t) + bb * t) >> 8;
    return (r << 16) | (g << 8) | bl;
}

void gfx_pixel(int x, int y, rgb_t c) {
    if (!ready || x < 0 || y < 0 || x >= (int)fb.width || y >= (int)fb.height) return;
    put_packed(x, y, pack(c));
}

void gfx_fill_rect(int x, int y, int w, int h, rgb_t c) {
    if (!ready || w <= 0 || h <= 0) return;
    int x1 = x + w, y1 = y + h;
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (x1 > (int)fb.width)  x1 = (int)fb.width;
    if (y1 > (int)fb.height) y1 = (int)fb.height;
    if (x >= x1 || y >= y1) return;

    uint32_t v = pack(c);
    for (int j = y; j < y1; j++) {
        if (fb.bpp == 32) {
            uint32_t *row = (uint32_t *)(render_base + (size_t)j * fb.pitch);
            for (int i = x; i < x1; i++) row[i] = v;
        } else {
            for (int i = x; i < x1; i++) put_packed(i, j, v);
        }
    }
}

void gfx_rect_outline(int x, int y, int w, int h, rgb_t c) {
    if (!ready || w <= 0 || h <= 0) return;
    gfx_fill_rect(x, y, w, 1, c);
    gfx_fill_rect(x, y + h - 1, w, 1, c);
    gfx_fill_rect(x, y, 1, h, c);
    gfx_fill_rect(x + w - 1, y, 1, h, c);
}

void gfx_fill_rrect(int x, int y, int w, int h, int r, rgb_t c) {
    if (!ready || w <= 0 || h <= 0) return;
    if (r * 2 > w) r = w / 2;
    if (r * 2 > h) r = h / 2;
    uint32_t v = pack(c);
    int r2 = r * r;

    for (int j = 0; j < h; j++) {
        int py = y + j;
        if (py < 0 || py >= (int)fb.height) continue;
        for (int i = 0; i < w; i++) {
            int px = x + i;
            if (px < 0 || px >= (int)fb.width) continue;

            int dx = 0, dy = 0;
            if (i < r) dx = r - 1 - i;
            else if (i >= w - r) dx = i - (w - r);
            if (j < r) dy = r - 1 - j;
            else if (j >= h - r) dy = j - (h - r);

            if (dx * dx + dy * dy >= r2 && (dx || dy)) continue;
            put_packed(px, py, v);
        }
    }
}

void gfx_vgradient(int x, int y, int w, int h, rgb_t top, rgb_t bottom) {
    if (h <= 0) return;
    for (int j = 0; j < h; j++) {
        gfx_fill_rect(x, y + j, w, 1, gfx_mix(top, bottom, (j * 256) / h));
    }
}

void gfx_blend_rect(int x, int y, int w, int h, rgb_t c, int alpha256) {
    if (!ready || w <= 0 || h <= 0 || alpha256 <= 0) return;
    if (alpha256 >= 256) { gfx_fill_rect(x, y, w, h, c); return; }

    int x1 = x + w, y1 = y + h;
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (x1 > (int)fb.width)  x1 = (int)fb.width;
    if (y1 > (int)fb.height) y1 = (int)fb.height;
    if (x >= x1 || y >= y1) return;

    for (int j = y; j < y1; j++) {
        for (int i = x; i < x1; i++) {
            rgb_t bg = unpack(get_packed(i, j));
            rgb_t mixed = gfx_mix(bg, c, alpha256);
            put_packed(i, j, pack(mixed));
        }
    }
}

void gfx_clear(rgb_t c) {
    gfx_fill_rect(0, 0, (int)fb.width, (int)fb.height, c);
}

int gfx_text_width(const char *s, int scale) {
    return (int)strlen(s) * 8 * scale;
}

void gfx_text(int x, int y, const char *s, int scale, rgb_t c) {
    if (scale < 1) scale = 1;
    for (; *s; s++, x += 8 * scale) {
        unsigned ch = (uint8_t)*s;
        if (ch < 0x20 || ch > 0x7E) ch = '?';
        const uint8_t *g = font8x8[ch - 0x20];
        for (int row = 0; row < 8; row++) {
            for (int col = 0; col < 8; col++) {
                if (g[row] & (1u << col)) {
                    gfx_fill_rect(x + col * scale, y + row * scale, scale, scale, c);
                }
            }
        }
    }
}

void gfx_text_center(int cx, int y, const char *s, int scale, rgb_t c) {
    gfx_text(cx - gfx_text_width(s, scale) / 2, y, s, scale, c);
}

void gfx_char(int x, int y, char ch, int scale, rgb_t fg, rgb_t bg) {
    unsigned u = (uint8_t)ch;
    if (u < 0x20 || u > 0x7E) u = '?';
    const uint8_t *g = font8x8[u - 0x20];
    gfx_fill_rect(x, y, 8 * scale, 8 * scale, bg);
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            if (g[row] & (1u << col)) {
                gfx_fill_rect(x + col * scale, y + row * scale, scale, scale, fg);
            }
        }
    }
}

void gfx_scroll_up(int x, int y, int w, int h, int dy, rgb_t fill) {
    if (!ready || dy <= 0 || w <= 0 || h <= 0) return;
    if (x < 0 || y < 0 || x + w > (int)fb.width || y + h > (int)fb.height) return;
    if (dy > h) dy = h;
    size_t bpp = fb.bpp / 8, bytes = (size_t)w * bpp;
    for (int j = y; j < y + h - dy; j++) {
        memcpy(render_base + (size_t)j * fb.pitch + (size_t)x * bpp,
               render_base + (size_t)(j + dy) * fb.pitch + (size_t)x * bpp, bytes);
    }
    gfx_fill_rect(x, y + h - dy, w, dy, fill);
}

/* Modern sleek cursor: 16x20 arrow with drop shadow */
static const uint16_t cursor_mask[20] = {
    0b1000000000000000,
    0b1100000000000000,
    0b1110000000000000,
    0b1111000000000000,
    0b1111100000000000,
    0b1111110000000000,
    0b1111111000000000,
    0b1111111100000000,
    0b1111111110000000,
    0b1111111111000000,
    0b1111110000000000,
    0b1101111000000000,
    0b1000111100000000,
    0b0000011110000000,
    0b0000011110000000,
    0b0000001111000000,
    0b0000001110000000,
    0b0000000110000000,
    0b0000000000000000,
    0b0000000000000000
};

static const uint16_t cursor_border[20] = {
    0b1100000000000000,
    0b1010000000000000,
    0b1001000000000000,
    0b1000100000000000,
    0b1000010000000000,
    0b1000001000000000,
    0b1000000100000000,
    0b1000000010000000,
    0b1000000001000000,
    0b1000011111100000,
    0b1011010000000000,
    0b1101101000000000,
    0b1000110100000000,
    0b0000011010000000,
    0b0000011010000000,
    0b0000001101000000,
    0b0000001101000000,
    0b0000000110000000,
    0b0000000000000000,
    0b0000000000000000
};

void gfx_draw_cursor(int x, int y) {
    if (!ready) return;
    /* Drop shadow */
    for (int row = 0; row < 18; row++) {
        for (int col = 0; col < 12; col++) {
            if (cursor_mask[row] & (1 << (15 - col))) {
                gfx_pixel(x + col + 2, y + row + 2, 0x101524);
            }
        }
    }
    /* Cursor body & border */
    for (int row = 0; row < 18; row++) {
        for (int col = 0; col < 12; col++) {
            if (cursor_mask[row] & (1 << (15 - col))) {
                if (cursor_border[row] & (1 << (15 - col))) {
                    gfx_pixel(x + col, y + row, 0x0F172A); /* Dark border */
                } else {
                    gfx_pixel(x + col, y + row, 0xFFFFFF); /* White fill */
                }
            }
        }
    }
}

void gfx_draw_icon(int x, int y, int icon_type, int size) {
    if (!ready || size <= 0) return;
    int r = size / 5;

    switch (icon_type) {
    case ICON_TERMINAL:
        gfx_fill_rrect(x, y, size, size, r, 0x1E293B);
        gfx_rect_outline(x, y, size, size, 0x334155);
        gfx_text(x + size / 4, y + size / 3, ">_", size > 24 ? 2 : 1, 0x4ADE80);
        break;

    case ICON_CALCULATOR:
        gfx_fill_rrect(x, y, size, size, r, 0x2563EB);
        gfx_fill_rect(x + size / 5, y + size / 6, size * 3 / 5, size / 5, 0x0F172A);
        gfx_text(x + size / 3, y + size / 2, "+-", size > 24 ? 2 : 1, 0xFFFFFF);
        break;

    case ICON_EXPLORER:
        gfx_fill_rrect(x, y, size, size, r, 0xD97706);
        gfx_fill_rect(x + size / 5, y + size / 4, size * 3 / 5, size / 2, 0xFBBF24);
        break;

    case ICON_NOTEPAD:
        gfx_fill_rrect(x, y, size, size, r, 0x0D9488);
        for (int l = 0; l < 3; l++) {
            gfx_fill_rect(x + size / 4, y + size / 4 + l * (size / 5), size / 2, 2, 0xFFFFFF);
        }
        break;

    case ICON_SETTINGS:
        gfx_fill_rrect(x, y, size, size, r, 0x4B5563);
        gfx_fill_rect(x + size / 4, y + size / 3, size / 2, 3, 0x9CA3AF);
        gfx_fill_rect(x + size / 4, y + size * 2 / 3, size / 2, 3, 0x9CA3AF);
        gfx_fill_rect(x + size / 3, y + size / 3 - 2, 4, 7, 0x38BDF8);
        gfx_fill_rect(x + size * 2 / 3, y + size * 2 / 3 - 2, 4, 7, 0x38BDF8);
        break;

    case ICON_TASKMGR:
        gfx_fill_rrect(x, y, size, size, r, 0x7C3AED);
        gfx_fill_rect(x + size / 5, y + size / 2, size / 5, 2, 0x22C55E);
        gfx_fill_rect(x + size * 2 / 5, y + size / 3, 2, size / 3, 0x22C55E);
        gfx_fill_rect(x + size * 2 / 5 + 2, y + size / 3, size / 5, 2, 0x22C55E);
        gfx_fill_rect(x + size * 3 / 5, y + size / 2, size / 5, 2, 0x22C55E);
        break;

    case ICON_POWER:
        gfx_fill_rrect(x, y, size, size, r, 0xDC2626);
        gfx_text_center(x + size / 2, y + size / 4, "!", size > 24 ? 2 : 1, 0xFFFFFF);
        break;

    case ICON_BRICKS: {
        gfx_fill_rrect(x, y, size, size, r, 0x0F172A);
        rgb_t cols[3] = { 0xEF4444, 0xFACC15, 0x22C55E };
        int bw = size / 4, bh = size / 8 + 1;
        for (int row = 0; row < 3; row++)
            for (int col = 0; col < 3; col++)
                gfx_fill_rect(x + size / 8 + col * (bw + 1), y + size / 6 + row * (bh + 1),
                              bw, bh, cols[row]);
        gfx_fill_rrect(x + size / 4, y + size - size / 6, size / 2, 2, 1, 0x38BDF8);
        break;
    }

    case ICON_YAZAN_LOGO:
    default:
        gfx_fill_rrect(x, y, size, size, r, 0x2563EB);
        gfx_text_center(x + size / 2, y + size / 4, "Y", size > 24 ? 2 : 1, 0xFFFFFF);
        break;
    }
}
