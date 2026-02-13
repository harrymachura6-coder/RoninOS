#include "keyboard.h"
#include "ports.h"
#include "pic.h"
#include <stdint.h>

extern void console_putc(char c);
extern void console_print(const char* s);
extern void console_prompt(void);
extern void shell_on_line(const char* line);

static char linebuf[128];
static int  len = 0;

static const char keymap[128] = {
  0,  27, '1','2','3','4','5','6','7','8','9','0','-','=', '\b',
  '\t','q','w','e','r','t','y','u','i','o','p','[',']','\n', 0,
  'a','s','d','f','g','h','j','k','l',';','\'','`', 0,'\\',
  'z','x','c','v','b','n','m',',','.','/', 0, '*', 0, ' ',
};

static void handle_char(char c) {
    if (c == '\n') {
        console_putc('\n');
        linebuf[len] = 0;
        shell_on_line(linebuf);
        len = 0;
        console_prompt();
        return;
    }
    if (c == '\b') {
        if (len > 0) {
            len--;
            // backspace "visuell": \b, space, \b
            console_putc('\b');
            console_putc(' ');
            console_putc('\b');
        }
        return;
    }
    if (c >= 32 && c <= 126) {
        if (len < (int)sizeof(linebuf) - 1) {
            linebuf[len++] = c;
            console_putc(c);
        }
    }
}

void keyboard_init(void) {
    // nichts weiter nötig im MVP
}

void keyboard_isr(void) {
    uint8_t sc = inb(0x60);
    if (sc & 0x80) {
        // key release ignorieren
    } else {
        char c = keymap[sc];
        if (c) handle_char(c);
    }
    pic_send_eoi(1);
}

void irq1_handler_c(void) {
    keyboard_isr();
}
