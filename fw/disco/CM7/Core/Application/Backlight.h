//     * **************************************************************** *
//   *                                                                      *
// *          (C) Copyright Matthias Schär, all rights reserved               *
//   *                                                                      *
//     * **************************************************************** *
/// @file Backlight.h

/// @brief Software-PWM backlight driver for the TPS61169 (PJ12 / BL_CTRL).
///
/// TIM17 fires at 100 kHz (PSC=63, ARR=9 at 64 MHz).
/// 100 ticks per period -> 1 kHz PWM, 1 % resolution (steps 0..100).
///
/// Call sequence:
///   BacklightInit()           -- once, after MX_TIM17_Init()
///   BacklightSetDuty(0..100)  -- any time from the main context
#ifndef __BACKLIGHT_H__
#define __BACKLIGHT_H__

#include <stdint.h>

// * ************************************************************************ *
// *                                DEFINES                                   *
// * ************************************************************************ *

/// Maximum value accepted by BacklightSetDuty() (= 100 %).
#define BACKLIGHT_DUTY_MAX  100U

// * ************************************************************************ *
// *                             GLOBAL FUNCTIONS                             *
// * ************************************************************************ *

/// @brief Start the TIM17-based software PWM and enable the backlight output.
///        Must be called after HAL and MX_TIM17_Init() have completed.
void BacklightInit(void);

/// @brief Set the PWM duty cycle.
/// @param duty  0 = off, 100 = full brightness.  Values > 100 are clamped.
void BacklightSetDuty(uint8_t duty);

#endif /* __BACKLIGHT_H__ */
