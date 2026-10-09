#pragma once
#include <stdint.h>

void keyboard_init(void);
void keyboard_handler(void);
int  keyboard_has_char(void);
char keyboard_getchar(void);
