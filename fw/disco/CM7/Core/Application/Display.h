//     * **************************************************************** *
//   *                                                                      *
// *          (C) Copyright Matthias Schär, all rights reserved               *
//   *                                                                      *
//     * **************************************************************** *
/// @file Display.h

/// @brief Display initialization and control functions
#ifndef __DISPLAY_H__
#define __DISPLAY_H__

#include <stdint.h>
#include "stm32h7xx_hal.h"

// * ************************************************************************ *
// *                             GLOBAL FUNCTIONS                             *
// * ************************************************************************ *
/// @brief Initializes the display and brings it up
/// @return Extracted state of display running
void DisplayInit(void);

/// @brief Runs display background tasks
void DisplayTask(void);

#endif // __DISPLAY_H__