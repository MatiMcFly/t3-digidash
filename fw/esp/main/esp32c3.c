#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "esp_err.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "host/util/util.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"
#include "os/os_mbuf.h"
#include "uart_parser.h"


#define UART_TXD 4
#define UART_RXD 5
#define UART_RTS       (-1)
#define UART_CTS      (-1)
#define UART_PORT_NUM 1
#define UART_BAUD_RATE 115200
#define UART_TASK_STACK_SIZE 3072
#define UART_BUF_SIZE (1024)

#define BLE_DEVICE_NAME "ESP32C3-NimBLE"
#define BATT_V_MIN_MV 10500
#define BATT_V_MAX_MV 14500

static const char *TAG = "ESP32C3_UART_TO_NUCLEO";
static const char *TAG_BLE = "BLE_GATT";

static uint8_t g_own_addr_type;
static uint16_t g_conn_handle = BLE_HS_CONN_HANDLE_NONE;
static uint16_t gatt_chr_val_handle_water_temp;
static uint16_t gatt_chr_val_handle_outside_temp;
static uint16_t gatt_chr_val_handle_batt_level;
static uint16_t gatt_chr_val_handle_rpm;
static uint16_t gatt_chr_val_handle_turn_signal;
static uint16_t gatt_chr_val_handle_high_beam;
static uint16_t gatt_chr_val_handle_preheat;
static uint16_t gatt_chr_val_handle_tank_level;
static uint16_t gatt_chr_val_handle_batt_mv;

static bool g_notify_water_temp;
static bool g_notify_outside_temp;
static bool g_notify_batt_level;
static bool g_notify_rpm;
static bool g_notify_turn_signal;
static bool g_notify_high_beam;
static bool g_notify_preheat;
static bool g_notify_tank_level;
static bool g_notify_batt_mv;

static int16_t g_water_temp_cC = 0;
static int16_t g_outside_temp_cC = 0;
static uint16_t g_batt_mv = 0;
static uint8_t g_batt_level_percent = 0;
static uint16_t g_rpm = 0;
static uint8_t g_turn_signal = 0;
static uint8_t g_high_beam = 0;
static uint8_t g_preheat = 0;
static uint8_t g_tank_level = 0;

static const char g_desc_water_temp[] = "Water Temperature";
static const char g_desc_outside_temp[] = "Outside Temperature";
static const char g_desc_rpm[] = "RPM";
static const char g_desc_turn_signal[] = "Turn Signal";
static const char g_desc_high_beam[] = "High Beam";
static const char g_desc_preheat[] = "Diesel Preheat";
static const char g_desc_tank_level[] = "Tank Level";
static const char g_desc_batt_mv[] = "Battery Voltage (mV)";

// Custom Car Signals Service UUID: c4e7b1a2-0f3d-4f2a-8d89-0d92b48e7d21
static const ble_uuid128_t gatt_svc_uuid_car = BLE_UUID128_INIT(
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
            return os_mbuf_append(ctxt->om, &g_water_temp_cC, sizeof(g_water_temp_cC));
        }
        if (attr_handle == gatt_chr_val_handle_outside_temp) {
            return os_mbuf_append(ctxt->om, &g_outside_temp_cC, sizeof(g_outside_temp_cC));
        }
        if (attr_handle == gatt_chr_val_handle_batt_level) {
            return os_mbuf_append(ctxt->om, &g_batt_level_percent, sizeof(g_batt_level_percent));
        }
        if (attr_handle == gatt_chr_val_handle_rpm) {
            return os_mbuf_append(ctxt->om, &g_rpm, sizeof(g_rpm));
        }
        if (attr_handle == gatt_chr_val_handle_turn_signal) {
            return os_mbuf_append(ctxt->om, &g_turn_signal, sizeof(g_turn_signal));
        }
        if (attr_handle == gatt_chr_val_handle_high_beam) {
            return os_mbuf_append(ctxt->om, &g_high_beam, sizeof(g_high_beam));
        }
        if (attr_handle == gatt_chr_val_handle_preheat) {
            return os_mbuf_append(ctxt->om, &g_preheat, sizeof(g_preheat));
        }
        if (attr_handle == gatt_chr_val_handle_tank_level) {
            return os_mbuf_append(ctxt->om, &g_tank_level, sizeof(g_tank_level));
        }
        if (attr_handle == gatt_chr_val_handle_batt_mv) {
            return os_mbuf_append(ctxt->om, &g_batt_mv, sizeof(g_batt_mv));
        }
    }

    return BLE_ATT_ERR_UNLIKELY;
}

