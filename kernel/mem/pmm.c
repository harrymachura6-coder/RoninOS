#include "pmm.h"

#include "multiboot2.h"
#include "../console.h"
#include "../panic.h"
#include "../lib/string.h"

#include <stdint.h>

#define PMM_MAX_PHYS_ADDR (512u * 1024u * 1024u)
#define PMM_MAX_FRAMES (PMM_MAX_PHYS_ADDR / PMM_FRAME_SIZE)
#define PMM_BITMAP_WORDS (PMM_MAX_FRAMES / 32u)

extern uint8_t _kernel_start;
extern uint8_t _kernel_end;

static uint32_t g_bitmap[PMM_BITMAP_WORDS];
static uint32_t g_max_addr;
static pmm_stats_t g_stats;
static int g_ready;
static volatile uint32_t g_lock;


static uint32_t irq_save_disable(void) {
    uint32_t flags;
    __asm__ volatile("pushf; pop %0; cli" : "=r"(flags) : : "memory");
    return flags;
}

static void irq_restore(uint32_t flags) {
    __asm__ volatile("push %0; popf" : : "r"(flags) : "memory", "cc");
}

static uint32_t pmm_lock(void) {
    uint32_t flags = irq_save_disable();
    while (__sync_lock_test_and_set(&g_lock, 1u) != 0u) {
    }
    return flags;
}

static void pmm_unlock(uint32_t flags) {
    __sync_lock_release(&g_lock);
    irq_restore(flags);
}

static uintptr_t align_up(uintptr_t v, uintptr_t a) {
    return (v + (a - 1u)) & ~(a - 1u);
}

static uintptr_t align_down(uintptr_t v, uintptr_t a) {
    return v & ~(a - 1u);
}

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

static void set_frame(uint32_t idx) {
    g_bitmap[idx / 32u] |= (1u << (idx % 32u));
}

static void clear_frame(uint32_t idx) {
    g_bitmap[idx / 32u] &= ~(1u << (idx % 32u));
}

static int is_set(uint32_t idx) {
    return (g_bitmap[idx / 32u] & (1u << (idx % 32u))) != 0u;
}

static void reserve_range(uintptr_t start, uintptr_t end) {
    uint32_t i;
    uint32_t s;
    uint32_t e;

    if (start >= end) {
        return;
    }

    if (start >= g_max_addr) {
        return;
    }

    if (end > g_max_addr) {
        end = g_max_addr;
    }

    s = (uint32_t)(align_down(start, PMM_FRAME_SIZE) / PMM_FRAME_SIZE);
    e = (uint32_t)(align_up(end, PMM_FRAME_SIZE) / PMM_FRAME_SIZE);

    for (i = s; i < e; i++) {
        if (!is_set(i)) {
            set_frame(i);
            g_stats.free_frames--;
            g_stats.used_frames++;
        }
    }
}

static void free_range(uintptr_t start, uintptr_t end) {
    uint32_t i;
    uint32_t s;
    uint32_t e;

    if (start >= end) {
        return;
    }

    if (start >= g_max_addr) {
        return;
    }

    if (end > g_max_addr) {
        end = g_max_addr;
    }

    s = (uint32_t)(align_down(start, PMM_FRAME_SIZE) / PMM_FRAME_SIZE);
    e = (uint32_t)(align_up(end, PMM_FRAME_SIZE) / PMM_FRAME_SIZE);

    for (i = s; i < e; i++) {
        if (is_set(i)) {
            clear_frame(i);
            g_stats.free_frames++;
            g_stats.used_frames--;
        }
    }
}

static uint32_t tag_end(const struct mb2_tag* tag) {
    return (uint32_t)((uintptr_t)tag + ((tag->size + 7u) & ~7u));
}

