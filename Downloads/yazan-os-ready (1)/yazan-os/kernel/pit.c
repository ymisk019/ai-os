#include "pit.h"
#include "io.h"
#include "pic.h"
#include "serial.h"

#define PIT_CMD  0x43
#define PIT_CH0  0x40
#define PIT_FREQ_BASE 1193182

static volatile uint64_t timer_ticks = 0;
static uint32_t timer_frequency = 100;

void pit_init(uint32_t frequency) {
    if (frequency == 0) frequency = 100;
    timer_frequency = frequency;
    uint32_t divisor = PIT_FREQ_BASE / frequency;

    outb(PIT_CMD, 0x36); /* Channel 0, lobyte/hibyte, rate generator */
    outb(PIT_CH0, (uint8_t)(divisor & 0xFF));
    outb(PIT_CH0, (uint8_t)((divisor >> 8) & 0xFF));

    pic_unmask(IRQ_TIMER);
    klog("[pit] Timer initialized at %u Hz\n", frequency);
}

void pit_handler(void) {
    timer_ticks++;
}

uint64_t pit_get_ticks(void) {
    return timer_ticks;
}

uint64_t pit_uptime_ms(void) {
    return (timer_ticks * 1000) / timer_frequency;
}

void pit_sleep_ms(uint32_t ms) {
    uint64_t target = timer_ticks + (ms * timer_frequency) / 1000;
    while (timer_ticks < target) {
        __asm__ volatile("pause");
    }
}
