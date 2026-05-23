#include "app.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "FreeRTOS.h"
#include "main.h"
#include "message_buffer.h"
#include "publication.h"
#include "shared.h"
#include "task.h"

static void heartbeat_task(void* params);

MessageBufferHandle_t ipc_message_buffer;

void app(void)
{
    HAL_UART_Transmit(&huart1, (uint8_t*)"Hello from M7\n", strlen("Hello from M7\n"), HAL_MAX_DELAY);

    ipc_message_buffer = (MessageBufferHandle_t)IPC_MB_STRUCT_ADDR; // Initialized by M4

    HAL_HSEM_ActivateNotification(__HAL_HSEM_SEMID_TO_MASK(IPC_HSEM_ID));

    if (xTaskCreate(heartbeat_task, "Heartbeat Task", 512, NULL, 1, NULL) != pdPASS) {
        while (true) {}
    }

    if (xTaskCreate(publication_task, "Publication Task", 512, NULL, 2, NULL) != pdPASS) {
        while (true) {}
    }

    vTaskStartScheduler();

    while (true) {
        // Should never reach here
    }
}

static void heartbeat_task(void* params)
{
    while (true) {
        HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_SET);
        vTaskDelay(pdMS_TO_TICKS(50));
        HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);
        vTaskDelay(pdMS_TO_TICKS(950));
    }
}
