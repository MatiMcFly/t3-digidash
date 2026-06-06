#ifndef FILTERING_H
#define FILTERING_H

#include <stdint.h>

int16_t filtering_moving_average(int16_t ringbuf[], uint16_t size, uint16_t* index, int16_t value);
int16_t filtering_debounce(int16_t ringbuf[], uint16_t size, uint16_t* index, int16_t value, int16_t* value_previous);

#endif /* FILTERING_H */
