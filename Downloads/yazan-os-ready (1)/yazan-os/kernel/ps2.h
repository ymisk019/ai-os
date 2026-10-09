#pragma once
#include <stdint.h>

#define PS2_DATA 0x60
#define PS2_CMD  0x64
#define PS2_STATUS 0x64

void    ps2_init(void);
uint8_t ps2_read(void);
void    ps2_write(uint8_t port, uint8_t val);
void    ps2_write_device(uint8_t dev, uint8_t val);
