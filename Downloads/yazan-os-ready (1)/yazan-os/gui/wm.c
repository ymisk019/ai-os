#include "wm.h"
#include "gfx.h"
#include "theme.h"
#include "heap.h"
#include "kstring.h"
#include "serial.h"

static window_t *windows[MAX_WINDOWS];
static int num_windows = 0;
static window_t *active_window = NULL;

static int is_dragging = 0;
static window_t *drag_win = NULL;
static int drag_off_x = 0;
static int drag_off_y = 0;
static int scr_w = 1024;
static int scr_h = 768;

void wm_init(int screen_w, int screen_h) {
    scr_w = screen_w;
    scr_h = screen_h;
    num_windows = 0;
    active_window = NULL;
    is_dragging = 0;
    drag_win = NULL;
    for (int i = 0; i < MAX_WINDOWS; i++) windows[i] = NULL;
    klog("[wm] Window manager initialized (%dx%d)\n", screen_w, screen_h);
}

window_t *wm_create_window(const char *title, int x, int y, int w, int h, int icon) {
    if (num_windows >= MAX_WINDOWS) return NULL;

    window_t *win = (window_t *)kmalloc(sizeof(window_t));
    if (!win) return NULL;

    memset(win, 0, sizeof(window_t));
    win->id = num_windows + 1;
    strncpy(win->title, title, sizeof(win->title) - 1);
    win->x = x;
    win->y = y;
    win->w = w;
    win->h = h;
    win->orig_x = x;
    win->orig_y = y;
    win->orig_w = w;
    win->orig_h = h;
    win->icon_type = icon;

    windows[num_windows++] = win;
    wm_focus_window(win);
    return win;
}

void wm_close_window(window_t *win) {
    if (!win) return;
    win->is_closed = 1;

    /* Remove from list */
    int idx = -1;
    for (int i = 0; i < num_windows; i++) {
        if (windows[i] == win) {
            idx = i;
            break;
        }
    }

    if (idx != -1) {
        for (int i = idx; i < num_windows - 1; i++) {
            windows[i] = windows[i + 1];
        }
        windows[--num_windows] = NULL;
    }

    if (active_window == win) {
        active_window = num_windows > 0 ? windows[num_windows - 1] : NULL;
        if (active_window) active_window->is_active = 1;
    }

    kfree(win);
}

void wm_focus_window(window_t *win) {
    if (!win || win->is_closed) return;

    /* Bring to front in array */
    int idx = -1;
    for (int i = 0; i < num_windows; i++) {
        if (windows[i] == win) { idx = i; break; }
    }

    if (idx != -1 && idx != num_windows - 1) {
        for (int i = idx; i < num_windows - 1; i++) {
            windows[i] = windows[i + 1];
        }
        windows[num_windows - 1] = win;
    }

    for (int i = 0; i < num_windows; i++) {
        windows[i]->is_active = (windows[i] == win);
    }
    active_window = win;
    win->is_minimized = 0;
}

void wm_toggle_minimize(window_t *win) {
    if (!win) return;
    win->is_minimized = !win->is_minimized;
    if (win->is_minimized && active_window == win) {
        active_window = NULL;
        for (int i = num_windows - 1; i >= 0; i--) {
            if (!windows[i]->is_minimized) {
                wm_focus_window(windows[i]);
                break;
            }
        }
    }
}

void wm_toggle_maximize(window_t *win) {
    if (!win) return;
    if (win->is_maximized) {
        win->x = win->orig_x;
        win->y = win->orig_y;
        win->w = win->orig_w;
        win->h = win->orig_h;
        win->is_maximized = 0;
    } else {
        win->orig_x = win->x;
        win->orig_y = win->y;
        win->orig_w = win->w;
        win->orig_h = win->h;
        win->x = 0;
        win->y = 28; /* below top bar */
        win->w = scr_w;
        win->h = scr_h - 28 - 48; /* above taskbar */
        win->is_maximized = 1;
    }
}

window_t *wm_get_active_window(void) { return active_window; }
window_t *wm_get_window(int idx) {
    if (idx >= 0 && idx < num_windows) return windows[idx];
    return NULL;
}
int wm_get_window_count(void) { return num_windows; }

