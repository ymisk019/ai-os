/* Yazan OS 3.0 - Graphical Calculator Application */
#include "calculator.h"
#include "../kernel/gfx.h"
#include "../kernel/theme.h"
#include "../kernel/kstring.h"
#include "../kernel/heap.h"

typedef struct {
    char display[32];
    long op1;
    long op2;
    char op;
    int  new_num;
} calc_state_t;

static const char *calc_grid[4][4] = {
    { "7", "8", "9", "/" },
    { "4", "5", "6", "*" },
    { "1", "2", "3", "-" },
    { "C", "0", "=", "+" }
};

static void calc_press(calc_state_t *st, const char *key) {
    if (key[0] >= '0' && key[0] <= '9') {
        if (st->new_num || strcmp(st->display, "0") == 0) {
            st->display[0] = key[0];
            st->display[1] = '\0';
            st->new_num = 0;
        } else {
            size_t len = strlen(st->display);
            if (len < 12) {
                st->display[len] = key[0];
                st->display[len + 1] = '\0';
            }
        }
    } else if (strcmp(key, "C") == 0) {
        strcpy(st->display, "0");
        st->op1 = 0;
        st->op2 = 0;
        st->op = 0;
        st->new_num = 1;
    } else if (strchr("+-*/", key[0])) {
        /* Parse integer from display */
        long v = 0;
        const char *p = st->display;
        while (*p >= '0' && *p <= '9') { v = v * 10 + (*p - '0'); p++; }
        st->op1 = v;
        st->op = key[0];
        st->new_num = 1;
    } else if (strcmp(key, "=") == 0 && st->op) {
        long v = 0;
        const char *p = st->display;
        while (*p >= '0' && *p <= '9') { v = v * 10 + (*p - '0'); p++; }
        st->op2 = v;

        long res = 0;
        if (st->op == '+') res = st->op1 + st->op2;
        else if (st->op == '-') res = st->op1 - st->op2;
        else if (st->op == '*') res = st->op1 * st->op2;
        else if (st->op == '/' && st->op2 != 0) res = st->op1 / st->op2;

        ksnprintf(st->display, sizeof st->display, "%ld", res);
        st->op = 0;
        st->new_num = 1;
    }
}

static void calc_paint(window_t *win, int cx, int cy, int cw, int ch) {
    calc_state_t *st = (calc_state_t *)win->user_data;
    gfx_fill_rect(cx, cy, cw, ch, 0x111827);

    /* LCD Screen Display */
    int disp_h = 44;
    gfx_fill_rrect(cx + 10, cy + 10, cw - 20, disp_h, 6, 0x1F2937);
    gfx_rect_outline(cx + 10, cy + 10, cw - 20, disp_h, 0x374151);

    /* Right-aligned text */
    int text_w = gfx_text_width(st->display, 2);
    gfx_text(cx + cw - 24 - text_w, cy + 22, st->display, 2, 0x38BDF8);

    /* Button Grid */
    int start_y = cy + 64;
    int bw = (cw - 40) / 4;
    int bh = (ch - 74) / 4;

    for (int r = 0; r < 4; r++) {
        for (int c = 0; c < 4; c++) {
            int bx = cx + 10 + c * (bw + 6);
            int by = start_y + r * (bh + 4);
            const char *label = calc_grid[r][c];

            rgb_t btn_col = 0x1F2937;
            rgb_t txt_col = COL_TEXT_LIGHT;

            if (strchr("+-*/=", label[0])) {
                btn_col = 0x2563EB;
                txt_col = COL_WHITE;
            } else if (label[0] == 'C') {
                btn_col = 0xDC2626;
                txt_col = COL_WHITE;
            }

            gfx_fill_rrect(bx, by, bw, bh, 6, btn_col);
            gfx_rect_outline(bx, by, bw, bh, 0x374151);
            gfx_text_center(bx + bw / 2, by + bh / 2 - 4, label, 1, txt_col);
        }
    }
}

static void calc_mouse(window_t *win, int rx, int ry, int btns, int clicked) {
    (void)btns;
    if (!clicked) return;
    calc_state_t *st = (calc_state_t *)win->user_data;

    int cw = win->w - 2;
    int ch = win->h - TITLEBAR_HEIGHT - 1;
    int start_y = 64;
    int bw = (cw - 40) / 4;
    int bh = (ch - 74) / 4;

    for (int r = 0; r < 4; r++) {
        for (int c = 0; c < 4; c++) {
            int bx = 10 + c * (bw + 6);
            int by = start_y + r * (bh + 4);
            if (rx >= bx && rx < bx + bw && ry >= by && ry < by + bh) {
                calc_press(st, calc_grid[r][c]);
                return;
            }
        }
    }
}

static void calc_key(window_t *win, char key) {
    calc_state_t *st = (calc_state_t *)win->user_data;
    char s[2] = { key, '\0' };
    if ((key >= '0' && key <= '9') || strchr("+-*/=", key)) {
        calc_press(st, s);
    } else if (key == '\n') {
        calc_press(st, "=");
    } else if (key == 'c' || key == 'C' || key == 27) {
        calc_press(st, "C");
    }
}

window_t *calculator_app_open(void) {
    window_t *win = wm_create_window("Calculator", 200, 100, 260, 320, ICON_CALCULATOR);
    if (!win) return NULL;

    calc_state_t *st = (calc_state_t *)kmalloc(sizeof(calc_state_t));
    if (!st) return NULL;
    memset(st, 0, sizeof(calc_state_t));
    strcpy(st->display, "0");
    st->new_num = 1;

    win->user_data = st;
    win->on_paint = calc_paint;
    win->on_mouse = calc_mouse;
    win->on_key = calc_key;
    return win;
}
