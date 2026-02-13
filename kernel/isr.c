#include "isr.h"
#include "idt.h"
#include "pic.h"
#include "ports.h"
#include "pit.h"
#include <stdint.h>

/* Exception gates (0..31) */
extern void isr0_stub(void);
extern void isr1_stub(void);
extern void isr2_stub(void);
extern void isr3_stub(void);
extern void isr4_stub(void);
extern void isr5_stub(void);
extern void isr6_stub(void);
extern void isr7_stub(void);
extern void isr8_stub(void);
extern void isr9_stub(void);
extern void isr10_stub(void);
extern void isr11_stub(void);
extern void isr12_stub(void);
extern void isr13_stub(void);
extern void isr14_stub(void);
extern void isr15_stub(void);
extern void isr16_stub(void);
extern void isr17_stub(void);
extern void isr18_stub(void);
extern void isr19_stub(void);
extern void isr20_stub(void);
extern void isr21_stub(void);
extern void isr22_stub(void);
extern void isr23_stub(void);
extern void isr24_stub(void);
extern void isr25_stub(void);
extern void isr26_stub(void);
extern void isr27_stub(void);
extern void isr28_stub(void);
extern void isr29_stub(void);
extern void isr30_stub(void);
extern void isr31_stub(void);

static void set_exc_gates(void) {
    void* stubs[32] = {
        isr0_stub,isr1_stub,isr2_stub,isr3_stub,isr4_stub,isr5_stub,isr6_stub,isr7_stub,
        isr8_stub,isr9_stub,isr10_stub,isr11_stub,isr12_stub,isr13_stub,isr14_stub,isr15_stub,
        isr16_stub,isr17_stub,isr18_stub,isr19_stub,isr20_stub,isr21_stub,isr22_stub,isr23_stub,
        isr24_stub,isr25_stub,isr26_stub,isr27_stub,isr28_stub,isr29_stub,isr30_stub,isr31_stub
    };
    int i;

    for (i = 0; i < 32; i++) {
        idt_set_gate((uint8_t)i, (uint32_t)stubs[i], 0x10, 0x8E);
    }
}

void isr_install(void) {
    uint8_t mask;

    set_exc_gates();

    pic_remap(0x20, 0x28);

    outb(0x21, 0xFF);
    outb(0xA1, 0xFF);

    idt_set_gate(0x20, (uint32_t)irq0_stub, 0x10, 0x8E);
    idt_set_gate(0x21, (uint32_t)irq1_stub, 0x10, 0x8E);

    mask = inb(0x21);
    mask &= ~(1 << 0);
    mask &= ~(1 << 1);
    outb(0x21, mask);

    pit_init(100);
}
