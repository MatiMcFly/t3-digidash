//     * **************************************************************** *
//   *                                                                      *
// *          (C) Copyright Matthias Schär, all rights reserved               *
//   *                                                                      *
//     * **************************************************************** *
/// @file SensorDataPublisher.h

/// @brief Bridge between the UART telemetry receiver (C, FreeRTOS task)
///        and the TouchGFX UI (C++, render task).
///
/// The producer side (UART receiver task) calls SensorDataPublisherPublish()
/// from its @ref UartReceiverOnSensor() override; the consumer side
/// (TouchGFX Model::tick()) drains pending samples via
/// SensorDataPublisherTryReceive() once per frame.
///
/// The transport is a plain FreeRTOS queue. It is intentionally lossy:
/// if the GUI stalls, the producer drops new samples instead of blocking
/// the UART task. Sample IDs are application-level (sensor_id_t), so the
/// most recent value of any sensor is what the UI ends up rendering --
/// missed intermediate samples are not user-visible.
#ifndef __SENSOR_DATA_PUBLISHER_H__
#define __SENSOR_DATA_PUBLISHER_H__

#include "shared.h"

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// * ************************************************************************ *
// *                             GLOBAL FUNCTIONS                             *
// * ************************************************************************ *

/// @brief Create the underlying FreeRTOS queue. Must be called before
///        UartReceiverInit() (so the very first sensor sample produced
///        by the IRQ-driven receiver finds a valid queue) and before
///        the TouchGFX task starts ticking.
/// @return true on success, false on resource-allocation failure.
bool SensorDataPublisherInit(void);

/// @brief Push a sensor sample to the GUI. Safe from any task context.
///        Drops the sample silently if the queue is full -- by design,
///        see file-level note above.
void SensorDataPublisherPublish(const sensor_data_t *data);

/// @brief Pop one sensor sample without blocking. Intended to be called
///        from Model::tick() in a loop until it returns false.
/// @param out  Receives the popped sample on success.
/// @return true if a sample was dequeued, false if the queue is empty
///         (or not initialised yet).
bool SensorDataPublisherTryReceive(sensor_data_t *out);

#ifdef __cplusplus
}
#endif

#endif /* __SENSOR_DATA_PUBLISHER_H__ */
