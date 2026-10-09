#pragma once
#include <stdint.h>

void desktop_init(int screen_w, int screen_h);
void desktop_handle_mouse(int mx, int my, int btns, int clicked);
void desktop_render(void);
void desktop_toggle_start_menu(void);
int  desktop_is_start_menu_open(void);
