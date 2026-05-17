#ifndef UART_PARSER_H
#define UART_PARSER_H

#include <stdbool.h>
#include <stdint.h>

bool parse_uart_values(const char *text, float *water_c, float *outside_c,
                       float *batt_v, int32_t *rpm, int32_t *turn_signal,
                       int32_t *high_beam, int32_t *preheat, int32_t *tank_level);

#endif
