#include "thread.h"

#include "../console.h"
#include "../heap.h"
#include "../panic.h"

#define THREAD_MAX 32
#define THREAD_STACK_SIZE 4096

extern void thread_switch(uint32_t** old_esp, uint32_t* new_esp);

static struct thread g_threads[THREAD_MAX];
static int g_thread_count;
static int g_current_tid;
static int g_preempt_enabled;

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
        i--;
        console_putc(buf[i]);
    }
}

static void print_hex(uint32_t n) {
    int shift;
    console_print("0x");
    for (shift = 28; shift >= 0; shift -= 4) {
        uint32_t d = (n >> (uint32_t)shift) & 0xFu;
        console_putc((char)(d < 10 ? ('0' + d) : ('A' + (d - 10))));
    }
}

static const char* state_name(enum thread_state state) {
    if (state == THREAD_RUNNING) return "RUNNING";
    if (state == THREAD_RUNNABLE) return "RUNNABLE";
    return "ZOMBIE";
}

static int find_next_runnable(int from_tid, int allow_current) {
    int i;

    if (g_thread_count <= 0) return -1;

    for (i = 1; i <= g_thread_count; i++) {
        int tid = (from_tid + i) % g_thread_count;
        if (!allow_current && tid == from_tid) {
            continue;
        }
        if (g_threads[tid].state == THREAD_RUNNABLE) {
            return tid;
        }
    }

    return -1;
}

static void thread_bootstrap(void) {
    struct thread* t = &g_threads[g_current_tid];
    void (*entry)(void*) = t->entry;
    void* arg = t->arg;

    entry(arg);
    thread_exit();
}

void sched_init(void) {
    g_thread_count = 1;
    g_current_tid = 0;
    g_preempt_enabled = 0;

    g_threads[0].esp = 0;
    g_threads[0].stack_base = 0;
    g_threads[0].stack_size = 0;
    g_threads[0].state = THREAD_RUNNING;
    g_threads[0].name = "main";
    g_threads[0].tid = 0;
    g_threads[0].entry = 0;
    g_threads[0].arg = 0;
}

int thread_create(const char* name, void (*entry)(void*), void* arg) {
    struct thread* t;
    uint32_t* sp;

    if (!entry) return -1;
    if (g_thread_count >= THREAD_MAX) return -1;

    t = &g_threads[g_thread_count];
    t->stack_size = THREAD_STACK_SIZE;
    t->stack_base = (uint32_t*)kmalloc(t->stack_size);
    if (!t->stack_base) return -1;

    sp = (uint32_t*)((uint8_t*)t->stack_base + t->stack_size);

    *--sp = (uint32_t)(uintptr_t)thread_bootstrap;
    *--sp = 0;
    *--sp = 0;
    *--sp = 0;
    *--sp = 0;

    t->esp = sp;
    t->state = THREAD_RUNNABLE;
    t->name = name ? name : "thread";
    t->tid = g_thread_count;
    t->entry = entry;
    t->arg = arg;

    g_thread_count++;
    return t->tid;
}

void thread_yield(void) {
    int prev_tid = g_current_tid;
    int next_tid = find_next_runnable(prev_tid, 0);
    struct thread* prev;
    struct thread* next;

    if (next_tid < 0) {
        return;
    }

    prev = &g_threads[prev_tid];
    next = &g_threads[next_tid];

    if (prev->state == THREAD_RUNNING) {
        prev->state = THREAD_RUNNABLE;
    }
    next->state = THREAD_RUNNING;
    g_current_tid = next_tid;

    thread_switch(&prev->esp, next->esp);
}

void thread_exit(void) {
    int next_tid;
    int dead_tid = g_current_tid;

    g_threads[dead_tid].state = THREAD_ZOMBIE;

    next_tid = find_next_runnable(dead_tid, 0);
    if (next_tid < 0) {
        panic("thread_exit: no runnable thread left");
    }

    g_threads[next_tid].state = THREAD_RUNNING;
    g_current_tid = next_tid;

    thread_switch(&g_threads[dead_tid].esp, g_threads[next_tid].esp);

    panic("thread_exit: switch returned unexpectedly");
}

void sched_dump(void) {
    int i;

    console_print("tid name state esp stack\n");
    for (i = 0; i < g_thread_count; i++) {
        struct thread* t = &g_threads[i];
        uint32_t stack_start = (uint32_t)(uintptr_t)t->stack_base;
        uint32_t stack_end = stack_start + (uint32_t)t->stack_size;

        print_u32((uint32_t)t->tid);
        console_putc(' ');
        console_print(t->name ? t->name : "-");
        console_putc(' ');
        console_print(state_name(t->state));
        console_putc(' ');
        print_hex((uint32_t)(uintptr_t)t->esp);
        console_putc(' ');
        print_hex(stack_start);
        console_putc('-');
        print_hex(stack_end);
        if (i == g_current_tid) {
            console_print(" <current>");
        }
        console_putc('\n');
    }
}

int sched_set_preempt(int enabled) {
    g_preempt_enabled = enabled ? 1 : 0;
    return g_preempt_enabled;
}

int sched_is_preempt_enabled(void) {
    return g_preempt_enabled;
}
