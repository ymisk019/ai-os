/* Yazan OS 3.0 - Desktop Shell (Windows + macOS + Android Hybrid UI)
 * Features Top Status Bar, Windows 11 Taskbar, Start Menu, Desktop Icons, and Window Manager. */
#include "desktop.h"
#include "wm.h"
#include "../kernel/gfx.h"
#include "../kernel/theme.h"
#include "../kernel/kstring.h"
#include "../kernel/pit.h"
#include "../kernel/pmm.h"
#include "../kernel/panic.h"
#include "../kernel/io.h"

/* Apps */
#include "../apps/terminal.h"
#include "../apps/calculator.h"
#include "../apps/file_explorer.h"
#include "../apps/notepad.h"
#include "../apps/sysinfo.h"
#include "../apps/taskmgr.h"
#include "../apps/brickbreak.h"

#define TOPBAR_HEIGHT  28
#define TASKBAR_HEIGHT 44

static int screen_w = 1024;
static int screen_h = 768;
static int start_menu_open = 0;

typedef struct {
    const char *title;
    int icon;
    int x, y;
} desktop_icon_t;

static desktop_icon_t d_icons[] = {
    { "Terminal",   ICON_TERMINAL,   30, 48 },
    { "Calculator", ICON_CALCULATOR, 30, 140 },
    { "Files",      ICON_EXPLORER,   30, 232 },
    { "Notepad",    ICON_NOTEPAD,    30, 324 },
    { "Settings",   ICON_SETTINGS,   30, 416 },
    { "Task Mgr",   ICON_TASKMGR,    30, 508 },
    { "Brick Rush", ICON_BRICKS,     30, 600 },
};
#define NUM_DESKTOP_ICONS 7

void desktop_init(int sw, int sh) {
    screen_w = sw;
    screen_h = sh;
    start_menu_open = 0;

    wm_init(sw, sh);

    /* Open default apps for a vibrant initial experience */
    terminal_app_open();
    sysinfo_app_open();
}

void desktop_toggle_start_menu(void) {
    start_menu_open = !start_menu_open;
}

int desktop_is_start_menu_open(void) {
    return start_menu_open;
}

static void launch_app(int icon_type) {
    switch (icon_type) {
    case ICON_TERMINAL:   terminal_app_open(); break;
    case ICON_CALCULATOR: calculator_app_open(); break;
    case ICON_EXPLORER:   file_explorer_app_open(); break;
    case ICON_NOTEPAD:    notepad_app_open(); break;
    case ICON_SETTINGS:   sysinfo_app_open(); break;
    case ICON_TASKMGR:    taskmgr_app_open(); break;
    case ICON_BRICKS:     brickbreak_app_open(); break;
    default: break;
    }
}

void desktop_handle_mouse(int mx, int my, int btns, int clicked) {
    int taskbar_y = screen_h - TASKBAR_HEIGHT;

    /* Check Start Menu interaction */
    if (start_menu_open) {
        int sm_w = 360, sm_h = 420;
        int sm_x = 16, sm_y = taskbar_y - sm_h - 8;

        if (mx >= sm_x && mx < sm_x + sm_w && my >= sm_y && my < sm_y + sm_h) {
            if (clicked) {
                /* App grid inside start menu */
                int grid_y = sm_y + 110;
                int item_h = 36;
                for (int i = 0; i < NUM_DESKTOP_ICONS; i++) {
                    int iy = grid_y + i * item_h;
                    if (my >= iy && my < iy + item_h) {
                        launch_app(d_icons[i].icon);
                        start_menu_open = 0;
                        return;
                    }
                }

                /* Test Panic button at bottom */
                int panic_btn_y = sm_y + sm_h - 40;
                if (mx >= sm_x + 16 && mx < sm_x + 130 && my >= panic_btn_y && my < panic_btn_y + 28) {
                    kpanic("Test Panic Screen triggered by user from Start Menu");
                }

                /* Restart button */
                if (mx >= sm_x + sm_w - 90 && mx < sm_x + sm_w - 16 && my >= panic_btn_y && my < panic_btn_y + 28) {
                    outb(0x64, 0xFE);
                    cpu_halt_forever();
                }
            }
            return;
        } else {
            /* Clicked outside start menu */
            if (clicked && my < taskbar_y) {
                start_menu_open = 0;
            }
        }
    }

    /* Check Top Bar clicks */
    if (my < TOPBAR_HEIGHT) {
        if (clicked && mx < 80) {
            desktop_toggle_start_menu();
        }
        return;
    }

    /* Check Taskbar clicks */
    if (my >= taskbar_y) {
        if (clicked) {
            /* Start button */
            if (mx >= 16 && mx < 130) {
                desktop_toggle_start_menu();
                return;
            }

            /* Running windows in taskbar */
            int start_items_x = 140;
            int item_w = 110;
            int count = wm_get_window_count();
            for (int i = 0; i < count; i++) {
                int ix = start_items_x + i * (item_w + 6);
                if (mx >= ix && mx < ix + item_w) {
                    window_t *w = wm_get_window(i);
                    if (w) {
                        if (w->is_active && !w->is_minimized) {
                            wm_toggle_minimize(w);
                        } else {
                            wm_focus_window(w);
                        }
                    }
                    return;
                }
            }
        }
        return;
    }

    /* Check Desktop Icons clicks (if not intercepted by a window) */
    window_t *active = wm_get_active_window();
    int hit_window = 0;
    if (active && mx >= active->x && mx < active->x + active->w &&
        my >= active->y && my < active->y + active->h) {
        hit_window = 1;
    }

    if (!hit_window && clicked) {
        for (int i = 0; i < NUM_DESKTOP_ICONS; i++) {
            int ix = d_icons[i].x;
            int iy = d_icons[i].y;
            if (mx >= ix && mx < ix + 64 && my >= iy && my < iy + 72) {
                launch_app(d_icons[i].icon);
                return;
            }
        }
    }

    /* Forward to Window Manager */
    wm_handle_mouse(mx, my, btns, clicked);
}

