//     * **************************************************************** *
//   *                                                                      *
// *          (C) Copyright Matthias Schär, all rights reserved               *
//   *                                                                      *
//     * **************************************************************** *
/// @file App.c

/// @brief Main application file
#include "App.h"
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
// *                                DEFINES                                   *
// * ************************************************************************ *

/* Step interval for the glow sweep: 10 ms/step -> ~1 s per 0->100 % ramp.  */
#define BL_GLOW_STEP_MS  10U

// * ************************************************************************ *
// *                                 TYPES                                    *
// * ************************************************************************ *

// * ************************************************************************ *
// *                             STATIC VARIABLES                             *
// * ************************************************************************ *

// * ************************************************************************ *
// *                           FORWARD DECLARATIONS                           *
// * ************************************************************************ *

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

  for (;;) {
    /* Ramp up: 0 -> 100 % */
    for (uint8_t d = 0U; d <= BACKLIGHT_DUTY_MAX; d++) {
      BacklightSetDuty(d);
      vTaskDelay(pdMS_TO_TICKS(BL_GLOW_STEP_MS));
    }
    /* Ramp down: 100 -> 0 % */
    for (uint8_t d = BACKLIGHT_DUTY_MAX; d-- > 0U; ) {
      BacklightSetDuty(d);
      vTaskDelay(pdMS_TO_TICKS(BL_GLOW_STEP_MS));
    }
  }
}

// * ************************************************************************ *
// *                              LOCAL FUNCTIONS                             *
// * ************************************************************************ *

