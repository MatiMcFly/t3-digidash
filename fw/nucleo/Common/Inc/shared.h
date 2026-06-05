#ifndef SHARED_H
#define SHARED_H

#include <stddef.h>
#include <stdint.h>

typedef enum {
    SENSOR_ID_COOLANT_TEMPERATURE  = 1, // analog signal
    SENSOR_ID_MOTOR_RPM            = 2, // pulsed signal
    SENSOR_ID_BATTERY_VOLTAGE      = 3, // analog signal
    SENSOR_ID_FUEL_LEVEL           = 4, // analog signal
    SENSOR_ID_TURN_SIGNAL          = 5, // binary signal
    SENSOR_ID_HIGH_BEAM            = 6, // binary signal
    SENSOR_ID_OIL_PRESSURE_0_3_BAR = 7, // binary signal
    SENSOR_ID_OIL_PRESSURE_1_8_BAR = 8, // binary signal
} sensor_id_t;

typedef struct {
    sensor_id_t id;
    int16_t     value;
} sensor_data_t;

#define IPC_HSEM_ID 1

extern uint8_t __ipc_mb_struct_start__;
extern uint8_t __ipc_mb_struct_end__;
extern uint8_t __ipc_mb_storage_start__;
extern uint8_t __ipc_mb_storage_end__;

#define IPC_MB_STRUCT_ADDR  (&__ipc_mb_struct_start__)
#define IPC_MB_STORAGE_ADDR (&__ipc_mb_storage_start__)

#define IPC_MB_STRUCT_SIZE  ((size_t)(&__ipc_mb_struct_end__ - &__ipc_mb_struct_start__))
#define IPC_MB_STORAGE_SIZE ((size_t)(&__ipc_mb_storage_end__ - &__ipc_mb_storage_start__))

#endif /* SHARED_H */
