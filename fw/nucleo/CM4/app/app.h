#ifndef APP_H
#define APP_H

#include "FreeRTOS.h"
#include "main.h"
#include "queue.h"

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
