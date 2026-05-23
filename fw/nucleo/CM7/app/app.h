#ifndef APP_H
#define APP_H

#include "FreeRTOS.h"
#include "main.h"
#include "message_buffer.h"

extern UART_HandleTypeDef huart1;

extern MessageBufferHandle_t ipc_message_buffer;

/**
 * @brief User application entry point. Never returns.
 *
 */
void app(void);

#endif /* APP_H */