void wm_handle_mouse(int mx, int my, int btns, int clicked) {
    if (is_dragging && drag_win) {
        if (btns & 1) {
            drag_win->x = mx - drag_off_x;
            drag_win->y = my - drag_off_y;
            if (drag_win->y < 28) drag_win->y = 28;
            return;
        } else {
            is_dragging = 0;
            drag_win = NULL;
        }
    }

    /* Hit test windows in reverse Z-order (top to bottom) */
    for (int i = num_windows - 1; i >= 0; i--) {
        window_t *win = windows[i];
        if (!win || win->is_minimized || win->is_closed) continue;

        if (mx >= win->x && mx < win->x + win->w &&
            my >= win->y && my < win->y + win->h) {

            if (clicked) {
                wm_focus_window(win);
            }

            /* Inside Titlebar? */
            if (my < win->y + TITLEBAR_HEIGHT) {
                int btn_y = win->y + 7;
                /* Close button */
                int close_x = win->x + win->w - 22;
                if (mx >= close_x && mx < close_x + 14 && my >= btn_y && my < btn_y + 14) {
                    if (clicked) { wm_close_window(win); return; }
                }
                /* Maximize button */
                int max_x = close_x - 18;
                if (mx >= max_x && mx < max_x + 14 && my >= btn_y && my < btn_y + 14) {
                    if (clicked) { wm_toggle_maximize(win); return; }
                }
                /* Minimize button */
                int min_x = max_x - 18;
                if (mx >= min_x && mx < min_x + 14 && my >= btn_y && my < btn_y + 14) {
                    if (clicked) { wm_toggle_minimize(win); return; }
                }

                /* Drag start */
                if (clicked && !win->is_maximized) {
                    is_dragging = 1;
                    drag_win = win;
                    drag_off_x = mx - win->x;
                    drag_off_y = my - win->y;
                }
                return;
            }

            /* Inside client area */
            if (win->on_mouse) {
                int rx = mx - win->x;
                int ry = my - (win->y + TITLEBAR_HEIGHT);
                win->on_mouse(win, rx, ry, btns, clicked);
            }
            return;
        }
    }
}

void wm_handle_key(char key) {
    if (active_window && active_window->on_key && !active_window->is_minimized) {
        active_window->on_key(active_window, key);
    }
}

static void draw_window_frame(window_t *win) {
    int x = win->x, y = win->y, w = win->w, h = win->h;

    /* Shadow */
    gfx_blend_rect(x + 4, y + 4, w, h, 0x05070F, 140);

    /* Window border & background */
    rgb_t title_bg = win->is_active ? WIN_TITLE_ACTIVE : WIN_TITLE_INACTIVE;
    rgb_t border_col = win->is_active ? WIN_BORDER_ACTIVE : WIN_BORDER_INACTIVE;

    gfx_fill_rrect(x, y, w, h, 8, WIN_CONTENT_BG);
    gfx_fill_rrect(x, y, w, TITLEBAR_HEIGHT, 8, title_bg);
    gfx_fill_rect(x, y + TITLEBAR_HEIGHT - 4, w, 4, title_bg); /* square bottom of titlebar */
    gfx_rect_outline(x, y, w, h, border_col);

    /* Titlebar Icon */
    gfx_draw_icon(x + 8, y + 6, win->icon_type, 16);

    /* Title text */
    rgb_t text_col = win->is_active ? COL_TEXT_LIGHT : COL_TEXT_MUTED;
    gfx_text(x + 30, y + 10, win->title, 1, text_col);

    /* Control buttons (macOS style traffic lights) */
    int btn_y = y + 8;
    int close_x = x + w - 22;
    int max_x = close_x - 18;
    int min_x = max_x - 18;

    gfx_fill_rrect(close_x, btn_y, 12, 12, 6, WIN_BTN_CLOSE);
    gfx_fill_rrect(max_x, btn_y, 12, 12, 6, WIN_BTN_MAX);
    gfx_fill_rrect(min_x, btn_y, 12, 12, 6, WIN_BTN_MIN);

    /* Client area */
    int cx = x + 1;
    int cy = y + TITLEBAR_HEIGHT;
    int cw = w - 2;
    int ch = h - TITLEBAR_HEIGHT - 1;

    if (win->on_paint) {
        win->on_paint(win, cx, cy, cw, ch);
    }
}

void wm_render(void) {
    for (int i = 0; i < num_windows; i++) {
        window_t *win = windows[i];
        if (win && !win->is_minimized && !win->is_closed) {
            draw_window_frame(win);
        }
    }
}
