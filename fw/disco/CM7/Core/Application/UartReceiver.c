//     * **************************************************************** *
//   *                                                                      *
// *          (C) Copyright Matthias Schär, all rights reserved               *
//   *                                                                      *
//     * **************************************************************** *
/// @file UartReceiver.c

/// @brief Interrupt-driven USART2 telemetry receiver — implementation.
///
/// The wire format produced by the nucleo (see
/// fw/nucleo/CM7/app/publication/publication.c) is a continuous ASCII
/// stream of "<id>:<value>;" tokens with no newlines or whitespace, e.g.
///   1:246;3:1320;4:556;5:1;6:0;7:1;8:0;1:246;3:1320;...
/// id is unsigned (one of sensor_id_t), value is signed and fits in int16.
#include "UartReceiver.h"

#include "TaskDefinitions.h"
#include "main.h"
#include "shared.h"
#include "stm32h7xx_hal.h"

#include "FreeRTOS.h"
#include "stream_buffer.h"
#include "task.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// * ************************************************************************ *
// *                                DEFINES                                   *
// * ************************************************************************ *

/// Stream buffer between the USART2 IRQ and the receiver task. Sized to
/// absorb several frames of burst traffic without the IRQ ever having to
/// wait. Trigger level is 1 byte: the task wakes as soon as any byte is
/// available, so end-to-end latency from "byte received" to "sensor
/// dispatched" is dominated by one task switch per token.
#define UART_RX_STREAM_SIZE      512U
#define UART_RX_STREAM_TRIGGER   1U

/// Lowest / highest valid sensor_id_t value. Validated before dispatch to
/// keep stray bytes from injecting garbage IDs into the consumer.
#define UART_RX_SENSOR_ID_MIN    SENSOR_ID_COOLANT_TEMPERATURE
#define UART_RX_SENSOR_ID_MAX    SENSOR_ID_OIL_PRESSURE_1_8_BAR

/// Magnitude limits for the signed int16 value field.
#define UART_RX_VALUE_MAX_POS    32767U
#define UART_RX_VALUE_MAX_NEG    32768U

// * ************************************************************************ *
// *                                 TYPES                                    *
// * ************************************************************************ *

typedef enum {
  PARSE_ID,      ///< accumulating digits before ':'
  PARSE_VALUE,   ///< accumulating optional '-' and digits before ';'
  PARSE_RESYNC,  ///< drop bytes until the next ';' to recover sync
} parse_state_t;

typedef struct {
  parse_state_t state;
  uint32_t      idAcc;
  uint32_t      valAcc;         ///< magnitude only
  bool          valNeg;
  bool          idHaveDigits;
  bool          valHaveDigits;
} parser_t;

// * ************************************************************************ *
// *                             EXTERNAL HANDLES                             *
// * ************************************************************************ *

/* Provided by CubeMX in main.c. */
extern UART_HandleTypeDef huart2;

// * ************************************************************************ *
// *                             STATIC VARIABLES                             *
// * ************************************************************************ *

static StreamBufferHandle_t s_rxStream;

/// HAL_UART_Receive_IT writes the next received byte here. Single-byte
/// mode is used so every byte triggers an RxCpltCallback; this gives the
/// receiver immediate visibility of token terminators without any timeout.
static uint8_t              s_rxByte;

// * ************************************************************************ *
// *                              LOCAL FUNCTIONS                             *
// * ************************************************************************ *

static inline void ParserReset(parser_t *p) {
  p->state         = PARSE_ID;
  p->idAcc         = 0U;
  p->valAcc        = 0U;
  p->valNeg        = false;
  p->idHaveDigits  = false;
  p->valHaveDigits = false;
}

static inline bool IsDigit(uint8_t b) {
  return (b >= '0') && (b <= '9');
}

static void ParserStepId(parser_t *p, uint8_t byte) {
  if (IsDigit(byte)) {
    p->idAcc        = (p->idAcc * 10U) + (uint32_t)(byte - '0');
    p->idHaveDigits = true;
    if (p->idAcc > UART_RX_SENSOR_ID_MAX) {
      /* No valid id can be that large; resync. */
      p->state = PARSE_RESYNC;
    }
  } else if (byte == ':') {
    p->state = p->idHaveDigits ? PARSE_VALUE : PARSE_RESYNC;
  } else {
    /* Anything else (incl. stray ';') means we're not aligned. */
    p->state = PARSE_RESYNC;
  }
}

