#pragma once

#include <stdint.h>

#include "host/ble_hs.h"

extern const struct ble_gatt_svc_def gatt_db_svcs[];

extern uint16_t gatt_chr_val_handle_water_temp;
extern uint16_t gatt_chr_val_handle_outside_temp;
extern uint16_t gatt_chr_val_handle_batt_level;
extern uint16_t gatt_chr_val_handle_rpm;
extern uint16_t gatt_chr_val_handle_turn_signal;
extern uint16_t gatt_chr_val_handle_high_beam;
extern uint16_t gatt_chr_val_handle_preheat;
extern uint16_t gatt_chr_val_handle_tank_level;
extern uint16_t gatt_chr_val_handle_batt_mv;

extern const ble_uuid128_t gatt_svc_uuid_car;
