//     * **************************************************************** *
//   *                                                                      *
// *          (C) Copyright Matthias Schär, all rights reserved               *
//   *                                                                      *
//     * **************************************************************** *
/// @file UartReceiver.h

/// @brief Interrupt-driven UART telemetry-frame receiver (USART2).
///
/// USART2 is configured by CubeMX (PD5/PD6, 115200 8N1). This module
/// enables USART2_IRQn, arms HAL_UART_Receive_IT in single-byte mode,
/// pushes received bytes from the IRQ context into a stream buffer, and
/// runs a task that decodes the nucleo-side wire format
///
///     <id>:<value>;<id>:<value>;...
///
/// (compact ASCII, no newlines, see fw/nucleo/CM7/app/publication/publication.c)
/// into sensor_data_t records. Each completed record is dispatched via
/// UartReceiverOnSensor(). Malformed tokens are dropped and parsing
/// resyncs at the next ';'.
///
/// Call sequence:
///   UartReceiverInit()        -- once, after MX_USART2_UART_Init() and
///                                BEFORE vTaskStartScheduler().
#ifndef __UART_RECEIVER_H__
#define __UART_RECEIVER_H__

#include "shared.h"

#include <stdbool.h>

// * ************************************************************************ *
// *                             GLOBAL FUNCTIONS                             *
// * ************************************************************************ *

/// @brief Create the receiver task, configure USART2_IRQn and arm the
///        first interrupt-mode reception. Must be called after
///        MX_USART2_UART_Init() and before vTaskStartScheduler().
/// @return true on success, false on resource-allocation failure.
bool UartReceiverInit(void);

/// @brief Hook called from the receiver task whenever a complete
///        "<id>:<value>;" token has been parsed and validated.
///        Default implementation is empty; override in application code
///        to consume the data (e.g. forward to UI or filter task).
///
/// The pointer is valid only for the duration of the call.
///
/// @param data  Decoded sensor sample (id is guaranteed to be one of
///              the values defined in sensor_id_t).
void UartReceiverOnSensor(const sensor_data_t *data);

#endif /* __UART_RECEIVER_H__ */
