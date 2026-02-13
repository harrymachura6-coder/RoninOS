#pragma once

#include <stdint.h>

typedef enum {
    KEY_NONE = 0,
    KEY_CHAR,
    KEY_ENTER,
    KEY_BACKSPACE,
    KEY_DELETE,
    KEY_LEFT,
    KEY_RIGHT,
    KEY_UP,
    KEY_DOWN,
    KEY_HOME,
    KEY_END,
    KEY_PAGEUP,
    KEY_PAGEDOWN,
    KEY_TAB,
} key_code_t;

enum {
    KEYMOD_SHIFT = 1 << 0,
    KEYMOD_CTRL = 1 << 1,
    KEYMOD_ALT = 1 << 2,
};

typedef struct {
    key_code_t code;
    char ch;
    uint8_t modifiers;
} key_event_t;

void keyboard_init(void);
void keyboard_isr(void);
void irq1_handler_c(void);
