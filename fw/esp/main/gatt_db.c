#include "gatt_db.h"

#include <string.h>

#include "os/os_mbuf.h"

#include "car_signals.h"

static const char g_desc_water_temp[] = "Water Temperature";
static const char g_desc_outside_temp[] = "Outside Temperature";
static const char g_desc_rpm[] = "RPM";
static const char g_desc_turn_signal[] = "Turn Signal";
static const char g_desc_high_beam[] = "High Beam";
static const char g_desc_preheat[] = "Diesel Preheat";
static const char g_desc_tank_level[] = "Tank Level";
static const char g_desc_batt_mv[] = "Battery Voltage (mV)";

// Custom Car Signals Service UUID: c4e7b1a2-0f3d-4f2a-8d89-0d92b48e7d21
const ble_uuid128_t gatt_svc_uuid_car = BLE_UUID128_INIT(
    0x21, 0x7d, 0x8e, 0xb4, 0x92, 0x0d, 0x89, 0x8d,
    0x2a, 0x4f, 0x3d, 0x0f, 0xa2, 0xb1, 0xe7, 0xc4);

// Standard Environmental Sensing Service (0x181A) and Temperature (0x2A6E)
static const ble_uuid16_t gatt_svc_uuid_env = BLE_UUID16_INIT(0x181A);
static const ble_uuid16_t gatt_chr_uuid_temp = BLE_UUID16_INIT(0x2A6E);

// Standard Battery Service (0x180F) and Battery Level (0x2A19)
static const ble_uuid16_t gatt_svc_uuid_batt = BLE_UUID16_INIT(0x180F);
static const ble_uuid16_t gatt_chr_uuid_batt_level = BLE_UUID16_INIT(0x2A19);

// Custom Car Signals characteristics
static const ble_uuid128_t gatt_chr_uuid_rpm = BLE_UUID128_INIT(
    0x21, 0x7d, 0x8e, 0xb4, 0x92, 0x0d, 0x89, 0x8d,
    0x2a, 0x4f, 0x3d, 0x0f, 0xa6, 0xb1, 0xe7, 0xc4);
static const ble_uuid128_t gatt_chr_uuid_turn_signal = BLE_UUID128_INIT(
    0x21, 0x7d, 0x8e, 0xb4, 0x92, 0x0d, 0x89, 0x8d,
    0x2a, 0x4f, 0x3d, 0x0f, 0xa7, 0xb1, 0xe7, 0xc4);
static const ble_uuid128_t gatt_chr_uuid_high_beam = BLE_UUID128_INIT(
    0x21, 0x7d, 0x8e, 0xb4, 0x92, 0x0d, 0x89, 0x8d,
    0x2a, 0x4f, 0x3d, 0x0f, 0xa8, 0xb1, 0xe7, 0xc4);
static const ble_uuid128_t gatt_chr_uuid_preheat = BLE_UUID128_INIT(
    0x21, 0x7d, 0x8e, 0xb4, 0x92, 0x0d, 0x89, 0x8d,
    0x2a, 0x4f, 0x3d, 0x0f, 0xa9, 0xb1, 0xe7, 0xc4);
static const ble_uuid128_t gatt_chr_uuid_tank_level = BLE_UUID128_INIT(
    0x21, 0x7d, 0x8e, 0xb4, 0x92, 0x0d, 0x89, 0x8d,
    0x2a, 0x4f, 0x3d, 0x0f, 0xaa, 0xb1, 0xe7, 0xc4);
static const ble_uuid128_t gatt_chr_uuid_batt_mv = BLE_UUID128_INIT(
    0x21, 0x7d, 0x8e, 0xb4, 0x92, 0x0d, 0x89, 0x8d,
    0x2a, 0x4f, 0x3d, 0x0f, 0xab, 0xb1, 0xe7, 0xc4);

