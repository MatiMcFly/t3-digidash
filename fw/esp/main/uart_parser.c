#include "uart_parser.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "esp_log.h"

#define UART_PARSER_BUF_SIZE 1024

static void normalize_key(char *key)
{
    if (key == NULL) {
        return;
    }
    while (*key != '\0') {
        *key = (char)tolower((unsigned char)*key);
        key++;
    }
}

bool parse_uart_values(const char *text, float *water_c, float *outside_c,
                       float *batt_v, int32_t *rpm, int32_t *turn_signal,
                       int32_t *high_beam, int32_t *preheat, int32_t *tank_level)
{
    if (text == NULL) {
        return false;
    }

    if (water_c && outside_c && batt_v) {
        float w = 0.0f;
        float b = 0.0f;
        if (sscanf(text, "WT=%f,BV=%f", &w, &b) == 2) {
            *water_c = w;
            *batt_v = b;
            return true;
        }
        if (sscanf(text, "%f,%f", &w, &b) == 2) {
            *water_c = w;
            *batt_v = b;
            return true;
        }
        if (sscanf(text, "%f %f", &w, &b) == 2) {
            *water_c = w;
            *batt_v = b;
            return true;
        }
    }

    char buffer[UART_PARSER_BUF_SIZE];
    strncpy(buffer, text, sizeof(buffer) - 1);
    buffer[sizeof(buffer) - 1] = '\0';

    bool parsed_any = false;
    char *context = NULL;
    char *token = strtok_r(buffer, ",", &context);
    while (token != NULL) {
        while (*token == ' ' || *token == '\t') {
            token++;
        }
        char *eq = strchr(token, '=');
        if (eq != NULL) {
            *eq = '\0';
            char *key = token;
            char *value = eq + 1;
            normalize_key(key);

            if (water_c && strcmp(key, "wt") == 0) {
                *water_c = strtof(value, NULL);
                parsed_any = true;
            } else if (outside_c && strcmp(key, "ot") == 0) {
                *outside_c = strtof(value, NULL);
                parsed_any = true;
            } else if (batt_v && strcmp(key, "bv") == 0) {
                *batt_v = strtof(value, NULL);
                parsed_any = true;
            } else if (rpm && strcmp(key, "rpm") == 0) {
                *rpm = strtol(value, NULL, 10);
                parsed_any = true;
            } else if (turn_signal && strcmp(key, "ts") == 0) {
                *turn_signal = strtol(value, NULL, 10);
                parsed_any = true;
            } else if (high_beam && strcmp(key, "hb") == 0) {
                *high_beam = strtol(value, NULL, 10);
                parsed_any = true;
            } else if (preheat && strcmp(key, "dp") == 0) {
                *preheat = strtol(value, NULL, 10);
                parsed_any = true;
            } else if (tank_level && strcmp(key, "tl") == 0) {
                *tank_level = strtol(value, NULL, 10);
                parsed_any = true;
            }
        }
        token = strtok_r(NULL, ",", &context);
    }

    return parsed_any;
}
