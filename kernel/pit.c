#include "pit.h"

#include "pic.h"
#include "ports.h"
#include "sched/thread.h"

static volatile uint32_t g_ticks = 0;
static volatile uint32_t g_hz = 100;

void pit_init(uint32_t hz) {
    uint32_t divisor;

    if (hz == 0) {
        hz = 100;
    }
    g_hz = hz;

    divisor = 1193182u / hz;
    if (divisor == 0) {
        divisor = 1;
    }

    outb(0x43, 0x36);
    outb(0x40, (uint8_t)(divisor & 0xFFu));
    outb(0x40, (uint8_t)((divisor >> 8) & 0xFFu));
}

void pit_irq_handler(void) {
    g_ticks++;
    pic_send_eoi(0);
    if (sched_is_preempt_enabled()) {
        thread_yield();
    }
}

void irq0_handler_c(void) {
    pit_irq_handler();
}

uint32_t pit_get_ticks(void) {
    return g_ticks;
}

uint32_t pit_get_hz(void) {
    return g_hz;
}
