#pragma once

#include <stdint.h>

void pit_init(uint32_t hz);
void pit_irq_handler(void);

uint32_t pit_get_ticks(void);
uint32_t pit_get_hz(void);
