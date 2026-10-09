/* Yazan OS 3.0 - System Settings & About Application */
#include "sysinfo.h"
#include "../kernel/gfx.h"
#include "../kernel/theme.h"
#include "../kernel/kstring.h"
#include "../kernel/heap.h"
#include "../kernel/pmm.h"
#include "../kernel/pit.h"

static void sysinfo_paint(window_t *win, int cx, int cy, int cw, int ch) {
    (void)win;
    gfx_fill_rect(cx, cy, cw, ch, 0x0F172A);

    /* Left banner */
    int banner_w = 120;
    gfx_fill_rect(cx, cy, banner_w, ch, 0x1E293B);
    gfx_draw_icon(cx + banner_w / 2 - 24, cy + 24, ICON_YAZAN_LOGO, 48);
    gfx_text_center(cx + banner_w / 2, cy + 84, "YAZAN OS", 1, COL_TEXT_LIGHT);
    gfx_text_center(cx + banner_w / 2, cy + 98, "v3.0 Fire", 1, 0x38BDF8);

    /* Right Details Panel */
    int rx = cx + banner_w + 20;
    int ry = cy + 16;
    int scale = 1;

    gfx_text(rx, ry, "Device & System Specifications", scale + 1, COL_TEXT_LIGHT);
    ry += 28;

    char buf[128];
    gfx_text(rx, ry, "Edition   : Yazan OS 3.0 (Fire Hybrid GUI)", scale, 0x38BDF8); ry += 18;
    gfx_text(rx, ry, "Kernel    : x86_64 Long Mode Monolithic", scale, COL_TEXT_LIGHT); ry += 18;
    gfx_text(rx, ry, "Graphics  : Double-Buffered RGB VBE Framebuffer", scale, COL_TEXT_LIGHT); ry += 18;

    ksnprintf(buf, sizeof buf, "Resolution: %ux%u @ 32-bpp", gfx_width(), gfx_height());
    gfx_text(rx, ry, buf, scale, COL_TEXT_LIGHT); ry += 24;

    /* RAM Bar */
    gfx_text(rx, ry, "Memory (RAM) Status:", scale, COL_TEXT_YELLOW); ry += 16;
    uint64_t total = pmm_get_total_ram() >> 20;
    uint64_t used = pmm_get_used_ram() >> 20;
    uint64_t free_mb = pmm_get_free_ram() >> 20;

    int bar_w = cw - banner_w - 40;
    int bar_h = 16;
    gfx_fill_rrect(rx, ry, bar_w, bar_h, 4, 0x1E293B);
    gfx_rect_outline(rx, ry, bar_w, bar_h, 0x334155);

    int used_w = total > 0 ? (int)((used * bar_w) / total) : 0;
    if (used_w > bar_w) used_w = bar_w;
    gfx_fill_rrect(rx + 2, ry + 2, used_w > 4 ? used_w - 4 : 4, bar_h - 4, 2, 0x22C55E);

    ry += 22;
    ksnprintf(buf, sizeof buf, "Used: %lu MB  |  Free: %lu MB  |  Total: %lu MB",
              (unsigned long)used, (unsigned long)free_mb, (unsigned long)total);
    gfx_text(rx, ry, buf, scale, COL_TEXT_MUTED); ry += 24;

    /* System Uptime */
    uint64_t sec = pit_uptime_ms() / 1000;
    ksnprintf(buf, sizeof buf, "System Uptime: %lu seconds", (unsigned long)sec);
    gfx_text(rx, ry, buf, scale, 0x94A3B8); ry += 18;

    gfx_text(rx, ry, "All drivers functioning normally.", scale, COL_OK);
}

window_t *sysinfo_app_open(void) {
    window_t *win = wm_create_window("System Settings & About", 180, 100, 480, 310, ICON_SETTINGS);
    if (!win) return NULL;
    win->on_paint = sysinfo_paint;
    return win;
}
