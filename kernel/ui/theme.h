#pragma once

#include <stdint.h>

typedef struct {
    uint8_t bg;
    uint8_t fg_output;
    uint8_t fg_prompt;
    uint8_t fg_input;
    uint8_t fg_error;
    uint8_t fg_status;
    uint8_t fg_accent;
} terminal_theme_t;

static inline uint8_t theme_attr(uint8_t fg, uint8_t bg) {
    return (uint8_t)((bg << 4) | (fg & 0x0F));
}

static const terminal_theme_t THEME_RONIN_DARK = {
    .bg = 0,
    .fg_output = 7,
    .fg_prompt = 10,
    .fg_input = 15,
    .fg_error = 12,
    .fg_status = 11,
    .fg_accent = 14,
};

static const terminal_theme_t THEME_RONIN_AMBER = {
    .bg = 0,
    .fg_output = 6,
    .fg_prompt = 14,
    .fg_input = 15,
    .fg_error = 4,
    .fg_status = 14,
    .fg_accent = 3,
};
