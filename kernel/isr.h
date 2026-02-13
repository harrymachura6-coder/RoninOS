#pragma once
#include <stdint.h>

void isr_install(void);

extern void irq0_stub(void);
extern void irq1_stub(void);

void irq0_handler_c(void);
void irq1_handler_c(void);