static void ParserStepValue(parser_t *p, uint8_t byte) {
  if (IsDigit(byte)) {
    p->valAcc        = (p->valAcc * 10U) + (uint32_t)(byte - '0');
    p->valHaveDigits = true;
    /* Magnitude must fit in int16 (max 32768 for INT16_MIN). */
    if (p->valAcc > UART_RX_VALUE_MAX_NEG) {
      p->state = PARSE_RESYNC;
    }
    return;
  }

  if ((byte == '-') && !p->valHaveDigits && !p->valNeg) {
    p->valNeg = true;
    return;
  }

  if (byte == ';') {
    const uint32_t valLimit = p->valNeg ? UART_RX_VALUE_MAX_NEG
                                        : UART_RX_VALUE_MAX_POS;
    const bool     ok       = p->valHaveDigits
                              && (p->idAcc >= UART_RX_SENSOR_ID_MIN)
                              && (p->idAcc <= UART_RX_SENSOR_ID_MAX)
                              && (p->valAcc <= valLimit);
    if (ok) {
      sensor_data_t d;
      d.id    = (sensor_id_t)p->idAcc;
      d.value = p->valNeg ? (int16_t)-(int32_t)p->valAcc
                          : (int16_t)p->valAcc;
      UartReceiverOnSensor(&d);
    }
    /* Whether ok or not, ';' aligns us to the next token. */
    ParserReset(p);
    return;
  }

  p->state = PARSE_RESYNC;
}

static void ParserStepResync(parser_t *p, uint8_t byte) {
  if (byte == ';') {
    ParserReset(p);
  }
}

static void ParserStep(parser_t *p, uint8_t byte) {
  switch (p->state) {
    case PARSE_ID:     ParserStepId(p, byte);     break;
    case PARSE_VALUE:  ParserStepValue(p, byte);  break;
    case PARSE_RESYNC:
    default:           ParserStepResync(p, byte); break;
  }
}

static void UartReceiverTask(void *arg) {
  (void)arg;

  parser_t parser;
  ParserReset(&parser);

  for (;;) {
    uint8_t      byte;
    const size_t got = xStreamBufferReceive(s_rxStream,
                                            &byte,
                                            1U,
                                            portMAX_DELAY);
    if (got == 0U) {
      continue;
    }
    ParserStep(&parser, byte);
  }
}

// * ************************************************************************ *
// *                             GLOBAL FUNCTIONS                             *
// * ************************************************************************ *

bool UartReceiverInit(void) {
  s_rxStream = xStreamBufferCreate(UART_RX_STREAM_SIZE,
                                   UART_RX_STREAM_TRIGGER);
  if (s_rxStream == NULL) {
    return false;
  }

  /* Arm the first IT receive. The RxCpltCallback re-arms after each byte. */
  if (HAL_UART_Receive_IT(&huart2, &s_rxByte, 1) != HAL_OK) {
    return false;
  }

  /* USART2 receiver task */
  const BaseType_t ok = xTaskCreate(UartReceiverTask,
                                    "UartRx",
                                    UART_RX_TASK_STACK_SIZE,
                                    NULL,
                                    UART_RX_TASK_PRIORITY,
                                    NULL);
  return (ok == pdPASS);
}

/// Default hook — does nothing. Override in application code.
__attribute__((weak))
void UartReceiverOnSensor(const sensor_data_t *data) {
  (void)data;
}

// * ************************************************************************ *
// *                          HAL UART CALLBACKS                              *
// * ************************************************************************ *

/* Both callbacks are invoked from HAL_UART_IRQHandler context (USART2_IRQn).
 * They guard on the instance because HAL routes ALL UART completions through
 * these single weak symbols; another driver in the project (e.g. USART1) may
 * legitimately want its own callbacks. Today USART1 is unused, but the
 * guard costs nothing and keeps the module self-contained. */

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
  if (huart->Instance != USART2) {
    return;
  }

  BaseType_t higherPrioWoken = pdFALSE;
  (void)xStreamBufferSendFromISR(s_rxStream,
                                 &s_rxByte,
                                 1U,
                                 &higherPrioWoken);

  /* Re-arm immediately so we don't miss the next byte. If this fails the
   * ErrorCallback path will re-arm. */
  (void)HAL_UART_Receive_IT(huart, &s_rxByte, 1);

  portYIELD_FROM_ISR(higherPrioWoken);
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart) {
  if (huart->Instance != USART2) {
    return;
  }

  /* Framing / noise / overrun errors abort the IT transfer. Clear by
   * re-arming; the offending byte is already lost. */
  (void)HAL_UART_Receive_IT(huart, &s_rxByte, 1);
}
