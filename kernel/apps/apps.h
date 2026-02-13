#pragma once

#include "../lib/types.h"

typedef int (*app_main_t)(int argc, char** argv);

struct app_entry {
    const char* name;
    const char* help;
    app_main_t main;
};

const struct app_entry* apps_get_table(size_t* out_count);
