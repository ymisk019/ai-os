/* Yazan OS 3.0 - Task Manager & Performance Monitor */
#include "taskmgr.h"
#include "../kernel/gfx.h"
#include "../kernel/theme.h"
#include "../kernel/kstring.h"
#include "../kernel/heap.h"
#include "../kernel/pmm.h"
#include "../kernel/pit.h"

static void taskmgr_paint(window_t *win, int cx, int cy, int cw, int ch) {
    (void)win;
    gfx_fill_rect(cx, cy, cw, ch, 0x0F172A);

    /* Tabs mock */
    gfx_fill_rect(cx, cy, cw, 28, 0x1E293B);
    gfx_fill_rrect(cx + 8, cy + 4, 90, 24, 4, 0x2563EB);
    gfx_text_center(cx + 8 + 45, cy + 10, "Processes", 1, COL_WHITE);
    gfx_text_center(cx + 150, cy + 10, "Performance", 1, COL_TEXT_MUTED);

    /* Process list header */
    int py = cy + 34;
    gfx_fill_rect(cx + 6, py, cw - 12, 20, 0x141E33);
    gfx_text(cx + 16, py + 6, "Task / Window Name", 1, COL_TEXT_MUTED);
    gfx_text(cx + 240, py + 6, "Status", 1, COL_TEXT_MUTED);
    gfx_text(cx + 330, py + 6, "Memory", 1, COL_TEXT_MUTED);

    int count = wm_get_window_count();
    py += 22;

    for (int i = 0; i < count; i++) {
        window_t *w = wm_get_window(i);
        if (!w) continue;

        rgb_t rbg = (i % 2 == 0) ? 0x111827 : 0x0F172A;
        gfx_fill_rect(cx + 6, py, cw - 12, 22, rbg);
        gfx_draw_icon(cx + 12, py + 3, w->icon_type, 16);
        gfx_text(cx + 34, py + 6, w->title, 1, COL_TEXT_LIGHT);
        gfx_text(cx + 240, py + 6, w->is_minimized ? "Minimized" : "Active", 1, w->is_minimized ? COL_WARN : COL_OK);
        gfx_text(cx + 330, py + 6, "2.4 MB", 1, COL_TEXT_MUTED);
        py += 24;
    }

    /* System Process rows */
    const char *sys_procs[] = { "kernel.core", "desktop.wm", "ps2.input", "pit.timer" };
    for (int j = 0; j < 4; j++) {
        rgb_t rbg = ((count + j) % 2 == 0) ? 0x111827 : 0x0F172A;
        gfx_fill_rect(cx + 6, py, cw - 12, 22, rbg);
        gfx_text(cx + 34, py + 6, sys_procs[j], 1, 0x94A3B8);
        gfx_text(cx + 240, py + 6, "Running", 1, COL_OK);
        gfx_text(cx + 330, py + 6, "512 KB", 1, COL_TEXT_MUTED);
        py += 24;
    }

    /* Bottom Resource Summary */
    gfx_fill_rect(cx, cy + ch - 30, cw, 30, 0x1E293B);
    char buf[128];
    ksnprintf(buf, sizeof buf, "Threads: 8 | Tasks: %d | Kernel Heap: %lu KB / %lu KB",
              count + 4,
              (unsigned long)(heap_get_used() >> 10),
              (unsigned long)(heap_get_total() >> 10));
    gfx_text(cx + 12, cy + ch - 20, buf, 1, COL_TEXT_LIGHT);
}

window_t *taskmgr_app_open(void) {
    window_t *win = wm_create_window("Task Manager", 220, 140, 440, 310, ICON_TASKMGR);
    if (!win) return NULL;
    win->on_paint = taskmgr_paint;
    return win;
}
