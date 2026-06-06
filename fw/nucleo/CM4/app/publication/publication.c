#include "publication.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "FreeRTOS.h"
#include "app.h"
#include "main.h"
#include "queue.h"
#include "shared.h"

void publication_mb_tx_callback(MessageBufferHandle_t xMessageBuffer, BaseType_t xIsInsideISR, BaseType_t* const pxHigherPriorityTaskWoken)
{
    if (xMessageBuffer != ipc_message_buffer) {
        HAL_UART_Transmit(&huart3, (uint8_t*)"publication: Invalid message buffer in tx callback\n", strlen("publication: Invalid message buffer in tx callback\n"), HAL_MAX_DELAY);
        return;
    }

    (void)xIsInsideISR;              // Doesn't matter
    (void)pxHigherPriorityTaskWoken; // Nothing to do: M4 tx will wake-up M7 task, not other M4 tasks

    if (HAL_HSEM_FastTake(IPC_HSEM_ID) == HAL_OK) {
        HAL_HSEM_Release(IPC_HSEM_ID, 0);
    } else {
        HAL_UART_Transmit(&huart3, (uint8_t*)"publication: HAL_HSEM_FastTake error\n", strlen("publication: HAL_HSEM_FastTake error\n"), HAL_MAX_DELAY);
    }
}

void publication_mb_rx_callback(MessageBufferHandle_t xMessageBuffer, BaseType_t xIsInsideISR, BaseType_t* const pxHigherPriorityTaskWoken)
{
    // Nothing to do: M4 only sends messages, never receives any
    (void)xMessageBuffer;
    (void)xIsInsideISR;
    (void)pxHigherPriorityTaskWoken;
}
