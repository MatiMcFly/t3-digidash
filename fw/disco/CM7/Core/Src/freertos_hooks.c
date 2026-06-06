/**
  ******************************************************************************
  * @file    freertos_hooks.c
  * @brief   Mandatory FreeRTOS application hooks for diagnostic configs:
  *            - configCHECK_FOR_STACK_OVERFLOW != 0
  *            - configUSE_MALLOC_FAILED_HOOK   == 1
  ******************************************************************************
  */

#include "FreeRTOS.h"
#include "task.h"

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    (void)xTask;
    (void)pcTaskName;
    taskDISABLE_INTERRUPTS();
    for (;;) { __asm volatile("bkpt #0"); }
}

void vApplicationMallocFailedHook(void)
{
    taskDISABLE_INTERRUPTS();
    for (;;) { __asm volatile("bkpt #0"); }
}
