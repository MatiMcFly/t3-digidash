#include "uart_parser.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "esp_log.h"

#define UART_PARSER_BUF_SIZE 1024

bool parse_uart_values(const char *text, float *water_c, float *outside_c,
                       float *batt_v, int32_t *rpm, int32_t *turn_signal,
                       int32_t *high_beam, int32_t *preheat, int32_t *tank_level)
{
    if (text == NULL) {
        return false;
    }

    char buffer[UART_PARSER_BUF_SIZE];
    strncpy(buffer, text, sizeof(buffer) - 1);
    buffer[sizeof(buffer) - 1] = '\0';
    
    // Expected format: "1:23.5;2:1500;3:12.6;4:18.2;5:1;6:0;7:1;8:75"
    // Where each pair is "id:value" and pairs are separated by ';'

    int parsed_count = 0;
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
                    *water_c = strtof(value, NULL);
                    parsed_count++;
                }
                break;
            case UART_FIELD_RPM:
                if (rpm) {
                    *rpm = strtol(value, NULL, 10);
                    parsed_count++;
                }
                break;
            case UART_FIELD_BATT_VOLT:
                if (batt_v) {
                    *batt_v = strtof(value, NULL);
                    parsed_count++;
                }
                break;
            case UART_FIELD_OUTSIDE_TEMP:
                if (outside_c) {
                    *outside_c = strtof(value, NULL);
                    parsed_count++;
                }
                break;
            case UART_FIELD_TURN_SIGNAL:
                if (turn_signal) {
                    *turn_signal = strtol(value, NULL, 10);
                    parsed_count++;
                }
                break;
            case UART_FIELD_HIGH_BEAM:
                if (high_beam) {
                    *high_beam = strtol(value, NULL, 10);
                    parsed_count++;
                }
                break;
            case UART_FIELD_PREHEAT:
                if (preheat) {
                    *preheat = strtol(value, NULL, 10);
                    parsed_count++;
                }
                break;
            case UART_FIELD_TANK_LEVEL:
                if (tank_level) {
                    *tank_level = strtol(value, NULL, 10);
                    parsed_count++;
                }
                break;
            default:
                break;
            }
        }
        // move to the next pair
        pairs = strtok_r(NULL, ";", &context);
    }

    return parsed_count == 8;
}
