//     * **************************************************************** *
//   *                                                                      *
// *          (C) Copyright Matthias Schär, all rights reserved               *
//   *                                                                      *
//     * **************************************************************** *
/// @file App.c

/// @brief Main application file
#include "App.h"
#include "TaskDefinitions.h"
#include "Backlight.h"
#include "Display.h"
#include "app_touchgfx.h"

#include "stm32h7xx_hal.h"
#include "main.h"

#include "FreeRTOS.h"
#include "task.h"

#include <stddef.h>
#include <stdint.h>

// * ************************************************************************ *
// *                             GLOBAL FUNCTIONS                             *
// * ************************************************************************ *
bool AppInit(void) {

  // DisplayInit();
  BacklightInit();
  DisplayInit();

  /* TouchGFX render task */
  BaseType_t ok;
  ok = xTaskCreate((TaskFunction_t)MX_TouchGFX_Process,
                   "TouchGFX",
                   TOUCHGFX_TASK_STACK_SIZE,
                   NULL,
                   TOUCHGFX_TASK_PRIORITY,
                   NULL);
  if (ok != pdPASS) { Error_Handler(); }

  /* AppRun: generic application task, used for debugging */
  ok = xTaskCreate((TaskFunction_t)AppRun,
                   "App",
                   APP_TASK_STACK_SIZE,
                   NULL,
                   APP_TASK_PRIORITY,
                   NULL);
  if (ok != pdPASS) { Error_Handler(); }

  return true;
}

void AppRun(void) {
  BacklightSetDuty(100);

  for (;;) {
    vTaskDelay(pdMS_TO_TICKS(1U));
  }
}

// * ************************************************************************ *
// *                              LOCAL FUNCTIONS                             *
// * ************************************************************************ *

