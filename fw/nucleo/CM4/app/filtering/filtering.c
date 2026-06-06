#include "filtering.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#ifndef UNIT_TEST
#include "app.h"
#endif

static void    ringbuf_update(int16_t ringbuf[], uint16_t size, uint16_t* index, int16_t value);
static int16_t ringbuf_mean(int16_t ringbuf[], uint16_t size);
static bool    ringbuf_is_consistent(int16_t ringbuf[], uint16_t size);

int16_t filtering_moving_average(int16_t ringbuf[], uint16_t size, uint16_t* index, int16_t value)
{
    ringbuf_update(ringbuf, size, index, value);
    return ringbuf_mean(ringbuf, size);
}

int16_t filtering_debounce(int16_t ringbuf[], uint16_t size, uint16_t* index, int16_t value, int16_t* value_previous)
{
    ringbuf_update(ringbuf, size, index, value);

    if (ringbuf_is_consistent(ringbuf, size)) {
        *value_previous = value;
    }

    return *value_previous;
}

/**
 * @brief Update ring buffer with a new value
 *
 * @param ringbuf -- Ring buffer array
 * @param size    -- Number of elements in the ring buffer
 * @param index   -- Pointer to the current index in the ring buffer
 * @param value   -- New value to add to the ring buffer
 */
static void ringbuf_update(int16_t ringbuf[], uint16_t size, uint16_t* index, int16_t value)
{
    if (size == 0) {
#ifndef UNIT_TEST
        HAL_UART_Transmit(&huart3, (uint8_t*)"ringbuf_update: Size cannot be zero\n", strlen("ringbuf_update: Size cannot be zero\n"), HAL_MAX_DELAY);
#endif
        return; // Invalid size
    }

    if (*index >= size) {
#ifndef UNIT_TEST
        HAL_UART_Transmit(&huart3, (uint8_t*)"ringbuf_update: Invalid index\n", strlen("ringbuf_update: Invalid index\n"), HAL_MAX_DELAY);
#endif
        *index = 0;
        return; // Invalid index
    }

    ringbuf[*index] = value;
    *index          = (*index + 1) % size;
}

/**
 * @brief Calculate the mean of a ring buffer
 *
 * @param ringbuf -- Ring buffer array
 * @param size    -- Number of elements in the ring buffer
 *
 * @return int16_t -- Mean value
 */
static int16_t ringbuf_mean(int16_t ringbuf[], uint16_t size)
{
    int32_t sum = 0;

    if (size == 0) {
#ifndef UNIT_TEST
        HAL_UART_Transmit(&huart3, (uint8_t*)"ringbuf_mean: Size cannot be zero\n", strlen("ringbuf_mean: Size cannot be zero\n"), HAL_MAX_DELAY);
#endif
        return 0; // Invalid size
    }

    for (uint16_t i = 0; i < size; i++) {
        sum += ringbuf[i];
    }

    return (int16_t)(sum / size);
}

/**
 * @brief Check if all values in the ring buffer are the same
 *
 * @param ringbuf -- Ring buffer array
 * @param size    -- Number of elements in the ring buffer
 *
 * @return true -- All elements in the ring buffer have the same value
 * @return false -- Not all elements in the ring buffer have the same value
 */
static bool ringbuf_is_consistent(int16_t ringbuf[], uint16_t size)
{
    if (size == 0) {
#ifndef UNIT_TEST
        HAL_UART_Transmit(&huart3, (uint8_t*)"ringbuf_is_consistent: Size cannot be zero\n", strlen("ringbuf_is_consistent: Size cannot be zero\n"), HAL_MAX_DELAY);
#endif
        return false; // Invalid size
    }

    int16_t value = ringbuf[0];

    for (uint16_t i = 1; i < size; i++) {
        if (ringbuf[i] != value) {
            return false;
        }
    }

    return true;
}
