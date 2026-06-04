#include "uart_task.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "esp_log.h"

#include "ble_app.h"
#include "car_signals.h"
#include "gatt_db.h"
#include "remotexy_protocol.h"
#include "uart_parser.h"

#define UART_TXD 4
#define UART_RXD 5
#define UART_RTS (-1)
#define UART_CTS (-1)
#define UART_PORT_NUM 1
#define UART_BAUD_RATE 115200
#define UART_TASK_STACK_SIZE 6144
#define UART_BUF_SIZE (1024)
#define UART_STREAM_BUF_SIZE (2048)
#define UART_LOG_EVERY 50

static const char *TAG = "ESP32C3_UART_TO_NUCLEO";

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
    static char stream_buf[UART_STREAM_BUF_SIZE];
    static size_t stream_len = 0;
    uint32_t line_count = 0;
    while (1) {
        int len = uart_read_bytes(UART_PORT_NUM, data, UART_BUF_SIZE - 1, 20 / portTICK_PERIOD_MS);
        if (len > 0) {
            if ((stream_len + (size_t)len) >= sizeof(stream_buf)) {
                ESP_LOGW(TAG, "UART stream overflow, dropping %u bytes", (unsigned)stream_len);
                stream_len = 0;
            }

            memcpy(&stream_buf[stream_len], data, (size_t)len);
            stream_len += (size_t)len;

            size_t start = 0;
            for (size_t i = 0; i < stream_len; ++i) {
                //if (stream_buf[i] == ';' || stream_buf[i] == '\n' || stream_buf[i] == '\r') {
                if (stream_buf[i] == ';') {
                    size_t line_len = i - start;
                    if (line_len > 0) {
                        char line[UART_BUF_SIZE];
                        size_t copy_len = line_len < (sizeof(line) - 1) ? line_len : (sizeof(line) - 1);
                        memcpy(line, &stream_buf[start], copy_len);
                        line[copy_len] = '\0';
                        line_count++;
                        ESP_LOGI(TAG, "UART token: %s", line);

                        uint16_t rpm = car_signals_get_rpm();
                        bool turn_signal = car_signals_get_turn_signal();
                        bool high_beam = car_signals_get_high_beam();
                        uint16_t tank_level = car_signals_get_tank_level();
                        int16_t water_c = car_signals_get_water_temp_cC();
                        uint16_t batt_v = car_signals_get_batt_mv();
                        bool oil_pressure_switch_3b = car_signals_get_oil_pressure_3b();
                        bool oil_pressure_switch_18b = car_signals_get_oil_pressure_18b();

                        if (parse_uart_values(line, &water_c, &rpm, &batt_v, &tank_level,
                                              &turn_signal, &high_beam, &oil_pressure_switch_3b, &oil_pressure_switch_18b)) {
                            car_signals_set_water_temp_c(water_c);
                            car_signals_set_batt_v(batt_v);
                            car_signals_set_rpm(rpm);
                            car_signals_set_turn_signal(turn_signal);
                            car_signals_set_high_beam(high_beam);
                            car_signals_set_tank_level(tank_level);
                            car_signals_oil_pressure_3b(oil_pressure_switch_3b);
                            car_signals_oil_pressure_18b(oil_pressure_switch_18b);

                            uint8_t turn = (uint8_t)turn_signal;
                            uint8_t high = (uint8_t)high_beam;
                            int16_t rpm_value = (int16_t)rpm;
                            float batt_v_out = (float)batt_v;

                            uint8_t oil_03 = (uint8_t)oil_pressure_switch_3b;
                            uint8_t oil_18 = (uint8_t)oil_pressure_switch_18b;
                            int8_t water_temp = (int8_t)water_c;
                            int8_t tank = (int8_t)tank_level;

                            remotexy_set_outputs(turn, high, rpm_value,
                                                 batt_v_out, oil_03, oil_18,
                                                 water_temp, tank);
                        } else {
                            ESP_LOGW(TAG, "UART parse failed for line: %s", line);
                        }
                    }
                    start = i + 1;
                }
            }

            if (start > 0) {
                size_t remaining = stream_len - start;
                if (remaining > 0) {
                    memmove(stream_buf, &stream_buf[start], remaining);
                }
                stream_len = remaining;
            }
        }
    }
}

void uart_task_start(void)
{
    xTaskCreate(uart_task_esp_to_nucleo, "uart_task_esp_to_nucleo", UART_TASK_STACK_SIZE, NULL, 10, NULL);
}
