#ifndef ACQUISITION_H
#define ACQUISITION_H

#include <stdint.h>

#define MEASUREMENT_PERIOD_MS 100

#define ADC1_BUFFER_SIZE 3
extern volatile uint16_t adc1_buffer[ADC1_BUFFER_SIZE];

void acquisition_binary_sensors(void);
void acquisition_pulse_sensors(void);

#endif /* ACQUISITION_H */
