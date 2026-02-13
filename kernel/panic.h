#pragma once

void panic(const char* message);
void panic_assert_failed(const char* expr, const char* file, int line);

#define assert(expr) ((expr) ? (void)0 : panic_assert_failed(#expr, __FILE__, __LINE__))
