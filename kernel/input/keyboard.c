#include "keyboard.h"

#include "../pic.h"
#include "../ports.h"
#include "../shell/shell.h"

static uint8_t g_mods = 0;
static uint8_t g_e0 = 0;

static const char keymap[128] = {
    0,  27, '1','2','3','4','5','6','7','8','9','0','-','=', 0,
  '\t','q','w','e','r','t','y','u','i','o','p','[',']','\n', 0,
   'a','s','d','f','g','h','j','k','l',';','\'','`', 0,'\\',
   'z','x','c','v','b','n','m',',','.','/', 0, '*', 0, ' ',
};

static const char keymap_shift[128] = {
    0,  27, '!','@','#','$','%','^','&','*','(',')','_','+', 0,
  '\t','Q','W','E','R','T','Y','U','I','O','P','{','}','\n', 0,
   'A','S','D','F','G','H','J','K','L',':','"','~', 0,'|',
   'Z','X','C','V','B','N','M','<','>','?', 0, '*', 0, ' ',
};

static void emit_key(key_event_t ev) {
    shell_handle_key(ev);
}

static char map_ascii(uint8_t sc) {
    if (sc >= 128) return 0;
    if (g_mods & KEYMOD_SHIFT) return keymap_shift[sc];
    return keymap[sc];
}

static void on_key_press(uint8_t sc) {
    key_event_t ev;
    char c;

    ev.code = KEY_NONE;
    ev.ch = 0;
    ev.modifiers = g_mods;

    if (g_e0) {
        g_e0 = 0;
        switch (sc) {
            case 0x4B: ev.code = KEY_LEFT; break;
            case 0x4D: ev.code = KEY_RIGHT; break;
            case 0x48: ev.code = KEY_UP; break;
            case 0x50: ev.code = KEY_DOWN; break;
            case 0x47: ev.code = KEY_HOME; break;
            case 0x4F: ev.code = KEY_END; break;
            case 0x49: ev.code = KEY_PAGEUP; break;
            case 0x51: ev.code = KEY_PAGEDOWN; break;
            case 0x53: ev.code = KEY_DELETE; break;
            default: break;
        }
        if (ev.code != KEY_NONE) emit_key(ev);
        return;
    }

    switch (sc) {
        case 0x1C: ev.code = KEY_ENTER; emit_key(ev); return;
        case 0x0E: ev.code = KEY_BACKSPACE; emit_key(ev); return;
        case 0x0F: ev.code = KEY_TAB; emit_key(ev); return;
        case 0x48: ev.code = KEY_UP; emit_key(ev); return;
        case 0x50: ev.code = KEY_DOWN; emit_key(ev); return;
        case 0x47: ev.code = KEY_HOME; emit_key(ev); return;
        case 0x4F: ev.code = KEY_END; emit_key(ev); return;
        case 0x49: ev.code = KEY_PAGEUP; emit_key(ev); return;
        case 0x51: ev.code = KEY_PAGEDOWN; emit_key(ev); return;
        default: break;
    }

    c = map_ascii(sc);
    if (c >= 32 && c <= 126) {
        ev.code = KEY_CHAR;
        ev.ch = c;
        emit_key(ev);
    }
}

void keyboard_init(void) {
    g_mods = 0;
    g_e0 = 0;
}

void keyboard_isr(void) {
    uint8_t sc = inb(0x60);

    if (sc == 0xE0) {
        g_e0 = 1;
        pic_send_eoi(1);
        return;
    }

    if (sc & 0x80) {
        uint8_t rel = (uint8_t)(sc & 0x7F);
        if (rel == 0x2A || rel == 0x36) g_mods &= (uint8_t)~KEYMOD_SHIFT;
        if (rel == 0x1D) g_mods &= (uint8_t)~KEYMOD_CTRL;
        if (rel == 0x38) g_mods &= (uint8_t)~KEYMOD_ALT;
    } else {
        if (sc == 0x2A || sc == 0x36) g_mods |= KEYMOD_SHIFT;
        else if (sc == 0x1D) g_mods |= KEYMOD_CTRL;
        else if (sc == 0x38) g_mods |= KEYMOD_ALT;
        else on_key_press(sc);
    }

    pic_send_eoi(1);
}

void irq1_handler_c(void) {
    keyboard_isr();
}