uint16_t gatt_chr_val_handle_water_temp;
uint16_t gatt_chr_val_handle_outside_temp;
uint16_t gatt_chr_val_handle_batt_level;
uint16_t gatt_chr_val_handle_rpm;
uint16_t gatt_chr_val_handle_turn_signal;
uint16_t gatt_chr_val_handle_high_beam;
uint16_t gatt_chr_val_handle_preheat;
uint16_t gatt_chr_val_handle_tank_level;
uint16_t gatt_chr_val_handle_batt_mv;

static int gatt_svr_chr_access(uint16_t conn_handle, uint16_t attr_handle,
                               struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    if (ctxt->op == BLE_GATT_ACCESS_OP_READ_DSC) {
        const char *desc = (const char *)arg;
        if (desc == NULL) {
            return BLE_ATT_ERR_UNLIKELY;
        }
        return os_mbuf_append(ctxt->om, desc, strlen(desc));
    }

    if (ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR) {
        if (attr_handle == gatt_chr_val_handle_water_temp) {
            int16_t value = car_signals_get_water_temp_cC();
            return os_mbuf_append(ctxt->om, &value, sizeof(value));
        }
        if (attr_handle == gatt_chr_val_handle_outside_temp) {
            int16_t value = car_signals_get_outside_temp_cC();
            return os_mbuf_append(ctxt->om, &value, sizeof(value));
        }
        if (attr_handle == gatt_chr_val_handle_batt_level) {
            uint8_t value = car_signals_get_batt_level_percent();
            return os_mbuf_append(ctxt->om, &value, sizeof(value));
        }
        if (attr_handle == gatt_chr_val_handle_rpm) {
            uint16_t value = car_signals_get_rpm();
            return os_mbuf_append(ctxt->om, &value, sizeof(value));
        }
        if (attr_handle == gatt_chr_val_handle_turn_signal) {
            uint8_t value = car_signals_get_turn_signal();
            return os_mbuf_append(ctxt->om, &value, sizeof(value));
        }
        if (attr_handle == gatt_chr_val_handle_high_beam) {
            uint8_t value = car_signals_get_high_beam();
            return os_mbuf_append(ctxt->om, &value, sizeof(value));
        }
        if (attr_handle == gatt_chr_val_handle_preheat) {
            uint8_t value = car_signals_get_preheat();
            return os_mbuf_append(ctxt->om, &value, sizeof(value));
        }
        if (attr_handle == gatt_chr_val_handle_tank_level) {
            const char *value = car_signals_get_tank_level_str();
            return os_mbuf_append(ctxt->om, value, strlen(value));
        }
        if (attr_handle == gatt_chr_val_handle_batt_mv) {
            uint16_t value = car_signals_get_batt_mv();
            return os_mbuf_append(ctxt->om, &value, sizeof(value));
        }
    }

    return BLE_ATT_ERR_UNLIKELY;
}

