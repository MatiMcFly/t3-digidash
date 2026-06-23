#include "car_signals.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>

#include "esp_log.h"

#include "ble_app.h"
#include "gatt_db.h"

#define BATT_V_MIN_MV 10500
#define BATT_V_MAX_MV 14500

static const char *TAG = "CAR_SIGNALS";

static bool g_notify_water_temp;
static bool g_notify_rpm;
static bool g_notify_batt_level;
static bool g_notify_batt_mv;
static bool g_notify_tank_level;
static bool g_notify_turn_signal;
static bool g_notify_high_beam;
static bool g_notify_oil_pressure_3b;
static bool g_notify_oil_pressure_18b;

static int16_t g_water_temp_cC = 0;
static uint16_t g_rpm = 0;
static uint16_t g_batt_mv = 0;
static uint16_t g_tank_level = 0;
static bool g_turn_signal = false;
static bool g_high_beam = false;
static bool g_oil_pressure_switch_3b = false;
static bool g_oil_pressure_switch_18b = false;
static char g_tank_level_str[16] = "0.0";

static int16_t clamp_int16(int32_t value)
{
    if (value > INT16_MAX) {
        return INT16_MAX;
    }
    if (value < INT16_MIN) {
        return INT16_MIN;
    }
    return (int16_t)value;
}


void car_signals_on_disconnect(void)
{
    g_notify_water_temp = false;
    g_notify_batt_level = false;
    g_notify_rpm = false;
    g_notify_turn_signal = false;
    g_notify_high_beam = false;
    g_notify_oil_pressure_3b = false;
    g_notify_oil_pressure_18b = false;
    g_notify_tank_level = false;
    g_notify_batt_mv = false;
}

void car_signals_on_subscribe(uint16_t attr_handle, bool notify_enabled)
{
    if (attr_handle == gatt_chr_val_handle_water_temp) {
        g_notify_water_temp = notify_enabled;
        ESP_LOGI(TAG, "Water temp notify %s", g_notify_water_temp ? "enabled" : "disabled");
    }
    if (attr_handle == gatt_chr_val_handle_batt_level) {
        g_notify_batt_level = notify_enabled;
        ESP_LOGI(TAG, "Battery notify %s", g_notify_batt_level ? "enabled" : "disabled");
    }
    if (attr_handle == gatt_chr_val_handle_rpm) {
        g_notify_rpm = notify_enabled;
        ESP_LOGI(TAG, "RPM notify %s", g_notify_rpm ? "enabled" : "disabled");
    }
    if (attr_handle == gatt_chr_val_handle_turn_signal) {
        g_notify_turn_signal = notify_enabled;
        ESP_LOGI(TAG, "Turn signal notify %s", g_notify_turn_signal ? "enabled" : "disabled");
    }
    if (attr_handle == gatt_chr_val_handle_high_beam) {
        g_notify_high_beam = notify_enabled;
        ESP_LOGI(TAG, "High beam notify %s", g_notify_high_beam ? "enabled" : "disabled");
    }
    if (attr_handle == gatt_chr_val_handle_oil_pressure_3b) {
        g_notify_oil_pressure_3b = notify_enabled;
        ESP_LOGI(TAG, "Oil pressure 3B notify %s", g_notify_oil_pressure_3b ? "enabled" : "disabled");
    }
    if (attr_handle == gatt_chr_val_handle_oil_pressure_18b) {
        g_notify_oil_pressure_18b = notify_enabled;
        ESP_LOGI(TAG, "Oil pressure 18B notify %s", g_notify_oil_pressure_18b ? "enabled" : "disabled");
    }
    if (attr_handle == gatt_chr_val_handle_tank_level) {
        g_notify_tank_level = notify_enabled;
        ESP_LOGI(TAG, "Tank level notify %s", g_notify_tank_level ? "enabled" : "disabled");
    }
    if (attr_handle == gatt_chr_val_handle_batt_mv) {
        g_notify_batt_mv = notify_enabled;
        ESP_LOGI(TAG, "Battery mV notify %s", g_notify_batt_mv ? "enabled" : "disabled");
    }
}

void car_signals_set_water_temp_c(int16_t temp_c)
{
    g_water_temp_cC = temp_c;
    if (!g_notify_water_temp) {
        return;
    }

    ble_app_notify(gatt_chr_val_handle_water_temp, &g_water_temp_cC, sizeof(g_water_temp_cC));
}

void car_signals_set_batt_v(uint16_t volts){
    g_batt_mv = volts;
    if (!g_notify_batt_mv) {
        return;
    }
    ble_app_notify(gatt_chr_val_handle_batt_mv, &g_batt_mv, sizeof(g_batt_mv));
}

void car_signals_set_rpm(uint16_t rpm)
{
    g_rpm = rpm;
    if (!g_notify_rpm) {
        return;
    }
    ble_app_notify(gatt_chr_val_handle_rpm, &g_rpm, sizeof(g_rpm));
}

void car_signals_set_tank_level(uint16_t value)
{
    g_tank_level = value;
    if (!g_notify_tank_level) {
        return;
    }
    ble_app_notify(gatt_chr_val_handle_tank_level, g_tank_level_str, strlen(g_tank_level_str));
}

void car_signals_set_turn_signal(bool value)
{
    g_turn_signal = value;
    if (!g_notify_turn_signal) {
        return;
    }
    ble_app_notify(gatt_chr_val_handle_turn_signal, &g_turn_signal, sizeof(g_turn_signal));
}

void car_signals_set_high_beam(bool value)
{
    g_high_beam = value;
    if (!g_notify_high_beam) {
        return;
    }
    ble_app_notify(gatt_chr_val_handle_high_beam, &g_high_beam, sizeof(g_high_beam));
}

void car_signals_oil_pressure_3b(bool value)
{
    g_oil_pressure_switch_3b = value;
    if (!g_notify_oil_pressure_3b) {
        return;
    }
    ble_app_notify(gatt_chr_val_handle_oil_pressure_3b, &g_oil_pressure_switch_3b, sizeof(g_oil_pressure_switch_3b));
}

void car_signals_oil_pressure_18b(bool value)
{
    g_oil_pressure_switch_18b = value;
    if (!g_notify_oil_pressure_18b) {
        return;
    }
    ble_app_notify(gatt_chr_val_handle_oil_pressure_18b, &g_oil_pressure_switch_18b, sizeof(g_oil_pressure_switch_18b));
}


int16_t car_signals_get_water_temp_cC(void)
{
    return g_water_temp_cC;
}


uint16_t car_signals_get_batt_mv(void)
{
    return g_batt_mv;
}


uint16_t car_signals_get_rpm(void)
{
    return g_rpm;
}

bool car_signals_get_turn_signal(void)
{
    return g_turn_signal;
}

bool car_signals_get_high_beam(void)
{
    return g_high_beam;
}

bool car_signals_get_oil_pressure_3b(void)
{
    return g_oil_pressure_switch_3b;
}

bool car_signals_get_oil_pressure_18b(void)
{
    return g_oil_pressure_switch_18b;
}

uint16_t car_signals_get_tank_level(void)
{
    return g_tank_level;
}

const char *car_signals_get_tank_level_str(void)
{
    return g_tank_level_str;
}
