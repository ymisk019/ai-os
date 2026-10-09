#pragma once
#include <stdint.h>

void     pit_init(uint32_t frequency);
void     pit_handler(void);
uint64_t pit_get_ticks(void);
uint64_t pit_uptime_ms(void);
void     pit_sleep_ms(uint32_t ms);
