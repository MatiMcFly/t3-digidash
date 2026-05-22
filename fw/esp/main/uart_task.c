#include "uart_task.h"

#include <limits.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "esp_log.h"

#include "car_signals.h"
#include "uart_parser.h"

#define UART_TXD 4
#define UART_RXD 5
#define UART_RTS (-1)
#define UART_CTS (-1)
#define UART_PORT_NUM 1
#define UART_BAUD_RATE 115200
#define UART_TASK_STACK_SIZE 3072
#define UART_BUF_SIZE (1024)

static const char *TAG = "ESP32C3_UART_TO_NUCLEO";

static uint16_t clamp_uint16_range(int32_t value, uint16_t min_value, uint16_t max_value)
{
    if (value < (int32_t)min_value) {
        return min_value;
    }
    if (value > (int32_t)max_value) {
        return max_value;
    }
    return (uint16_t)value;
}

static uint8_t clamp_uint8(int32_t value)
{
    if (value > UINT8_MAX) {
        return UINT8_MAX;
    }
    if (value < 0) {
        return 0;
    }
    return (uint8_t)value;
}

static void uart_task_esp_to_nucleo(void *arg)
{
    uart_config_t uart_config = {
        .baud_rate = UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };

    ESP_ERROR_CHECK(uart_driver_install(UART_PORT_NUM, UART_BUF_SIZE * 2, 0, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(UART_PORT_NUM, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(UART_PORT_NUM, UART_TXD, UART_RXD, UART_RTS, UART_CTS));

    uint8_t data[UART_BUF_SIZE];
    while (1) {
        int len = uart_read_bytes(UART_PORT_NUM, data, UART_BUF_SIZE - 1, 20 / portTICK_PERIOD_MS);
        if (len > 0) {
            data[len] = '\0';
            ESP_LOGI(TAG, "Received: %s", (char *)data);

            float water_c = car_signals_get_water_temp_cC() / 100.0f;
            float outside_c = car_signals_get_outside_temp_cC() / 100.0f;
            float batt_v = car_signals_get_batt_mv() / 1000.0f;
            int32_t rpm = car_signals_get_rpm();
            int32_t turn_signal = car_signals_get_turn_signal();
            int32_t high_beam = car_signals_get_high_beam();
            int32_t preheat = car_signals_get_preheat();
            int32_t tank_level = car_signals_get_tank_level();

            if (parse_uart_values((char *)data, &water_c, &batt_v,
                                  &rpm, &turn_signal, &high_beam, &tank_level)) {
                car_signals_set_water_temp_c(water_c);
                car_signals_set_batt_v(batt_v);
                car_signals_set_rpm(clamp_uint16_range(rpm, 0, UINT16_MAX));
                car_signals_set_turn_signal(clamp_uint8(turn_signal));
                car_signals_set_high_beam(clamp_uint8(high_beam));
                car_signals_set_tank_level(clamp_uint8(tank_level));
            } else {
                ESP_LOGW(TAG, "UART parse failed; expected 'WT=23.5,OT=12.1,BV=12.6,RPM=1500,TS=1,HB=0,DP=0,TL=45'");
            }
        }
    }
}

void uart_task_start(void)
{
    xTaskCreate(uart_task_esp_to_nucleo, "uart_task_esp_to_nucleo", UART_TASK_STACK_SIZE, NULL, 10, NULL);
}
