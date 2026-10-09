/* ============================================================================
 * Yazan OS 3.0 SDK - Example Application: "Paint & Draw"
 * Demonstrates how to write custom GUI apps for Yazan OS in C.
 * ============================================================================ */

#include "../gui/wm.h"
#include "../kernel/gfx.h"
#include "../kernel/theme.h"
#include "../kernel/heap.h"
#include "../kernel/kstring.h"

typedef struct {
    rgb_t current_color;
    int   brush_size;
    int   last_x, last_y;
    int   is_drawing;
    /* Drawing canvas buffer */
    rgb_t canvas[200 * 200];
} paint_app_state_t;

/* Colors available in palette */
static const rgb_t palette[] = {
    0xFFFFFF, 0x000000, 0xEF4444, 0x22C55E, 0x3B82F6, 0xF59E0B, 0x8B5CF6
};
#define NUM_COLORS 7

/* 1. Paint Callback: Called every frame when window renders */
static void paint_app_render(window_t *win, int cx, int cy, int cw, int ch) {
    paint_app_state_t *st = (paint_app_state_t *)win->user_data;

    /* Fill background */
    gfx_fill_rect(cx, cy, cw, ch, 0x1E293B);

    /* Toolbar at top */
    gfx_fill_rect(cx, cy, cw, 36, 0x0F172A);
    gfx_text(cx + 10, cy + 12, "Palette:", 1, COL_TEXT_MUTED);

    /* Color Swatches */
    for (int i = 0; i < NUM_COLORS; i++) {
        int sx = cx + 80 + i * 26;
        int sy = cy + 8;
        gfx_fill_rrect(sx, sy, 20, 20, 4, palette[i]);
        if (palette[i] == st->current_color) {
            gfx_rect_outline(sx - 2, sy - 2, 24, 24, 0x38BDF8);
        }
    }

    /* Drawing Canvas */
    int can_x = cx + 10;
    int can_y = cy + 44;
    int can_w = cw - 20;
    int can_h = ch - 54;

    gfx_fill_rect(can_x, can_y, can_w, can_h, 0xFFFFFF);
    gfx_rect_outline(can_x, can_y, can_w, can_h, 0x334155);

    /* Render canvas pixels */
    for (int y = 0; y < 150 && y < can_h; y++) {
        for (int x = 0; x < 180 && x < can_w; x++) {
            rgb_t c = st->canvas[y * 200 + x];
            if (c != 0) {
                gfx_pixel(can_x + x, can_y + y, c);
            }
        }
    }
}

/* 2. Mouse Callback: Called when user clicks or moves mouse inside window */
static void paint_app_mouse(window_t *win, int rx, int ry, int btns, int clicked) {
    paint_app_state_t *st = (paint_app_state_t *)win->user_data;

    /* Check palette clicks */
    if (clicked && ry >= 8 && ry <= 28) {
        for (int i = 0; i < NUM_COLORS; i++) {
            int sx = 80 + i * 26;
            if (rx >= sx && rx < sx + 20) {
                st->current_color = palette[i];
                return;
            }
        }
    }

    /* Check canvas drawing */
    int can_x = 10, can_y = 44;
    if (btns & 1) {
        int px = rx - can_x;
        int py = ry - can_y;
        if (px >= 0 && px < 180 && py >= 0 && py < 150) {
            /* Draw brush stroke */
            for (int dy = -1; dy <= 1; dy++) {
                for (int dx = -1; dx <= 1; dx++) {
                    int nx = px + dx, ny = py + dy;
                    if (nx >= 0 && nx < 200 && ny >= 0 && ny < 200) {
                        st->canvas[ny * 200 + nx] = st->current_color;
                    }
                }
            }
        }
    }
}

/* 3. Keyboard Callback: Handles key presses */
static void paint_app_key(window_t *win, char key) {
    paint_app_state_t *st = (paint_app_state_t *)win->user_data;
    if (key == 'c' || key == 'C') {
        /* Clear canvas */
        memset(st->canvas, 0, sizeof(st->canvas));
    }
}

/* 4. Entry point to register & launch the App */
window_t *paint_app_open(void) {
    window_t *win = wm_create_window("Yazan Paint", 140, 90, 380, 280, ICON_NOTEPAD);
    if (!win) return NULL;

    paint_app_state_t *st = (paint_app_state_t *)kmalloc(sizeof(paint_app_state_t));
    if (!st) return NULL;
    memset(st, 0, sizeof(paint_app_state_t));
    st->current_color = 0x3B82F6; /* Default blue brush */

    win->user_data = st;
    win->on_paint = paint_app_render;
    win->on_mouse = paint_app_mouse;
    win->on_key = paint_app_key;
    return win;
}
