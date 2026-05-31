#include "uart_parser.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "esp_log.h"

#define UART_PARSER_BUF_SIZE 256

bool parse_uart_values(const char *text, int16_t *water_c, uint16_t *rpm, uint16_t *batt_v , uint16_t *tank_level, bool *turn_signal,
                       bool *high_beam, bool *oil_pressure_switch_3b, bool *oil_pressure_switch_18b)
{
    if (text == NULL) {
        return false;
    }

    static char buffer[UART_PARSER_BUF_SIZE];
    strncpy(buffer, text, sizeof(buffer) - 1);
    buffer[sizeof(buffer) - 1] = '\0';
    
    // Expected format: "1:23.5;2:1500;3:12.6;4:18.2;5:1;6:0;7:1;8:75"
    // 1:23;2:1500;3:100;4:50;5:1;6:0;7:1;8:0
    // Where each pair is "id:value" and pairs are separated by ';'

    int parsed_count = 0;
    bool had_error = false;
    // context is used by strtok_r to maintain state between calls
    char *context = NULL;
    // Get the first pair of id and value
    char *pairs = strtok_r(buffer, ";", &context);
    while (pairs != NULL) {
        // separate the id and value by finding the ':' character
        char *sep = strchr(pairs, ':');
        if (sep != NULL) {
            *sep = '\0';
            // Convert the id to an integer
            int32_t id = strtol(pairs, NULL, 10);
            // pointer to the value string (after the ':')
            char *value = sep + 1;

            switch (id) {
            case UART_FIELD_WATER_TEMP:
                if (water_c) {
                    // Note: strtof returns float
                    *water_c = (int16_t)strtof(value, NULL);
                    parsed_count++;
                }
                break;
            case UART_FIELD_RPM:
                if (rpm) {
                    *rpm = (uint16_t)strtol(value, NULL, 10);
                    parsed_count++;
                }
                break;
            case UART_FIELD_BATT_VOLT:
                if (batt_v) {
                    *batt_v = (uint16_t)strtof(value, NULL);
                    parsed_count++;
                }
                break;
            case UART_FIELD_TANK_LEVEL:
            if (tank_level) {
                *tank_level = (int16_t)strtol(value, NULL, 10);
                parsed_count++;
            }
            break;
            case UART_FIELD_TURN_SIGNAL:
                if (turn_signal) {
                    *turn_signal = (bool)strtol(value, NULL, 10);
                    parsed_count++;
                }
                break;
            case UART_FIELD_HIGH_BEAM:
                if (high_beam) {
                    *high_beam = (bool)strtol(value, NULL, 10);
                    parsed_count++;
                }
                break;
            case UART_FIELD_OIL_PRESSURE_SWITCH_3B:
                if (oil_pressure_switch_3b) {
                    *oil_pressure_switch_3b = (bool)strtol(value, NULL, 10);
                    parsed_count++;
                }
                break;
            case UART_FIELD_OIL_PRESSURE_SWITCH_18B:
                if (oil_pressure_switch_18b) {
                    *oil_pressure_switch_18b = (bool)strtol(value, NULL, 10);
                    parsed_count++;
                }
                break;
            
            default:
                had_error = true;
                break;
            }
        } else {
            had_error = true;
        }
        // move to the next pair
        pairs = strtok_r(NULL, ";", &context);
    }

    if (parsed_count == 0) {
        return false;
    }

    return !had_error;
}