static const struct ble_gatt_svc_def gatt_svr_svcs[] = {
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

static uint16_t clamp_uint16(int32_t value)
{
    if (value > UINT16_MAX) {
        return UINT16_MAX;
    }
    if (value < 0) {
        return 0;
    }
    return (uint16_t)value;
}

static uint8_t clamp_uint8(int32_t value)
{
    if (value > UINT8_MAX) {
        return UINT8_MAX;
    }
    if (value < 0) {
        return 0;
    }
    return (uint8_t)value;
}

static uint16_t clamp_uint16_range(int32_t value, uint16_t min_value, uint16_t max_value)
{
    if (value < (int32_t)min_value) {
        return min_value;
    }
    if (value > (int32_t)max_value) {
        return max_value;
    }
    return (uint16_t)value;
}

static void ble_app_advertise(void);

static int ble_gap_event(struct ble_gap_event *event, void *arg)
{
    switch (event->type) {
    case BLE_GAP_EVENT_CONNECT:
        if (event->connect.status == 0) {
            g_conn_handle = event->connect.conn_handle;
            ESP_LOGI(TAG_BLE, "Connected; handle=%d", g_conn_handle);
        } else {
            ESP_LOGW(TAG_BLE, "Connect failed; status=%d", event->connect.status);
            ble_app_advertise();
        }
        return 0;
    case BLE_GAP_EVENT_DISCONNECT:
        ESP_LOGI(TAG_BLE, "Disconnected; reason=%d", event->disconnect.reason);
        g_conn_handle = BLE_HS_CONN_HANDLE_NONE;
        g_notify_water_temp = false;
        g_notify_outside_temp = false;
        g_notify_batt_level = false;
        g_notify_rpm = false;
        g_notify_turn_signal = false;
        g_notify_high_beam = false;
        g_notify_preheat = false;
        g_notify_tank_level = false;
        g_notify_batt_mv = false;
        ble_app_advertise();
        return 0;
    case BLE_GAP_EVENT_SUBSCRIBE:
        if (event->subscribe.attr_handle == gatt_chr_val_handle_water_temp) {
            g_notify_water_temp = event->subscribe.cur_notify;
            ESP_LOGI(TAG_BLE, "Water temp notify %s", g_notify_water_temp ? "enabled" : "disabled");
        }
        if (event->subscribe.attr_handle == gatt_chr_val_handle_outside_temp) {
            g_notify_outside_temp = event->subscribe.cur_notify;
            ESP_LOGI(TAG_BLE, "Outside temp notify %s", g_notify_outside_temp ? "enabled" : "disabled");
        }
        if (event->subscribe.attr_handle == gatt_chr_val_handle_batt_level) {
            g_notify_batt_level = event->subscribe.cur_notify;
            ESP_LOGI(TAG_BLE, "Battery notify %s", g_notify_batt_level ? "enabled" : "disabled");
        }
        if (event->subscribe.attr_handle == gatt_chr_val_handle_rpm) {
            g_notify_rpm = event->subscribe.cur_notify;
            ESP_LOGI(TAG_BLE, "RPM notify %s", g_notify_rpm ? "enabled" : "disabled");
        }
        if (event->subscribe.attr_handle == gatt_chr_val_handle_turn_signal) {
            g_notify_turn_signal = event->subscribe.cur_notify;
            ESP_LOGI(TAG_BLE, "Turn signal notify %s", g_notify_turn_signal ? "enabled" : "disabled");
        }
        if (event->subscribe.attr_handle == gatt_chr_val_handle_high_beam) {
            g_notify_high_beam = event->subscribe.cur_notify;
            ESP_LOGI(TAG_BLE, "High beam notify %s", g_notify_high_beam ? "enabled" : "disabled");
        }
        if (event->subscribe.attr_handle == gatt_chr_val_handle_preheat) {
            g_notify_preheat = event->subscribe.cur_notify;
            ESP_LOGI(TAG_BLE, "Preheat notify %s", g_notify_preheat ? "enabled" : "disabled");
        }
        if (event->subscribe.attr_handle == gatt_chr_val_handle_tank_level) {
            g_notify_tank_level = event->subscribe.cur_notify;
            ESP_LOGI(TAG_BLE, "Tank level notify %s", g_notify_tank_level ? "enabled" : "disabled");
        }
        if (event->subscribe.attr_handle == gatt_chr_val_handle_batt_mv) {
            g_notify_batt_mv = event->subscribe.cur_notify;
            ESP_LOGI(TAG_BLE, "Battery mV notify %s", g_notify_batt_mv ? "enabled" : "disabled");
        }
        return 0;
    case BLE_GAP_EVENT_ADV_COMPLETE:
        ble_app_advertise();
        return 0;
    default:
        return 0;
    }
}

static void ble_on_reset(int reason)
{
    ESP_LOGE(TAG_BLE, "Resetting state; reason=%d", reason);
}

static void ble_app_on_sync(void)
{
    int rc = ble_hs_id_infer_auto(0, &g_own_addr_type);
    if (rc != 0) {
        ESP_LOGE(TAG_BLE, "ble_hs_id_infer_auto failed: %d", rc);
        return;
    }
    ble_app_advertise();
}

static void ble_app_advertise(void)
{
    struct ble_hs_adv_fields fields;
    memset(&fields, 0, sizeof(fields));

    fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;
    fields.tx_pwr_lvl_is_present = 1;
    fields.tx_pwr_lvl = BLE_HS_ADV_TX_PWR_LVL_AUTO;
    fields.uuids128 = (ble_uuid128_t *)&gatt_svc_uuid_car;
    fields.num_uuids128 = 1;
    fields.uuids128_is_complete = 1;

    static const ble_uuid16_t adv_uuids16[] = {
        BLE_UUID16_INIT(0x180F),
        BLE_UUID16_INIT(0x181A)
    };
    fields.uuids16 = (ble_uuid16_t *)adv_uuids16;
    fields.num_uuids16 = 2;
    fields.uuids16_is_complete = 1;

    int rc = ble_gap_adv_set_fields(&fields);
    if (rc != 0) {
        ESP_LOGE(TAG_BLE, "ble_gap_adv_set_fields failed: %d", rc);
        return;
    }

    struct ble_hs_adv_fields rsp_fields;
    memset(&rsp_fields, 0, sizeof(rsp_fields));
    rsp_fields.name = (uint8_t *)ble_svc_gap_device_name();
    rsp_fields.name_len = strlen(ble_svc_gap_device_name());
    rsp_fields.name_is_complete = 1;

    rc = ble_gap_adv_rsp_set_fields(&rsp_fields);
    if (rc != 0) {
        ESP_LOGE(TAG_BLE, "ble_gap_adv_rsp_set_fields failed: %d", rc);
        return;
    }

    struct ble_gap_adv_params adv_params;
    memset(&adv_params, 0, sizeof(adv_params));
    adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;

    rc = ble_gap_adv_start(g_own_addr_type, NULL, BLE_HS_FOREVER,
                           &adv_params, ble_gap_event, NULL);
    if (rc != 0) {
        ESP_LOGE(TAG_BLE, "ble_gap_adv_start failed: %d", rc);
    }
}

static void ble_notify_value(uint16_t attr_handle, const void *data, size_t len)
{
    if (data == NULL || len == 0) {
        return;
    }

    if (g_conn_handle == BLE_HS_CONN_HANDLE_NONE || attr_handle == 0) {
        return;
    }

    struct os_mbuf *om = ble_hs_mbuf_from_flat(data, len);
    if (om == NULL) {
        ESP_LOGW(TAG_BLE, "Notify: no mbuf");
        return;
    }

    int rc = ble_gatts_notify_custom(g_conn_handle, attr_handle, om);
    if (rc != 0) {
        os_mbuf_free_chain(om);
        ESP_LOGW(TAG_BLE, "Notify failed: %d", rc);
    }
}

static void ble_notify_water_temp_c(float temp_c)
{
    int32_t cC = (int32_t)(temp_c * 100.0f);
    g_water_temp_cC = clamp_int16(cC);

    if (!g_notify_water_temp) {
        return;
    }

    ble_notify_value(gatt_chr_val_handle_water_temp, &g_water_temp_cC, sizeof(g_water_temp_cC));
}

static void ble_notify_outside_temp_c(float temp_c)
{
    int32_t cC = (int32_t)(temp_c * 100.0f);
    g_outside_temp_cC = clamp_int16(cC);

    if (!g_notify_outside_temp) {
        return;
    }

    ble_notify_value(gatt_chr_val_handle_outside_temp, &g_outside_temp_cC, sizeof(g_outside_temp_cC));
}

static void ble_notify_batt_v(float volts)
{
    int32_t mv = (int32_t)(volts * 1000.0f);
    g_batt_mv = clamp_uint16(mv);
    if (BATT_V_MAX_MV > BATT_V_MIN_MV) {
        int32_t scaled = (int32_t)(g_batt_mv - BATT_V_MIN_MV) * 100;
        int32_t range = (int32_t)(BATT_V_MAX_MV - BATT_V_MIN_MV);
        g_batt_level_percent = clamp_uint8(scaled / range);
    } else {
        g_batt_level_percent = 0;
    }

    if (!g_notify_batt_level) {
        return;
    }

    ble_notify_value(gatt_chr_val_handle_batt_level,
                     &g_batt_level_percent,
                     sizeof(g_batt_level_percent));

    if (g_notify_batt_mv) {
        ble_notify_value(gatt_chr_val_handle_batt_mv, &g_batt_mv, sizeof(g_batt_mv));
    }
}

static void ble_notify_rpm(uint16_t rpm)
{
    g_rpm = rpm;
    if (!g_notify_rpm) {
        return;
    }
    ble_notify_value(gatt_chr_val_handle_rpm, &g_rpm, sizeof(g_rpm));
}

static void ble_notify_turn_signal(uint8_t value)
{
    g_turn_signal = value ? 1 : 0;
    if (!g_notify_turn_signal) {
        return;
    }
    ble_notify_value(gatt_chr_val_handle_turn_signal, &g_turn_signal, sizeof(g_turn_signal));
}

static void ble_notify_high_beam(uint8_t value)
{
    g_high_beam = value ? 1 : 0;
    if (!g_notify_high_beam) {
        return;
    }
    ble_notify_value(gatt_chr_val_handle_high_beam, &g_high_beam, sizeof(g_high_beam));
}

static void ble_notify_preheat(uint8_t value)
{
    g_preheat = value ? 1 : 0;
    if (!g_notify_preheat) {
        return;
    }
    ble_notify_value(gatt_chr_val_handle_preheat, &g_preheat, sizeof(g_preheat));
}

static void ble_notify_tank_level(uint8_t percent)
{
    g_tank_level = clamp_uint8(percent);
    if (!g_notify_tank_level) {
        return;
    }
    ble_notify_value(gatt_chr_val_handle_tank_level, &g_tank_level, sizeof(g_tank_level));
}

static void ble_host_task(void *param)
{
    nimble_port_run();
    nimble_port_freertos_deinit();
}

static void ble_init(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ret = nimble_port_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG_BLE, "nimble_port_init failed: %s", esp_err_to_name(ret));
        return;
    }

    ble_hs_cfg.reset_cb = ble_on_reset;
    ble_hs_cfg.sync_cb = ble_app_on_sync;

    ble_svc_gap_init();
    ble_svc_gatt_init();
    ble_svc_gap_device_name_set(BLE_DEVICE_NAME);

    int rc_gatt = ble_gatts_count_cfg(gatt_svr_svcs);
    if (rc_gatt != 0) {
        ESP_LOGE(TAG_BLE, "ble_gatts_count_cfg failed: %d", rc_gatt);
        return;
    }
    rc_gatt = ble_gatts_add_svcs(gatt_svr_svcs);
    if (rc_gatt != 0) {
        ESP_LOGE(TAG_BLE, "ble_gatts_add_svcs failed: %d", rc_gatt);
        return;
    }

    nimble_port_freertos_init(ble_host_task);
}

