#ifndef APP_H
#define APP_H

#include "FreeRTOS.h"
#include "main.h"
#include "queue.h"

typedef enum {
    SENSOR_ID_WATER_TEMPERATURE = 1,
    SENSOR_ID_BATTERY_VOLTAGE   = 3,
} sensor_id_t;

typedef struct {
    sensor_id_t id;
    int16_t     value;
} sensor_data_t;
extern UART_HandleTypeDef huart3;

extern QueueHandle_t queue_data_raw;
extern QueueHandle_t queue_data_converted;
extern QueueHandle_t queue_data_filtered;

/**
 * @brief User application entry point. Never returns.
 *
 */
void app(void);

#endif /* APP_H */