int pmm_init(uint32_t mb_magic, uint32_t mb_info_addr) {
    const struct mb2_info_header* info;
    const struct mb2_tag* tag;
    const struct mb2_tag* end_tag;
    uint32_t highest = 0;

    if (mb_magic != MULTIBOOT2_BOOTLOADER_MAGIC || mb_info_addr == 0u) {
        panic("pmm_init: invalid multiboot2 info");
    }

    memset(g_bitmap, 0xFF, sizeof(g_bitmap));
    memset(&g_stats, 0, sizeof(g_stats));

    info = (const struct mb2_info_header*)(uintptr_t)mb_info_addr;
    tag = (const struct mb2_tag*)((uintptr_t)info + 8u);
    end_tag = (const struct mb2_tag*)((uintptr_t)info + info->total_size - 8u);

    while ((uintptr_t)tag < (uintptr_t)end_tag && tag->type != MULTIBOOT2_TAG_TYPE_END) {
        if (tag->type == MULTIBOOT2_TAG_TYPE_MMAP) {
            const struct mb2_tag_mmap* mmap_tag = (const struct mb2_tag_mmap*)tag;
            const uint8_t* ptr = (const uint8_t*)mmap_tag->entries;
            const uint8_t* lim = (const uint8_t*)tag + tag->size;
            while (ptr + mmap_tag->entry_size <= lim) {
                const struct mb2_mmap_entry* e = (const struct mb2_mmap_entry*)ptr;
                uint64_t end = e->addr + e->len;
                if (e->type == MULTIBOOT2_MMAP_TYPE_AVAILABLE) {
                    if (end > highest) {
                        highest = (uint32_t)(end > 0xFFFFFFFFu ? 0xFFFFFFFFu : end);
                    }
                }
                ptr += mmap_tag->entry_size;
            }
        } else if (tag->type == MULTIBOOT2_TAG_TYPE_BASIC_MEMINFO) {
            const struct mb2_tag_basic_meminfo* mem = (const struct mb2_tag_basic_meminfo*)tag;
            uint32_t basic_top = 0x100000u + (mem->mem_upper * 1024u);
            if (basic_top > highest) {
                highest = basic_top;
            }
        }
        tag = (const struct mb2_tag*)(uintptr_t)tag_end(tag);
    }

    if (highest == 0 || highest > PMM_MAX_PHYS_ADDR) {
        highest = PMM_MAX_PHYS_ADDR;
    }

    g_max_addr = (uint32_t)align_down(highest, PMM_FRAME_SIZE);
    g_stats.total_frames = g_max_addr / PMM_FRAME_SIZE;
    g_stats.used_frames = g_stats.total_frames;
    g_stats.free_frames = 0;
    g_stats.managed_bytes = g_max_addr;

    tag = (const struct mb2_tag*)((uintptr_t)info + 8u);
    while ((uintptr_t)tag < (uintptr_t)end_tag && tag->type != MULTIBOOT2_TAG_TYPE_END) {
        if (tag->type == MULTIBOOT2_TAG_TYPE_MMAP) {
            const struct mb2_tag_mmap* mmap_tag = (const struct mb2_tag_mmap*)tag;
            const uint8_t* ptr = (const uint8_t*)mmap_tag->entries;
            const uint8_t* lim = (const uint8_t*)tag + tag->size;
            while (ptr + mmap_tag->entry_size <= lim) {
                const struct mb2_mmap_entry* e = (const struct mb2_mmap_entry*)ptr;
                if (e->type == MULTIBOOT2_MMAP_TYPE_AVAILABLE) {
                    uint32_t start = (uint32_t)e->addr;
                    uint32_t end = (uint32_t)(e->addr + e->len);
                    free_range(start, end);
                }
                ptr += mmap_tag->entry_size;
            }
        }
        tag = (const struct mb2_tag*)(uintptr_t)tag_end(tag);
    }

    reserve_range(0, 0x100000u);
    reserve_range((uintptr_t)&_kernel_start, (uintptr_t)&_kernel_end);
    reserve_range((uintptr_t)mb_info_addr, (uintptr_t)mb_info_addr + info->total_size);

    tag = (const struct mb2_tag*)((uintptr_t)info + 8u);
    while ((uintptr_t)tag < (uintptr_t)end_tag && tag->type != MULTIBOOT2_TAG_TYPE_END) {
        if (tag->type == MULTIBOOT2_TAG_TYPE_MODULE) {
            const struct mb2_tag_module* mod = (const struct mb2_tag_module*)tag;
            reserve_range(mod->mod_start, mod->mod_end);
        } else if (tag->type == MULTIBOOT2_TAG_TYPE_FRAMEBUFFER) {
            const struct mb2_tag_framebuffer_common* fb = (const struct mb2_tag_framebuffer_common*)tag;
            uint32_t bytes = fb->framebuffer_pitch * fb->framebuffer_height;
            reserve_range((uintptr_t)fb->framebuffer_addr, (uintptr_t)fb->framebuffer_addr + bytes);
        }
        tag = (const struct mb2_tag*)(uintptr_t)tag_end(tag);
    }

    g_lock = 0;
    g_ready = 1;

    console_print("Memory map: managed bytes=");
    print_u32(g_stats.managed_bytes);
    console_print(", frames=");
    print_u32(g_stats.total_frames);
    console_putc('\n');

    return 0;
}

uint32_t pmm_alloc_frame(void) {
    uint32_t i;
    uint32_t bit;
    uint32_t frame;
    uint32_t flags;

    if (!g_ready) {
        return 0;
    }

    flags = pmm_lock();

    for (i = 0; i < PMM_BITMAP_WORDS; i++) {
        if (g_bitmap[i] != 0xFFFFFFFFu) {
            for (bit = 0; bit < 32u; bit++) {
                frame = i * 32u + bit;
                if (frame >= g_stats.total_frames) {
                    break;
                }
                if ((g_bitmap[i] & (1u << bit)) == 0u) {
                    g_bitmap[i] |= (1u << bit);
                    g_stats.free_frames--;
                    g_stats.used_frames++;
                    pmm_unlock(flags);
                    return frame * PMM_FRAME_SIZE;
                }
            }
        }
    }

    pmm_unlock(flags);
    return 0;
}

void pmm_free_frame(uint32_t phys_addr) {
    uint32_t frame;
    uint32_t flags;

    if (!g_ready || (phys_addr & (PMM_FRAME_SIZE - 1u)) != 0u) {
        return;
    }

    flags = pmm_lock();

    frame = phys_addr / PMM_FRAME_SIZE;
    if (frame >= g_stats.total_frames) {
        pmm_unlock(flags);
        return;
    }

    if (is_set(frame)) {
        clear_frame(frame);
        g_stats.free_frames++;
        g_stats.used_frames--;
    }

    pmm_unlock(flags);
}

void pmm_get_stats(pmm_stats_t* out) {
    uint32_t flags;
    if (!out) {
        return;
    }
    flags = pmm_lock();
    *out = g_stats;
    pmm_unlock(flags);
}

void pmm_dump_stats(void) {
    console_print("PMM: total=");
    print_u32(g_stats.total_frames);
    console_print(" free=");
    print_u32(g_stats.free_frames);
    console_print(" used=");
    print_u32(g_stats.used_frames);
    console_putc('\n');
}

uint32_t pmm_get_max_phys_addr(void) {
    return g_max_addr;
}
