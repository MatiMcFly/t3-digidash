#ifndef PUBLICATION_H
#define PUBLICATION_H

#include "FreeRTOS.h"
#include "message_buffer.h"

void publication_task(void* params);
void publication_mb_tx_callback(MessageBufferHandle_t xMessageBuffer, BaseType_t xIsInsideISR, BaseType_t* const pxHigherPriorityTaskWoken);
void publication_mb_rx_callback(MessageBufferHandle_t xMessageBuffer, BaseType_t xIsInsideISR, BaseType_t* const pxHigherPriorityTaskWoken);

#endif /* PUBLICATION_H */
