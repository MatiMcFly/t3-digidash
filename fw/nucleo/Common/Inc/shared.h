#ifndef SHARED_H
#define SHARED_H

#include <stdint.h>

typedef enum {
    SENSOR_ID_WATER_TEMPERATURE = 1,
    SENSOR_ID_BATTERY_VOLTAGE   = 3,
} sensor_id_t;

typedef struct {
    sensor_id_t id;
    int16_t     value;
} sensor_data_t;

#endif /* SHARED_H */
