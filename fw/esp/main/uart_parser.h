#ifndef UART_PARSER_H
#define UART_PARSER_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    UART_FIELD_WATER_TEMP = 1,
    UART_FIELD_RPM = 2,
    UART_FIELD_BATT_VOLT = 3,
    UART_FIELD_TURN_SIGNAL = 5,
    UART_FIELD_HIGH_BEAM = 6,
    UART_FIELD_TANK_LEVEL = 7
} uart_field_id_t;

bool parse_uart_values(const char *text, int16_t *water_c,
                       uint16_t *batt_v, uint16_t *rpm, bool *turn_signal,
                       bool *high_beam, uint16_t *tank_level);

#endif
