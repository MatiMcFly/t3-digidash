#ifndef APP_H
#define APP_H

#include "FreeRTOS.h"
#include "main.h"
#include "message_buffer.h"
#include "queue.h"

#define QUEUE_SIZE       50
#define QUEUE_TIMEOUT_MS 20

extern UART_HandleTypeDef huart3;

extern QueueHandle_t queue_data_raw;
extern QueueHandle_t queue_data_converted;
extern QueueHandle_t queue_data_filtered;

extern MessageBufferHandle_t ipc_message_buffer;

/**
 * @brief User application entry point. Never returns.
 *
 */
void app(void);

#endif /* APP_H */
