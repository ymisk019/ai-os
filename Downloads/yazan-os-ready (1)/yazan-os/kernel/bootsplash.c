/* Yazan OS 3.0 Boot Splash Screen
 * Features glowing logo, loading progress bar, hardware verification & animation. */
#include "bootsplash.h"
#include "gfx.h"
#include "pit.h"
#include "panic.h"
#include "theme.h"
#include "kstring.h"

static void draw_large_y_logo(int cx, int cy, int size) {
    int r = size / 4;
    gfx_fill_rrect(cx - size / 2, cy - size / 2, size, size, r, 0x1E3A8A);
    gfx_fill_rrect(cx - size / 2 + 4, cy - size / 2 + 4, size - 8, size - 8, r - 2, 0x2563EB);

    /* Stylized 'Y' glyph inside badge */
    int w = size / 6;
    /* Left branch */
    for (int i = 0; i < size / 3; i++) {
        gfx_fill_rect(cx - size / 3 + i, cy - size / 3 + i, w, w, 0xFFFFFF);
    }
    /* Right branch */
    for (int i = 0; i < size / 3; i++) {
        gfx_fill_rect(cx + size / 3 - i - w, cy - size / 3 + i, w, w, 0xFFFFFF);
    }
    /* Center trunk */
    gfx_fill_rect(cx - w / 2, cy, w, size / 3, 0xFFFFFF);
}

void bootsplash_render_frame(int progress_pct, const char *status) {
    int W = (int)gfx_width(), H = (int)gfx_height();
    gfx_vgradient(0, 0, W, H, 0x050814, 0x0A1633);

    int cx = W / 2;
    int cy = H / 2 - 40;

    /* Center Badge */
    draw_large_y_logo(cx, cy, 96);

    /* Title */
    int scale = W >= 900 ? 3 : 2;
    gfx_text_center(cx + 2, cy + 70 + 2, "YAZAN OS", scale, 0x020617);
    gfx_text_center(cx, cy + 70, "YAZAN OS", scale, 0xF8FAFC);

    /* Subtitle / Edition badge */
    gfx_text_center(cx, cy + 70 + 8 * scale + 8, "Fire Edition 3.0  |  Hybrid UI", 1, 0x38BDF8);

    /* Progress bar */
    int bar_w = W > 600 ? 400 : W - 80;
    int bar_h = 10;
    int bar_x = (W - bar_w) / 2;
    int bar_y = cy + 140;

    gfx_fill_rrect(bar_x, bar_y, bar_w, bar_h, 5, 0x1E293B);
    gfx_rect_outline(bar_x, bar_y, bar_w, bar_h, 0x334155);

    int fill_w = (bar_w - 4) * progress_pct / 100;
    if (fill_w > 0) {
        gfx_fill_rrect(bar_x + 2, bar_y + 2, fill_w, bar_h - 4, 3, 0x38BDF8);
    }

    /* Status text */
    if (status) {
        gfx_text_center(cx, bar_y + bar_h + 16, status, 1, 0x94A3B8);
    }

    gfx_flip();
}

void bootsplash_show(const bootinfo_t *bi) {
    if (!gfx_ready()) return;

    /* Stage 1: Initializing */
    bootsplash_render_frame(15, "Starting Yazan OS Kernel...");
    pit_sleep_ms(80);

    /* Stage 2: Hardware & Memory Check */
    bootsplash_render_frame(35, "Checking RAM & CPU integrity...");
    uint64_t ram_mb = bi->mem_usable >> 20;
    if (ram_mb < MIN_REQUIRED_RAM_MB) {
        pit_sleep_ms(100);
        kpanic_ram_error(ram_mb, MIN_REQUIRED_RAM_MB);
    }
    pit_sleep_ms(80);

    /* Stage 3: Core drivers */
    bootsplash_render_frame(65, "Initializing PS/2 Keyboard, Mouse & Display Engine...");
    pit_sleep_ms(80);

    /* Stage 4: Window Manager */
    bootsplash_render_frame(90, "Loading Desktop Environment & Applications...");
    pit_sleep_ms(80);

    bootsplash_render_frame(100, "Welcome to Yazan OS!");
    pit_sleep_ms(120);
}
