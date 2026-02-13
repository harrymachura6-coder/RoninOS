#include "console.h"
#include "shell/shell.h"
#include "terminal/terminal.h"
#include "ui/theme.h"
#include "heap.h"
#include "idt.h"
#include "isr.h"
#include "keyboard.h"
#include "mem/paging.h"
#include "mem/pmm.h"
#include "panic.h"
#include "fs/ramfs.h"
#include "fs/vfs.h"
#include "fs/initrd.h"
#include "sched/thread.h"
#include "block/blockdev.h"
#include "drivers/ata_pio.h"

#include <stdint.h>

static void memory_smoke_test(void) {
    uint8_t* a;
    uint8_t* b;
    uint8_t* c;
    uint8_t* d;

    console_print("Memory smoke: kmalloc/kfree...\n");

    a = (uint8_t*)kmalloc(16);
    b = (uint8_t*)kmalloc(4096);
    c = (uint8_t*)kmalloc(8192);

    if (!a || !b || !c) {
        panic("memory smoke: allocation failed");
    }

    a[0] = 0x11;
    b[0] = 0x22;
    c[0] = 0x33;

    kfree(b);
    d = (uint8_t*)kmalloc(4096);

    if (!d) {
        panic("memory smoke: realloc alloc failed");
    }

    d[0] = 0x44;

    if (a == c || a == d || c == d) {
        panic("memory smoke: overlap detected");
    }

    kfree(a);
    kfree(c);
    kfree(d);

    console_print("Memory smoke: PASS\n");
}

void kmain(uint32_t mb_magic, uint32_t mb_info_addr) {
    terminal_init(&THEME_RONIN_DARK);
    console_print("RoninOS kernel startet!\n");

    console_print("Init: PMM...\n");
    pmm_init(mb_magic, mb_info_addr);
    pmm_dump_stats();

    console_print("Init: Paging...\n");
    paging_init(pmm_get_max_phys_addr());

    console_print("Init: Heap...\n");
    heap_init();

    memory_smoke_test();

    console_print("Init: RAMFS + VFS...\n");
    ramfs_init();
    vfs_init();
    vfs_mount("/", ramfs_ops(), 0);
    vfs_mkdir("/initrd");
    initrd_mount_from_multiboot(mb_magic, mb_info_addr);

    console_print("Init: Block devices...\n");
    block_init();
    ata_pio_discover();

    console_print("Init: IDT + PIC + Keyboard + Scheduler...\n");
    idt_init();
    isr_install();
    keyboard_init();
    sched_init();

    __asm__ volatile("sti");

    console_print("OK. Tippe 'help' und druecke Enter.\n");
    shell_init();

    for (;;) {
        __asm__ volatile("hlt");
    }
}