static void draw_wallpaper(void) {
    /* Rich Cosmic / Deep Ocean gradient */
    gfx_vgradient(0, 0, screen_w, screen_h / 2, WALLPAPER_TOP, WALLPAPER_MID);
    gfx_vgradient(0, screen_h / 2, screen_w, screen_h - screen_h / 2, WALLPAPER_MID, WALLPAPER_BOTTOM);

    /* Subtle geometric ambient lighting circles / accents */
    for (int i = 0; i < screen_w; i += 64) {
        gfx_pixel(i, screen_h / 3, 0x1E293B);
    }
}

static void draw_top_bar(void) {
    /* macOS / Android style sleek top bar */
    gfx_fill_rect(0, 0, screen_w, TOPBAR_HEIGHT, TOPBAR_BG);
    gfx_fill_rect(0, TOPBAR_HEIGHT - 1, screen_w, 1, 0x1E293B);

    /* Yazan OS badge */
    gfx_draw_icon(8, 4, ICON_YAZAN_LOGO, 20);
    gfx_text(34, 9, "Yazan OS", 1, COL_TEXT_LIGHT);

    /* Active window title */
    window_t *active = wm_get_active_window();
    if (active) {
        gfx_text(110, 9, "|", 1, COL_TEXT_DIM);
        gfx_text(124, 9, active->title, 1, 0x38BDF8);
    }

    /* Center: System Time */
    char time_str[32];
    uint64_t total_sec = pit_uptime_ms() / 1000;
    int hrs = (int)((total_sec / 3600) % 24);
    int mins = (int)((total_sec / 60) % 60);
    int secs = (int)(total_sec % 60);
    ksnprintf(time_str, sizeof time_str, "%02d:%02d:%02d", hrs, mins, secs);
    gfx_text_center(screen_w / 2, 9, time_str, 1, COL_TEXT_LIGHT);

    /* Right: RAM & System Status */
    char ram_str[48];
    uint64_t usable_mb = pmm_get_usable_ram() >> 20;
    uint64_t used_mb = pmm_get_used_ram() >> 20;
    ksnprintf(ram_str, sizeof ram_str, "RAM: %luM/%luM", (unsigned long)used_mb, (unsigned long)usable_mb);
    int ram_w = gfx_text_width(ram_str, 1);
    gfx_text(screen_w - ram_w - 70, 9, ram_str, 1, 0x38BDF8);

    /* Status indicator dot */
    gfx_fill_rrect(screen_w - 50, 9, 8, 8, 4, COL_OK);
    gfx_text(screen_w - 36, 9, "OK", 1, COL_OK);
}

static void draw_desktop_icons(void) {
    for (int i = 0; i < NUM_DESKTOP_ICONS; i++) {
        int x = d_icons[i].x;
        int y = d_icons[i].y;

        /* Icon backdrop tile with rounded corners */
        gfx_blend_rect(x, y, 54, 54, 0x1E293B, 180);
        gfx_draw_icon(x + 11, y + 11, d_icons[i].icon, 32);

        /* Icon text label with shadow */
        gfx_text_center(x + 27 + 1, y + 58 + 1, d_icons[i].title, 1, 0x05070F);
        gfx_text_center(x + 27, y + 58, d_icons[i].title, 1, COL_TEXT_LIGHT);
    }
}

