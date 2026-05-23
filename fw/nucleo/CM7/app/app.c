#include "app.h"

#include <stdbool.h>

#include "FreeRTOS.h"
#include "main.h"
#include "shared.h"
#include "task.h"

static void heartbeat_task(void* params);

void app(void)
{
    xTaskCreate(heartbeat_task, "Heartbeat Task", 512, NULL, 1, NULL);

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
