//     * **************************************************************** *
//   *                                                                      *
// *          (C) Copyright Matthias Schär, all rights reserved               *
//   *                                                                      *
//     * **************************************************************** *
/// @file SensorDataPublisher.c

/// @brief Implementation of the UART -> TouchGFX sensor-data bridge.
///
/// Also provides the strong override of UartReceiverOnSensor() so the
/// receiver module is wired to the GUI just by linking this file.
#include "SensorDataPublisher.h"

#include "UartReceiver.h"
#include "shared.h"

#include "FreeRTOS.h"
#include "queue.h"

#include <stdbool.h>
#include <stddef.h>

// * ************************************************************************ *
// *                                DEFINES                                   *
// * ************************************************************************ *

/// Queue depth (in samples). Sized for ~half a second of bursty
/// telemetry at the nucleo's publication rate; the GUI consumes the
/// queue once per frame so anything more is wasted RAM.
#define SENSOR_DATA_QUEUE_LENGTH  32U

// * ************************************************************************ *
// *                             STATIC VARIABLES                             *
// * ************************************************************************ *

static QueueHandle_t s_queue;

// * ************************************************************************ *
// *                             GLOBAL FUNCTIONS                             *
// * ************************************************************************ *

bool SensorDataPublisherInit(void) {
  if (s_queue != NULL) {
    return true;
  }
  s_queue = xQueueCreate(SENSOR_DATA_QUEUE_LENGTH, sizeof(sensor_data_t));
  return (s_queue != NULL);
}

void SensorDataPublisherPublish(const sensor_data_t *data) {
  if ((s_queue == NULL) || (data == NULL)) {
    return;
  }
  /* Non-blocking: if the GUI is too slow we drop the sample rather
   * than stall the UART receiver task. */
  (void)xQueueSend(s_queue, data, 0);
}

bool SensorDataPublisherTryReceive(sensor_data_t *out) {
  if ((s_queue == NULL) || (out == NULL)) {
    return false;
  }
  return (xQueueReceive(s_queue, out, 0) == pdTRUE);
}

// * ************************************************************************ *
// *                       UART RECEIVER HOOK OVERRIDE                        *
// * ************************************************************************ *

/// Strong override of the weak hook in UartReceiver.c. Runs in the
/// UART receiver task context.
void UartReceiverOnSensor(const sensor_data_t *data) {
  SensorDataPublisherPublish(data);
}
