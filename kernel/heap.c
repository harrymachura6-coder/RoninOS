#include "heap.h"

#include "mem/paging.h"
#include "mem/pmm.h"
#include "lib/string.h"
#include "panic.h"

#include <stdint.h>

typedef struct heap_block {
    size_t size;
    int free;
    struct heap_block* next;
    struct heap_block* prev;
} heap_block_t;

#define HEAP_ALIGN 16u
#define KHEAP_BASE 0x18000000u
#define KHEAP_MAX_SIZE (16u * 1024u * 1024u)
#define HEAP_INITIAL_PAGES 4u

static heap_block_t* g_head;
static uintptr_t g_heap_start;
static uintptr_t g_heap_end;
static uintptr_t g_heap_limit;
static volatile uint32_t g_lock;
static int g_heap_ready;

static uintptr_t align_up(uintptr_t v, uintptr_t a) {
    return (v + (a - 1u)) & ~(a - 1u);
}

static uint32_t irq_save_disable(void) {
    uint32_t flags;
    __asm__ volatile("pushf; pop %0; cli" : "=r"(flags) : : "memory");
    return flags;
}

static void irq_restore(uint32_t flags) {
    __asm__ volatile("push %0; popf" : : "r"(flags) : "memory", "cc");
}

static uint32_t heap_lock(void) {
    uint32_t flags = irq_save_disable();
    while (__sync_lock_test_and_set(&g_lock, 1u) != 0u) {
    }
    return flags;
}

static void heap_unlock(uint32_t flags) {
    __sync_lock_release(&g_lock);
    irq_restore(flags);
}

static size_t block_overhead(void) {
    return sizeof(heap_block_t);
}

static void split_block(heap_block_t* block, size_t wanted) {
    uintptr_t new_addr;
    heap_block_t* n;

    if (block->size <= wanted + block_overhead() + HEAP_ALIGN) {
        return;
    }

    new_addr = (uintptr_t)block + block_overhead() + wanted;
    n = (heap_block_t*)new_addr;
    n->size = block->size - wanted - block_overhead();
    n->free = 1;
    n->next = block->next;
    n->prev = block;

    if (n->next) {
        n->next->prev = n;
    }

    block->size = wanted;
    block->next = n;
}

static void merge_next(heap_block_t* block) {
    heap_block_t* n = block->next;
    if (!n || !n->free) {
        return;
    }
    block->size += block_overhead() + n->size;
    block->next = n->next;
    if (block->next) {
        block->next->prev = block;
    }
}

static heap_block_t* last_block(void) {
    heap_block_t* cur = g_head;
    while (cur && cur->next) {
        cur = cur->next;
    }
    return cur;
}

static int heap_expand(size_t min_bytes) {
    size_t needed = align_up(min_bytes + block_overhead(), PMM_FRAME_SIZE);
    size_t added = 0;
    heap_block_t* tail;

    while (added < needed) {
        uint32_t frame = pmm_alloc_frame();
        if (frame == 0u) {
            return -1;
        }
        if (g_heap_end + PMM_FRAME_SIZE > g_heap_limit) {
            pmm_free_frame(frame);
            return -1;
        }
        if (map_page((uint32_t)g_heap_end, frame, PAGE_WRITE) != 0) {
            pmm_free_frame(frame);
            return -1;
        }
        g_heap_end += PMM_FRAME_SIZE;
        added += PMM_FRAME_SIZE;
    }

    tail = last_block();
    if (!tail) {
        return -1;
    }

    if (tail->free) {
        tail->size += added;
    } else {
        heap_block_t* nb = (heap_block_t*)((uintptr_t)tail + block_overhead() + tail->size);
        nb->size = added - block_overhead();
        nb->free = 1;
        nb->next = 0;
        nb->prev = tail;
        tail->next = nb;
    }

    return 0;
}

void heap_init(void) {
    uint32_t i;

    g_heap_start = KHEAP_BASE;
    g_heap_end = g_heap_start;
    g_heap_limit = g_heap_start + KHEAP_MAX_SIZE;
    g_lock = 0;

    for (i = 0; i < HEAP_INITIAL_PAGES; i++) {
        uint32_t frame = pmm_alloc_frame();
        if (frame == 0u) {
            panic("heap_init: out of physical memory");
        }
        if (map_page((uint32_t)g_heap_end, frame, PAGE_WRITE) != 0) {
            panic("heap_init: map_page failed");
        }
        g_heap_end += PMM_FRAME_SIZE;
    }

    g_head = (heap_block_t*)g_heap_start;
    g_head->size = (size_t)(g_heap_end - g_heap_start - block_overhead());
    g_head->free = 1;
    g_head->next = 0;
    g_head->prev = 0;
    g_heap_ready = 1;
}

void* kmalloc(size_t size) {
    heap_block_t* cur;
    size_t wanted;
    uint32_t flags;

    if (!g_heap_ready || size == 0) {
        return 0;
    }

    wanted = (size + (HEAP_ALIGN - 1u)) & ~(HEAP_ALIGN - 1u);

    flags = heap_lock();

retry:
    cur = g_head;
    while (cur) {
        if (cur->free && cur->size >= wanted) {
            split_block(cur, wanted);
            cur->free = 0;
            heap_unlock(flags);
            return (void*)((uintptr_t)cur + block_overhead());
        }
        cur = cur->next;
    }

    if (heap_expand(wanted) == 0) {
        goto retry;
    }

    heap_unlock(flags);
    return 0;
}

void kfree(void* ptr) {
    heap_block_t* block;
    uint32_t flags;

    if (!g_heap_ready || !ptr) {
        return;
    }

    flags = heap_lock();

    block = (heap_block_t*)((uintptr_t)ptr - block_overhead());
    block->free = 1;
    merge_next(block);
    if (block->prev && block->prev->free) {
        merge_next(block->prev);
    }

    heap_unlock(flags);
}

void* krealloc(void* ptr, size_t new_size) {
    heap_block_t* block;
    size_t copy_size;
    void* np;

    if (!ptr) {
        return kmalloc(new_size);
    }

    if (new_size == 0) {
        kfree(ptr);
        return 0;
    }

    block = (heap_block_t*)((uintptr_t)ptr - block_overhead());
    if (block->size >= new_size) {
        return ptr;
    }

    np = kmalloc(new_size);
    if (!np) {
        return 0;
    }

    copy_size = block->size;
    if (copy_size > new_size) {
        copy_size = new_size;
    }

    memcpy(np, ptr, copy_size);
    kfree(ptr);
    return np;
}

void heap_get_stats(heap_stats_t* out) {
    heap_block_t* cur;
    heap_stats_t stats;
    uint32_t flags;

    if (!out) {
        return;
    }

    memset(&stats, 0, sizeof(stats));

    if (!g_heap_ready) {
        *out = stats;
        return;
    }

    flags = heap_lock();
    stats.heap_start = g_heap_start;
    stats.heap_end = g_heap_end;
    stats.total_bytes = g_heap_end - g_heap_start;

    cur = g_head;
    while (cur) {
        if (cur->free) {
            stats.free_blocks++;
            stats.free_bytes += cur->size;
            if (cur->size > stats.largest_free_block) {
                stats.largest_free_block = cur->size;
            }
        } else {
            stats.used_blocks++;
            stats.used_bytes += cur->size;
        }
        cur = cur->next;
    }
    heap_unlock(flags);

    *out = stats;
}
