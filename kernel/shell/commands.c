#include "commands.h"

#include "../apps/apps.h"
#include "../console.h"
#include "../lib/string.h"
#include "../pit.h"
#include "../ports.h"
#include "../terminal/terminal.h"

extern int app_meminfo_main(int argc, char** argv);

typedef int (*cmd_fn_t)(int argc, char** argv);

typedef struct {
    const char* name;
    const char* help;
    cmd_fn_t fn;
} shell_cmd_t;

static int parse_args(char* line, char** argv, int max_argv) {
    int argc = 0;
    char* p = line;

    while (*p && argc < max_argv) {
        while (*p == ' ' || *p == '\t') {
            *p++ = 0;
        }
        if (!*p) break;

        if (*p == '"') {
            p++;
            argv[argc++] = p;
            while (*p && *p != '"') p++;
            if (*p == '"') *p++ = 0;
        } else {
            argv[argc++] = p;
            while (*p && *p != ' ' && *p != '\t') p++;
            if (*p) *p++ = 0;
        }
    }

    return argc;
}

static void print_u32(uint32_t v) {
    char tmp[16];
    int i = 0;
    if (v == 0) {
        console_putc('0');
        return;
    }
    while (v > 0 && i < (int)sizeof(tmp)) {
        tmp[i++] = (char)('0' + (v % 10));
        v /= 10;
    }
    while (i-- > 0) console_putc(tmp[i]);
}

static int cmd_help(int argc, char** argv);
static int cmd_clear(int argc, char** argv);
static int cmd_clean(int argc, char** argv);
static int cmd_echo(int argc, char** argv);
static int cmd_uname(int argc, char** argv);
static int cmd_mem(int argc, char** argv);
static int cmd_uptime(int argc, char** argv);
static int cmd_reboot(int argc, char** argv);

static const shell_cmd_t g_cmds[] = {
    {"help", "show command list", cmd_help},
    {"clear", "clear terminal", cmd_clear},
    {"clean", "alias for clear", cmd_clean},
    {"echo", "echo [text]", cmd_echo},
    {"uname", "show OS name", cmd_uname},
    {"mem", "show memory info", cmd_mem},
    {"uptime", "show uptime", cmd_uptime},
    {"reboot", "reboot machine", cmd_reboot},
};

void shell_print_help(void) {
    size_t i;
    size_t app_count = 0;
    const struct app_entry* apps = apps_get_table(&app_count);

    console_print("Builtins:\n");
    for (i = 0; i < sizeof(g_cmds) / sizeof(g_cmds[0]); i++) {
        console_print("  ");
        console_print(g_cmds[i].name);
        console_print(" - ");
        console_print(g_cmds[i].help);
        console_putc('\n');
    }

    console_print("Apps:\n");
    for (i = 0; i < app_count; i++) {
        if (!apps[i].main) continue;
        console_print("  ");
        console_print(apps[i].name);
        console_print(" - ");
        console_print(apps[i].help ? apps[i].help : "(no help)");
        console_putc('\n');
    }
}

static int cmd_help(int argc, char** argv) {
    (void)argc; (void)argv;
    shell_print_help();
    return 0;
}

static int cmd_clear(int argc, char** argv) {
    (void)argc; (void)argv;
    terminal_clear_scrollback();
    return 0;
}

static int cmd_clean(int argc, char** argv) {
    return cmd_clear(argc, argv);
}

static int cmd_echo(int argc, char** argv) {
    int i;
    for (i = 1; i < argc; i++) {
        if (i > 1) console_putc(' ');
        console_print(argv[i]);
    }
    console_putc('\n');
    return 0;
}

static int cmd_uname(int argc, char** argv) {
    (void)argc; (void)argv;
    console_print("RoninOS v2-shell i386\n");
    return 0;
}

static int cmd_mem(int argc, char** argv) {
    (void)argc;
    argv[0] = "meminfo";
    return app_meminfo_main(1, argv);
}

static int cmd_uptime(int argc, char** argv) {
    uint32_t ticks;
    uint32_t hz;
    (void)argc; (void)argv;

    ticks = pit_get_ticks();
    hz = pit_get_hz();
    console_print("uptime: ");
    if (hz == 0) hz = 100;
    print_u32(ticks / hz);
    console_print("s\n");
    return 0;
}

static int cmd_reboot(int argc, char** argv) {
    int i;
    (void)argc; (void)argv;
    console_print("Rebooting...\n");
    __asm__ volatile ("cli");

    for (i = 0; i < 100000; i++) {
        if ((inb(0x64) & 0x02u) == 0) break;
    }
    outb(0x64, 0xFE);

    for (i = 0; i < 100000; i++) {
        __asm__ volatile ("pause");
    }

    for (;;) {
        __asm__ volatile ("hlt");
    }
    return 0;
}

void shell_execute_line(const char* line) {
    char buf[128];
    char* argv[16];
    int argc;
    int i;
    const struct app_entry* app;
    size_t app_count = 0;
    const struct app_entry* apps;

    for (i = 0; i < (int)sizeof(buf) - 1 && line[i]; i++) {
        buf[i] = line[i];
    }
    buf[i] = 0;

    argc = parse_args(buf, argv, 16);
    if (argc <= 0) return;

    for (i = 0; i < (int)(sizeof(g_cmds) / sizeof(g_cmds[0])); i++) {
        if (strcmp(argv[0], g_cmds[i].name) == 0) {
            g_cmds[i].fn(argc, argv);
            return;
        }
    }

    apps = apps_get_table(&app_count);
    app = 0;
    for (i = 0; i < (int)app_count; i++) {
        if (strcmp(argv[0], apps[i].name) == 0) {
            app = &apps[i];
            break;
        }
    }

    if (app && app->main) {
        app->main(argc, argv);
    } else {
        terminal_write_error("Unknown command. Type 'help'.\n");
    }
}
