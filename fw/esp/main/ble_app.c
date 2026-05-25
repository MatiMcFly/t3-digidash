#include "ble_app.h"

#include <string.h>

#include "esp_err.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"
#include "os/os_mbuf.h"

#include "car_signals.h"
#include "gatt_db.h"
#include "remotexy_protocol.h"

#define BLE_DEVICE_NAME "ESP32C3-NimBLE"

static const char *TAG_BLE = "BLE_GATT";

static uint8_t g_own_addr_type;
static uint16_t g_conn_handle = BLE_HS_CONN_HANDLE_NONE;
static bool g_uart_notify_enabled;

static void ble_app_advertise(void);

static int ble_gap_event(struct ble_gap_event *event, void *arg)
{
    switch (event->type) {
    case BLE_GAP_EVENT_CONNECT:
        if (event->connect.status == 0) {
            g_conn_handle = event->connect.conn_handle;
            ESP_LOGI(TAG_BLE, "Connected; handle=%d", g_conn_handle);
            remotexy_set_connected(true);
        } else {
            ESP_LOGW(TAG_BLE, "Connect failed; status=%d", event->connect.status);
            ble_app_advertise();
        }
        return 0;
    case BLE_GAP_EVENT_DISCONNECT:
        ESP_LOGI(TAG_BLE, "Disconnected; reason=%d", event->disconnect.reason);
        g_conn_handle = BLE_HS_CONN_HANDLE_NONE;
        g_uart_notify_enabled = false;
        remotexy_set_connected(false);
        remotexy_set_notify_enabled(false);
        car_signals_on_disconnect();
        ble_app_advertise();
        return 0;
    case BLE_GAP_EVENT_SUBSCRIBE:
        if (event->subscribe.attr_handle == gatt_chr_val_handle_uart_tx) {
            g_uart_notify_enabled = event->subscribe.cur_notify;
            remotexy_set_notify_enabled(g_uart_notify_enabled);
        }
        car_signals_on_subscribe(event->subscribe.attr_handle, event->subscribe.cur_notify);
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
        BLE_UUID16_INIT(0xFFE0)
    };
    fields.uuids16 = (ble_uuid16_t *)adv_uuids16;
    fields.num_uuids16 = 1;
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

void ble_app_notify(uint16_t attr_handle, const void *data, size_t len)
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
    } else if (attr_handle == gatt_chr_val_handle_uart_tx) {
        ESP_LOGI(TAG_BLE, "UART notify sent (%u bytes)", (unsigned)len);
    }
}

uint16_t ble_app_conn_handle_get(void)
{
    return g_conn_handle;
}

static void ble_host_task(void *param)
{
    nimble_port_run();
    nimble_port_freertos_deinit();
}

void ble_app_init(void)
{
    g_uart_notify_enabled = false;
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

    int rc_gatt = ble_gatts_count_cfg(gatt_db_svcs);
    if (rc_gatt != 0) {
        ESP_LOGE(TAG_BLE, "ble_gatts_count_cfg failed: %d", rc_gatt);
        return;
    }
    rc_gatt = ble_gatts_add_svcs(gatt_db_svcs);
    if (rc_gatt != 0) {
        ESP_LOGE(TAG_BLE, "ble_gatts_add_svcs failed: %d", rc_gatt);
        return;
    }

    nimble_port_freertos_init(ble_host_task);
}

bool ble_app_uart_notify_enabled(void)
{
    return g_uart_notify_enabled;
}
