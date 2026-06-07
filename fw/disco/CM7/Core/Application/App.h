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
// *                             GLOBAL FUNCTIONS                             *
// * ************************************************************************ *
/// @brief Application initialization. Creates the necessary OS ressources and returns.
bool AppInit(void);

/// @brief Main application entry point
void AppRun(void);

#endif // __APP_H__