static void uart_task_esp_to_nucleo(void *arg)
{
    // Configure UART parameters
    uart_config_t uart_config = {
        .baud_rate = UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };
    // Install UART driver
    ESP_ERROR_CHECK(uart_driver_install(UART_PORT_NUM, UART_BUF_SIZE * 2, 0, 0, NULL, 0));
    // Configure UART pins
    ESP_ERROR_CHECK(uart_param_config(UART_PORT_NUM, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(UART_PORT_NUM, UART_TXD, UART_RXD, UART_RTS, UART_CTS));

    uint8_t data[UART_BUF_SIZE];
    while (1) {
        // Read data from UART
        int len = uart_read_bytes(UART_PORT_NUM, data, UART_BUF_SIZE - 1, 20 / portTICK_PERIOD_MS);
        if (len > 0) {
            data[len] = '\0'; // Null-terminate the string
            ESP_LOGI(TAG, "Received: %s", (char *)data);
            float water_c = g_water_temp_cC / 100.0f;
            float outside_c = g_outside_temp_cC / 100.0f;
            float batt_v = g_batt_mv / 1000.0f;
            int32_t rpm = g_rpm;
            int32_t turn_signal = g_turn_signal;
            int32_t high_beam = g_high_beam;
            int32_t preheat = g_preheat;
            int32_t tank_level = g_tank_level;
            if (parse_uart_values((char *)data, &water_c, &outside_c, &batt_v,
                                  &rpm, &turn_signal, &high_beam, &preheat, &tank_level)) {
                ble_notify_water_temp_c(water_c);
                ble_notify_outside_temp_c(outside_c);
                ble_notify_batt_v(batt_v);
                ble_notify_rpm(clamp_uint16_range(rpm, 0, UINT16_MAX));
                ble_notify_turn_signal(clamp_uint8(turn_signal));
                ble_notify_high_beam(clamp_uint8(high_beam));
                ble_notify_preheat(clamp_uint8(preheat));
                ble_notify_tank_level(clamp_uint8(tank_level));
            } else {
                ESP_LOGW(TAG, "UART parse failed; expected 'WT=23.5,OT=12.1,BV=12.6,RPM=1500,TS=1,HB=0,DP=0,TL=45'");
            }
        }
    }
}

void app_main(void)
{
    ble_init();
    xTaskCreate(uart_task_esp_to_nucleo, "uart_task_esp_to_nucleo", UART_TASK_STACK_SIZE, NULL, 10, NULL);

}
