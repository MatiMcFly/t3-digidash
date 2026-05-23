#include "app.h"

#include <stdbool.h>
#include <string.h>

#include "FreeRTOS.h"
#include "acquisition.h"
#include "conversion.h"
#include "filtering.h"
#include "main.h"
#include "publication.h"
#include "task.h"

QueueHandle_t queue_data_raw;
QueueHandle_t queue_data_converted;
QueueHandle_t queue_data_filtered;

static void heartbeat_task(void* params);

void app(void)
{
    HAL_UART_Transmit(&huart3, (uint8_t*)"Hello from M4\n", strlen("Hello from M4\n"), HAL_MAX_DELAY);

    if ((queue_data_raw = xQueueCreate(20, sizeof(sensor_data_t))) == NULL) {
        while (true) {} // TODO: Error handling
    }

    if ((queue_data_converted = xQueueCreate(20, sizeof(sensor_data_t))) == NULL) {
        while (true) {} // TODO: Error handling
    }

    if ((queue_data_filtered = xQueueCreate(20, sizeof(sensor_data_t))) == NULL) {
        while (true) {} // TODO: Error handling
    }

    if (xTaskCreate(heartbeat_task, "Heartbeat Task", 512, NULL, 1, NULL) != pdPASS) {
        while (true) {} // TODO: Error handling
    }

    if (xTaskCreate(acquisition_task, "Acquisition Task", 512, NULL, 3, NULL) != pdPASS) {
        while (true) {} // TODO: Error handling
    }

    if (xTaskCreate(conversion_task, "Conversion Task", 512, NULL, 2, NULL) != pdPASS) {
        while (true) {} // TODO: Error handling
    }

    if (xTaskCreate(filtering_task, "Filtering Task", 512, NULL, 2, NULL) != pdPASS) {
        while (true) {} // TODO: Error handling
    }

    if (xTaskCreate(publication_task, "Publication Task", 512, NULL, 2, NULL) != pdPASS) {
        while (true) {} // TODO: Error handling
    }

    vTaskStartScheduler();

    while (true) {
        // Should never reach here
    }
}

static void heartbeat_task(void* params)
{
    while (true) {
        HAL_GPIO_WritePin(LD1_GPIO_Port, LD1_Pin, GPIO_PIN_SET);
        vTaskDelay(pdMS_TO_TICKS(50));
        HAL_GPIO_WritePin(LD1_GPIO_Port, LD1_Pin, GPIO_PIN_RESET);
        vTaskDelay(pdMS_TO_TICKS(950));
    }
}
