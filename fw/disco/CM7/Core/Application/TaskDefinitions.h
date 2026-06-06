//     * **************************************************************** *
//   *                                                                      *
// *          (C) Copyright Matthias Schär, all rights reserved               *
//   *                                                                      *
//     * **************************************************************** *
/// @file TaskDefinitions.h

/// @brief Defines task stack sizes and priorities for the application tasks.
#ifndef __TASK_DEFINITIONS_H__
#define __TASK_DEFINITIONS_H__

/** *** TASK STACK SIZES *** */
#define UART_RX_TASK_STACK_SIZE   (1024U / sizeof(StackType_t))
#define TOUCHGFX_TASK_STACK_SIZE  (4096U  / sizeof(StackType_t))
#define APP_TASK_STACK_SIZE       (512U  / sizeof(StackType_t))

/** *** TASK PRIORITIES *** */
#define UART_RX_TASK_PRIORITY    (tskIDLE_PRIORITY + 3)
#define TOUCHGFX_TASK_PRIORITY   (tskIDLE_PRIORITY + 2)
#define APP_TASK_PRIORITY        (tskIDLE_PRIORITY + 1)

#endif /* __TASK_DEFINITIONS_H__ */