#pragma once
#include <stddef.h>

typedef int (*command_fn)(int argc, char** argv);

typedef struct {
    const char* name;
    const char* desc;
    command_fn fn;
} command_t;

int cmd_help(int argc, char** argv);
int cmd_clear(int argc, char** argv);
int cmd_echo(int argc, char** argv);

// zentrale Liste
extern const command_t g_commands[];
extern const size_t g_command_count;
