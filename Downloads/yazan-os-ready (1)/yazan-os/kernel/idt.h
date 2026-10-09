#pragma once
#include "regs.h"

void idt_init(void);
void exception_handler(regs_t *r);
void irq_handler(regs_t *r);
extern volatile int idt_bp_hits;
