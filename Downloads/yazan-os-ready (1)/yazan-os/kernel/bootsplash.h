#pragma once
#include "bootinfo.h"

void bootsplash_show(const bootinfo_t *bi);
void bootsplash_render_frame(int progress_pct, const char *status);
