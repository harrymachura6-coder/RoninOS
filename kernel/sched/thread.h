#pragma once

#include <stdint.h>
#include "../lib/types.h"

enum thread_state {
    THREAD_RUNNABLE = 0,
    THREAD_RUNNING = 1,
    THREAD_ZOMBIE = 2,
};

struct thread {
    uint32_t* esp;
    uint32_t* stack_base;
    size_t stack_size;
    enum thread_state state;
    const char* name;
    int tid;

    void (*entry)(void*);
    void* arg;
};

void sched_init(void);
int thread_create(const char* name, void (*entry)(void*), void* arg);
void thread_yield(void);
void thread_exit(void);
void sched_dump(void);

int sched_set_preempt(int enabled);
int sched_is_preempt_enabled(void);
