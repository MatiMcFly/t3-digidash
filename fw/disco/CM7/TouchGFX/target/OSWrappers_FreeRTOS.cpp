/**
  ******************************************************************************
  * @file    OSWrappers_FreeRTOS.cpp
  * @brief   Native-FreeRTOS implementation of TouchGFX OSWrappers.
  *
  * Replaces the TouchGFX-Generator-emitted target/generated/OSWrappers.cpp,
  * which is rewritten on every CubeMX/TouchGFX regen and would otherwise be
  * emitted against CMSIS-RTOSv2. This project uses native FreeRTOS, so the
  * generated file is excluded from the build (see CM7/CMakeLists.txt) and
  * this file is compiled in its place.
  ******************************************************************************
  */

#include <touchgfx/hal/HAL.hpp>
#include <touchgfx/hal/OSWrappers.hpp>

#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"

static SemaphoreHandle_t frame_buffer_sem = NULL;
static SemaphoreHandle_t vsync_sem        = NULL;

using namespace touchgfx;

void OSWrappers::initialize()
{
    frame_buffer_sem = xSemaphoreCreateBinary();
    configASSERT(frame_buffer_sem != NULL);
    /* Framebuffer "mutex" starts available. */
    xSemaphoreGive(frame_buffer_sem);

    vsync_sem = xSemaphoreCreateBinary();
    configASSERT(vsync_sem != NULL);
}

void OSWrappers::takeFrameBufferSemaphore()
{
    xSemaphoreTake(frame_buffer_sem, portMAX_DELAY);
}

void OSWrappers::giveFrameBufferSemaphore()
{
    xSemaphoreGive(frame_buffer_sem);
}

void OSWrappers::tryTakeFrameBufferSemaphore()
{
    xSemaphoreTake(frame_buffer_sem, 0);
}

void OSWrappers::giveFrameBufferSemaphoreFromISR()
{
    BaseType_t higherPriorityTaskWoken = pdFALSE;
    xSemaphoreGiveFromISR(frame_buffer_sem, &higherPriorityTaskWoken);
    portYIELD_FROM_ISR(higherPriorityTaskWoken);
}

void OSWrappers::signalVSync()
{
    if (vsync_sem != NULL)
    {
        BaseType_t higherPriorityTaskWoken = pdFALSE;
        xSemaphoreGiveFromISR(vsync_sem, &higherPriorityTaskWoken);
        portYIELD_FROM_ISR(higherPriorityTaskWoken);
    }
}

void OSWrappers::signalRenderingDone()
{
    /* Nothing to do for the binary-semaphore VSYNC scheme. */
}

void OSWrappers::waitForVSync()
{
    /* Drain any stale signal, then wait for the next one. */
    xSemaphoreTake(vsync_sem, 0);
    xSemaphoreTake(vsync_sem, portMAX_DELAY);
}

void OSWrappers::taskDelay(uint16_t ms)
{
    vTaskDelay(pdMS_TO_TICKS(ms));
}

void OSWrappers::taskYield()
{
    taskYIELD();
}
