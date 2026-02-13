#include "paging.h"

#include "pmm.h"
#include "../console.h"
#include "../panic.h"
#include "../serial.h"
#include "../lib/string.h"

#include <stdint.h>

#define MAX_BOOTSTRAP_TABLES 256u

static uint32_t g_page_directory[1024] __attribute__((aligned(4096)));
static uint32_t g_table_pool[MAX_BOOTSTRAP_TABLES][1024] __attribute__((aligned(4096)));
static uint32_t g_tables_used;
static int g_paging_enabled;

static void print_u32(uint32_t n) {
    char buf[11];
    int i = 0;
    if (n == 0) {
        console_putc('0');
        return;
    }
    while (n > 0 && i < (int)sizeof(buf)) {
        buf[i++] = (char)('0' + (n % 10u));
        n /= 10u;
    }
    while (i > 0) {
        console_putc(buf[--i]);
    }
}

static void serial_print_u32_dec(uint32_t n) {
    char buf[11];
    int i = 0;
    if (n == 0) {
        serial_putc('0');
        return;
    }
    while (n > 0 && i < (int)sizeof(buf)) {
        buf[i++] = (char)('0' + (n % 10u));
        n /= 10u;
    }
    while (i > 0) {
        serial_putc(buf[--i]);
    }
}

static void serial_print_u64_hex(uint64_t n) {
    static const char* hex = "0123456789ABCDEF";
    int shift;

    serial_print("0x");
    for (shift = 60; shift >= 0; shift -= 4) {
        serial_putc(hex[(n >> (uint32_t)shift) & 0xFu]);
    }
}

static uint32_t* alloc_table(void) {
    uint32_t* table;
    uint32_t phys;

    if (g_tables_used >= MAX_BOOTSTRAP_TABLES) {
        panic("paging: table pool exhausted");
    }

    table = g_table_pool[g_tables_used++];
    memset(table, 0, 4096);
    phys = (uint32_t)(uintptr_t)table;
    return (uint32_t*)(uintptr_t)phys;
}

static uint32_t* get_table(uint32_t virt, int create) {
    uint32_t pd_index = (virt >> 22) & 0x3FFu;
    if ((g_page_directory[pd_index] & PAGE_PRESENT) == 0u) {
        uint32_t* table;
        if (!create) {
            return 0;
        }
        table = alloc_table();
        g_page_directory[pd_index] = ((uint32_t)(uintptr_t)table) | PAGE_PRESENT | PAGE_WRITE;
        return table;
    }
    return (uint32_t*)(uintptr_t)(g_page_directory[pd_index] & 0xFFFFF000u);
}

static int map_range_identity(uint32_t phys_start, uint32_t bytes, uint32_t flags) {
    uint32_t start = phys_start & 0xFFFFF000u;
    uint32_t end;
    uint32_t addr;

    if (bytes == 0u) {
        return 0;
    }

    end = (phys_start + bytes + 0xFFFu) & 0xFFFFF000u;
    if (end < start) {
        return -1;
    }

    for (addr = start; addr < end; addr += PMM_FRAME_SIZE) {
        if (map_page(addr, addr, flags) != 0) {
            return -1;
        }
    }

    return 0;
}

int map_page(uint32_t virt, uint32_t phys, uint32_t flags) {
    uint32_t* table;
    uint32_t pt_index;

    if ((virt & 0xFFFu) || (phys & 0xFFFu)) {
        return -1;
    }

    table = get_table(virt, 1);
    pt_index = (virt >> 12) & 0x3FFu;
    table[pt_index] = (phys & 0xFFFFF000u) | (flags & 0xFFFu) | PAGE_PRESENT;

    if (g_paging_enabled) {
        __asm__ volatile("invlpg (%0)" : : "r"((void*)(uintptr_t)virt) : "memory");
    }

    return 0;
}

void unmap_page(uint32_t virt) {
    uint32_t* table;
    uint32_t pt_index;

    if (virt & 0xFFFu) {
        return;
    }

    table = get_table(virt, 0);
    if (!table) {
        return;
    }

    pt_index = (virt >> 12) & 0x3FFu;
    table[pt_index] = 0;

    if (g_paging_enabled) {
        __asm__ volatile("invlpg (%0)" : : "r"((void*)(uintptr_t)virt) : "memory");
    }
}

uint32_t translate(uint32_t virt) {
    uint32_t* table;
    uint32_t pt_index;

    table = get_table(virt, 0);
    if (!table) {
        return 0;
    }

    pt_index = (virt >> 12) & 0x3FFu;
    if ((table[pt_index] & PAGE_PRESENT) == 0u) {
        return 0;
    }

    return (table[pt_index] & 0xFFFFF000u) | (virt & 0xFFFu);
}

void paging_init(uint32_t phys_limit) {
    uint32_t cr0;
    uint64_t fb_addr = 0;
    uint32_t fb_pitch = 0;
    uint32_t fb_width = 0;
    uint32_t fb_height = 0;
    uint32_t fb_bpp = 0;

    memset(g_page_directory, 0, sizeof(g_page_directory));
    g_tables_used = 0;

    if (map_range_identity(0, phys_limit, PAGE_WRITE) != 0) {
        panic("paging_init: identity map failed");
    }

    if (console_get_framebuffer(&fb_addr, &fb_pitch, &fb_width, &fb_height, &fb_bpp)) {
        uint64_t fb_bytes64 = (uint64_t)fb_pitch * (uint64_t)fb_height;

        serial_print("paging: framebuffer addr=");
        serial_print_u64_hex(fb_addr);
        serial_print(" w=");
        serial_print_u32_dec(fb_width);
        serial_print(" h=");
        serial_print_u32_dec(fb_height);
        serial_print(" pitch=");
        serial_print_u32_dec(fb_pitch);
        serial_print(" bpp=");
        serial_print_u32_dec(fb_bpp);
        serial_print("\n");

        if (fb_addr > 0xFFFFFFFFull || fb_bytes64 > 0xFFFFFFFFull) {
            serial_print("paging: framebuffer mapping skipped (outside 32-bit range)\n");
        } else if (map_range_identity((uint32_t)fb_addr, (uint32_t)fb_bytes64, PAGE_WRITE) != 0) {
            panic("paging_init: framebuffer identity map failed");
        } else {
            serial_print("paging: framebuffer identity map ok\n");
        }
    } else {
        serial_print("paging: no active framebuffer backend\n");
    }

    __asm__ volatile("mov %0, %%cr3" : : "r"((uint32_t)(uintptr_t)g_page_directory) : "memory");

    __asm__ volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= 0x80000000u;
    __asm__ volatile("mov %0, %%cr0" : : "r"(cr0) : "memory");

    g_paging_enabled = 1;

    console_print("Paging: enabled, identity mapped bytes=");
    print_u32(phys_limit);
    console_putc('\n');
}
