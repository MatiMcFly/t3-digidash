#include "publication.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "FreeRTOS.h"
#include "app.h"
#include "shared.h"
#include "task.h"

void HAL_HSEM_FreeCallback(uint32_t SemMask)
{
    BaseType_t     higher_priority_task_woken = pdFALSE;
    const uint32_t ipc_hsem_mask              = __HAL_HSEM_SEMID_TO_MASK(IPC_HSEM_ID);

    if ((SemMask & ipc_hsem_mask) != 0u) {
        HAL_HSEM_ActivateNotification(ipc_hsem_mask);

        (void)xMessageBufferSendCompletedFromISR(ipc_message_buffer, &higher_priority_task_woken);
    }

    portYIELD_FROM_ISR(higher_priority_task_woken);
}

void publication_task(void* params)
{
    sensor_data_t data;
    char          string[20] = "";

    while (true) {
        if (xMessageBufferReceive(ipc_message_buffer, &data, sizeof(data), portMAX_DELAY) == sizeof(data)) {
            snprintf(string, sizeof(string), "%u:%d;", data.id, data.value);
            HAL_UART_Transmit(&huart1, (uint8_t*)string, strlen(string), HAL_MAX_DELAY);
        }
    }
}