static void draw_taskbar(void) {
    int ty = screen_h - TASKBAR_HEIGHT;

    /* Glass effect translucent taskbar */
    gfx_blend_rect(0, ty, screen_w, TASKBAR_HEIGHT, TASKBAR_BG, 240);
    gfx_fill_rect(0, ty, screen_w, 1, TASKBAR_BORDER);

    /* Start Button (Windows 11 style) */
    rgb_t start_col = start_menu_open ? 0x1D4ED8 : START_BTN_COLOR;
    gfx_fill_rrect(14, ty + 6, 110, 32, 6, start_col);
    gfx_draw_icon(22, ty + 12, ICON_YAZAN_LOGO, 20);
    gfx_text(48, ty + 17, "Start", 1, COL_WHITE);

    /* Running window tabs */
    int start_items_x = 136;
    int item_w = 120;
    int count = wm_get_window_count();

    for (int i = 0; i < count; i++) {
        window_t *w = wm_get_window(i);
        if (!w) continue;

        int ix = start_items_x + i * (item_w + 6);
        rgb_t item_bg = w->is_active ? TASKBAR_ITEM_ACTIVE : TASKBAR_ITEM_BG;

        gfx_fill_rrect(ix, ty + 6, item_w, 32, 6, item_bg);
        gfx_draw_icon(ix + 6, ty + 12, w->icon_type, 18);

        char title_clip[14];
        strncpy(title_clip, w->title, 10);
        title_clip[10] = '\0';
        gfx_text(ix + 30, ty + 17, title_clip, 1, COL_TEXT_LIGHT);

        /* Active underline */
        if (w->is_active) {
            gfx_fill_rect(ix + 10, ty + 34, item_w - 20, 2, 0x38BDF8);
        }
    }

    /* Right quick action widgets */
    gfx_text(screen_w - 110, ty + 17, "Yazan OS", 1, COL_TEXT_MUTED);
}

static void draw_start_menu(void) {
    if (!start_menu_open) return;

    int sm_w = 340, sm_h = 390;
    int sm_x = 14, sm_y = screen_h - TASKBAR_HEIGHT - sm_h - 8;

    /* Shadow */
    gfx_blend_rect(sm_x + 6, sm_y + 6, sm_w, sm_h, 0x020617, 160);

    /* Menu container */
    gfx_fill_rrect(sm_x, sm_y, sm_w, sm_h, 10, START_MENU_BG);
    gfx_rect_outline(sm_x, sm_y, sm_w, sm_h, START_MENU_BORDER);

    /* User Profile Card */
    gfx_fill_rrect(sm_x + 12, sm_y + 12, sm_w - 24, 46, 6, START_MENU_CARD);
    gfx_draw_icon(sm_x + 20, sm_y + 19, ICON_YAZAN_LOGO, 32);
    gfx_text(sm_x + 62, sm_y + 20, "Yazan Administrator", 1, COL_TEXT_LIGHT);
    gfx_text(sm_x + 62, sm_y + 36, "yazan@os.local  |  Root", 1, COL_TEXT_MUTED);

    /* Search Bar Mock */
    gfx_fill_rrect(sm_x + 12, sm_y + 66, sm_w - 24, 28, 4, 0x1E293B);
    gfx_text(sm_x + 24, sm_y + 74, "Search apps & files...", 1, COL_TEXT_DIM);

    /* Section Title */
    gfx_text(sm_x + 16, sm_y + 104, "ALL APPLICATIONS", 1, 0x38BDF8);

    /* Apps list */
    int item_y = sm_y + 122;
    int item_h = 34;

    for (int i = 0; i < NUM_DESKTOP_ICONS; i++) {
        gfx_fill_rrect(sm_x + 12, item_y, sm_w - 24, 30, 4, 0x162032);
        gfx_draw_icon(sm_x + 18, item_y + 6, d_icons[i].icon, 18);
        gfx_text(sm_x + 46, item_y + 11, d_icons[i].title, 1, COL_TEXT_LIGHT);
        item_y += item_h;
    }

    /* Bottom Actions: Panic Test & Restart */
    int btn_y = sm_y + sm_h - 40;
    gfx_fill_rrect(sm_x + 12, btn_y, 130, 28, 4, 0x7F1D1D);
    gfx_text(sm_x + 20, btn_y + 9, "Test Panic RSOD", 1, 0xFCA5A5);

    gfx_fill_rrect(sm_x + sm_w - 110, btn_y, 98, 28, 4, 0x1E293B);
    gfx_draw_icon(sm_x + sm_w - 104, btn_y + 6, ICON_POWER, 16);
    gfx_text(sm_x + sm_w - 82, btn_y + 9, "Restart", 1, COL_TEXT_LIGHT);
}

void desktop_render(void) {
    draw_wallpaper();
    draw_desktop_icons();
    wm_render();
    draw_top_bar();
    draw_taskbar();
    draw_start_menu();
}
