//     * **************************************************************** *
//   *                                                                      *
// *          (C) Copyright Matthias Schär, all rights reserved               *
//   *                                                                      *
//     * **************************************************************** *
/// @file App.h

/// @brief Main application header file
#ifndef __APP_H__
#define __APP_H__

#include <stdbool.h>

// * ************************************************************************ *
// *                                DEFINES                                   *
// * ************************************************************************ *
/** *** TASK STACK SIZES *** */
#define TOUCHGFX_TASK_STACK_SIZE (4096U  / sizeof(StackType_t))
#define APP_TASK_STACK_SIZE       (512U  / sizeof(StackType_t))

/** *** TASK PRIORITIES *** */
#define TOUCHGFX_TASK_PRIORITY   (tskIDLE_PRIORITY + 2)
#define APP_TASK_PRIORITY        (tskIDLE_PRIORITY + 1)

// * ************************************************************************ *
// *                                MACROS                                    *
// * ************************************************************************ *

// * ************************************************************************ *
// *                                 TYPES                                    *
// * ************************************************************************ *

// * ************************************************************************ *
// *                             GLOBAL FUNCTIONS                             *
// * ************************************************************************ *
/// @brief Application initialization. Creates the necessary OS ressources and returns.
bool AppInit(void);

/// @brief Main application entry point
void AppRun(void);

#endif // __APP_H__
