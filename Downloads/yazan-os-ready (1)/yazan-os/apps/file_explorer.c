/* Yazan OS 3.0 - Graphical File Explorer Application */
#include "file_explorer.h"
#include "../kernel/gfx.h"
#include "../kernel/theme.h"
#include "../kernel/kstring.h"
#include "../kernel/heap.h"

typedef struct {
    const char *name;
    const char *type;
    const char *size;
    int is_dir;
} file_item_t;

static const file_item_t demo_files[] = {
    { "system",       "Directory", "--",      1 },
    { "apps",         "Directory", "--",      1 },
    { "home",         "Directory", "--",      1 },
    { "boot.cfg",     "Config",    "1.2 KB",  0 },
    { "kernel.elf",   "Binary",    "184 KB",  0 },
    { "yazan_art.png","Image",     "42 KB",   0 },
    { "notes.txt",    "Text",      "512 B",   0 },
};
#define NUM_DEMO_FILES 7

typedef struct {
    int selected_idx;
} explorer_state_t;

static void explorer_paint(window_t *win, int cx, int cy, int cw, int ch) {
    explorer_state_t *st = (explorer_state_t *)win->user_data;
    gfx_fill_rect(cx, cy, cw, ch, 0x0F172A);

    /* Address bar */
    gfx_fill_rrect(cx + 8, cy + 8, cw - 16, 26, 4, 0x1E293B);
    gfx_text(cx + 16, cy + 16, "Path: /system/yazan_os/", 1, 0x38BDF8);

    /* Table headers */
    int ty = cy + 42;
    gfx_fill_rect(cx + 8, ty, cw - 16, 20, 0x1E293B);
    gfx_text(cx + 20, ty + 6, "Name", 1, COL_TEXT_MUTED);
    gfx_text(cx + 200, ty + 6, "Type", 1, COL_TEXT_MUTED);
    gfx_text(cx + 310, ty + 6, "Size", 1, COL_TEXT_MUTED);

    /* Rows */
    int row_y = ty + 24;
    for (int i = 0; i < NUM_DEMO_FILES; i++) {
        rgb_t row_bg = (i == st->selected_idx) ? 0x1D4ED8 : ((i % 2 == 0) ? 0x111827 : 0x0F172A);
        gfx_fill_rect(cx + 8, row_y, cw - 16, 22, row_bg);

        /* Icon */
        gfx_draw_icon(cx + 14, row_y + 3, demo_files[i].is_dir ? ICON_EXPLORER : ICON_NOTEPAD, 16);

        /* Name */
        gfx_text(cx + 36, row_y + 6, demo_files[i].name, 1, COL_TEXT_LIGHT);
        /* Type */
        gfx_text(cx + 200, row_y + 6, demo_files[i].type, 1, COL_TEXT_MUTED);
        /* Size */
        gfx_text(cx + 310, row_y + 6, demo_files[i].size, 1, COL_TEXT_MUTED);

        row_y += 24;
    }

    /* Bottom status */
    gfx_fill_rect(cx, cy + ch - 22, cw, 22, 0x1E293B);
    char sbuf[64];
    ksnprintf(sbuf, sizeof sbuf, "%d items | Selected: %s",
              NUM_DEMO_FILES, demo_files[st->selected_idx].name);
    gfx_text(cx + 12, cy + ch - 16, sbuf, 1, COL_TEXT_MUTED);
}

static void explorer_mouse(window_t *win, int rx, int ry, int btns, int clicked) {
    (void)btns;
    if (!clicked) return;
    explorer_state_t *st = (explorer_state_t *)win->user_data;

    int ty = 42 + 24;
    for (int i = 0; i < NUM_DEMO_FILES; i++) {
        if (rx >= 8 && rx < win->w - 16 && ry >= ty + i * 24 && ry < ty + (i + 1) * 24) {
            st->selected_idx = i;
            return;
        }
    }
}

window_t *file_explorer_app_open(void) {
    window_t *win = wm_create_window("File Explorer - /", 120, 80, 440, 310, ICON_EXPLORER);
    if (!win) return NULL;

    explorer_state_t *st = (explorer_state_t *)kmalloc(sizeof(explorer_state_t));
    if (!st) return NULL;
    memset(st, 0, sizeof(explorer_state_t));

    win->user_data = st;
    win->on_paint = explorer_paint;
    win->on_mouse = explorer_mouse;
    return win;
}
