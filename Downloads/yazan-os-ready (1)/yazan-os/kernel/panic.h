#pragma once
#include <stdint.h>
#include "regs.h"

void kpanic(const char *fmt, ...) __attribute__((noreturn, format(printf, 1, 2)));
void kpanic_exception(const regs_t *r) __attribute__((noreturn));
void kpanic_ram_error(uint64_t detected_mb, uint64_t required_mb) __attribute__((noreturn));
void panic_exception_screen(const regs_t *r, uint64_t cr2, uint64_t cr3);