const struct ble_gatt_svc_def gatt_db_svcs[] = {
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &gatt_svc_uuid_env.u,
        .characteristics = (struct ble_gatt_chr_def[]) {
            {
                .uuid = &gatt_chr_uuid_temp.u,
                .access_cb = gatt_svr_chr_access,
                .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
                .val_handle = &gatt_chr_val_handle_water_temp,
                .descriptors = (struct ble_gatt_dsc_def[]) {
                    {
                        .uuid = BLE_UUID16_DECLARE(0x2901),
                        .att_flags = BLE_ATT_F_READ,
                        .access_cb = gatt_svr_chr_access,
                        .arg = (void *)g_desc_water_temp,
                    },
                    { 0 }
                },
            },
            {
                .uuid = &gatt_chr_uuid_temp.u,
                .access_cb = gatt_svr_chr_access,
                .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
                .val_handle = &gatt_chr_val_handle_outside_temp,
                .descriptors = (struct ble_gatt_dsc_def[]) {
                    {
                        .uuid = BLE_UUID16_DECLARE(0x2901),
                        .att_flags = BLE_ATT_F_READ,
                        .access_cb = gatt_svr_chr_access,
                        .arg = (void *)g_desc_outside_temp,
                    },
                    { 0 }
                },
            },
            { 0 }
        },
    },
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &gatt_svc_uuid_batt.u,
        .characteristics = (struct ble_gatt_chr_def[]) {
            {
                .uuid = &gatt_chr_uuid_batt_level.u,
                .access_cb = gatt_svr_chr_access,
                .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
                .val_handle = &gatt_chr_val_handle_batt_level,
            },
            { 0 }
        },
    },
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &gatt_svc_uuid_car.u,
        .characteristics = (struct ble_gatt_chr_def[]) {
            {
                .uuid = &gatt_chr_uuid_rpm.u,
                .access_cb = gatt_svr_chr_access,
                .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
                .val_handle = &gatt_chr_val_handle_rpm,
                .descriptors = (struct ble_gatt_dsc_def[]) {
                    {
                        .uuid = BLE_UUID16_DECLARE(0x2901),
                        .att_flags = BLE_ATT_F_READ,
                        .access_cb = gatt_svr_chr_access,
                        .arg = (void *)g_desc_rpm,
                    },
                    { 0 }
                },
            },
            {
                .uuid = &gatt_chr_uuid_turn_signal.u,
                .access_cb = gatt_svr_chr_access,
                .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
                .val_handle = &gatt_chr_val_handle_turn_signal,
                .descriptors = (struct ble_gatt_dsc_def[]) {
                    {
                        .uuid = BLE_UUID16_DECLARE(0x2901),
                        .att_flags = BLE_ATT_F_READ,
                        .access_cb = gatt_svr_chr_access,
                        .arg = (void *)g_desc_turn_signal,
                    },
                    { 0 }
                },
            },
            {
                .uuid = &gatt_chr_uuid_high_beam.u,
                .access_cb = gatt_svr_chr_access,
                .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
                .val_handle = &gatt_chr_val_handle_high_beam,
                .descriptors = (struct ble_gatt_dsc_def[]) {
                    {
                        .uuid = BLE_UUID16_DECLARE(0x2901),
                        .att_flags = BLE_ATT_F_READ,
                        .access_cb = gatt_svr_chr_access,
                        .arg = (void *)g_desc_high_beam,
                    },
                    { 0 }
                },
            },
            {
                .uuid = &gatt_chr_uuid_preheat.u,
                .access_cb = gatt_svr_chr_access,
                .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
                .val_handle = &gatt_chr_val_handle_preheat,
                .descriptors = (struct ble_gatt_dsc_def[]) {
                    {
                        .uuid = BLE_UUID16_DECLARE(0x2901),
                        .att_flags = BLE_ATT_F_READ,
                        .access_cb = gatt_svr_chr_access,
                        .arg = (void *)g_desc_preheat,
                    },
                    { 0 }
                },
            },
            {
                .uuid = &gatt_chr_uuid_tank_level.u,
                .access_cb = gatt_svr_chr_access,
                .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
                .val_handle = &gatt_chr_val_handle_tank_level,
                .descriptors = (struct ble_gatt_dsc_def[]) {
                    {
                        .uuid = BLE_UUID16_DECLARE(0x2901),
                        .att_flags = BLE_ATT_F_READ,
                        .access_cb = gatt_svr_chr_access,
                        .arg = (void *)g_desc_tank_level,
                    },
                    { 0 }
                },
            },
            {
                .uuid = &gatt_chr_uuid_batt_mv.u,
                .access_cb = gatt_svr_chr_access,
                .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
                .val_handle = &gatt_chr_val_handle_batt_mv,
                .descriptors = (struct ble_gatt_dsc_def[]) {
                    {
                        .uuid = BLE_UUID16_DECLARE(0x2901),
                        .att_flags = BLE_ATT_F_READ,
                        .access_cb = gatt_svr_chr_access,
                        .arg = (void *)g_desc_batt_mv,
                    },
                    { 0 }
                },
            },
            { 0 }
        },
    },
    { 0 }
};
