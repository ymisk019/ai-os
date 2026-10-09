#pragma once
#include <stdint.h>

#define MOUSE_BTN_LEFT   (1 << 0)
#define MOUSE_BTN_RIGHT  (1 << 1)
#define MOUSE_BTN_MIDDLE (1 << 2)

void mouse_init(int screen_w, int screen_h);
void mouse_handler(void);
void mouse_get_state(int *x, int *y, int *buttons);
int  mouse_was_clicked(int button);
