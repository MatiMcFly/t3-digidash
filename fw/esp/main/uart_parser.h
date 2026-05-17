#ifndef UART_PARSER_H
#define UART_PARSER_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    UART_FIELD_WATER_TEMP = 1,
    UART_FIELD_RPM = 2,
    UART_FIELD_BATT_VOLT = 3,
    UART_FIELD_OUTSIDE_TEMP = 4,
    UART_FIELD_TURN_SIGNAL = 5,
    UART_FIELD_HIGH_BEAM = 6,
    UART_FIELD_PREHEAT = 7,
    UART_FIELD_TANK_LEVEL = 8
} uart_field_id_t;

bool parse_uart_values(const char *text, float *water_c, float *outside_c,
                       float *batt_v, int32_t *rpm, int32_t *turn_signal,
                       int32_t *high_beam, int32_t *preheat, int32_t *tank_level);

#endif
