#include "remotexy_protocol.h"

#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_timer.h"

#include "ble_app.h"
#include "gatt_db.h"

#define REMOTEXY_PACKAGE_VERSION 20
#define REMOTEXY_PACKAGE_START_BYTE 0x55
#define REMOTEXY_PACKAGE_MIN_LENGTH 6

#define REMOTEXY_PACKAGE_COMMAND_GETCONF 0x00
#define REMOTEXY_PACKAGE_COMMAND_PING 0x10
#define REMOTEXY_PACKAGE_COMMAND_TIME 0x20
#define REMOTEXY_PACKAGE_COMMAND_BOARD 0x30
#define REMOTEXY_PACKAGE_COMMAND_ALLVAR 0x40
#define REMOTEXY_PACKAGE_COMMAND_INPUTVAR 0x80
#define REMOTEXY_PACKAGE_COMMAND_OUTPUTVAR 0xC0

#define REMOTEXY_BLE_CHUNK_SIZE 20
#define REMOTEXY_RX_BUFFER_SIZE 256

static const char *TAG_RXY = "RemoteXY";

#pragma pack(push, 1)
typedef struct {
    uint8_t turn_signal_w;
    uint8_t high_beam_w;
    int16_t rpm_w;
    float batt_volt_w;
    uint8_t oil_w;
    int8_t water_temp_w;
    int8_t tank_level_w;
    uint8_t connect_flag;
} RemoteXYData;
#pragma pack(pop)

static const uint8_t k_remotexy_conf_full[] =  { 255,0,0,11,0,12,1,19,0,0,0,0,8,1,106,200,1,1,18,0,
  70,12,147,8,8,16,26,147,0,70,29,147,8,8,16,26,191,0,71,15,
  5,74,74,104,16,2,31,135,0,0,0,0,0,192,218,69,0,0,122,68,
  0,0,250,67,0,0,122,67,2,0,129,9,142,15,4,64,17,66,108,105,
  110,107,101,114,0,129,25,142,19,4,64,17,70,101,114,110,108,105,99,104,
  116,0,129,36,2,37,5,64,17,68,114,101,104,122,97,104,108,32,91,114,
  112,109,93,0,68,6,75,91,60,1,8,36,129,39,70,34,6,64,17,66,
  97,116,116,101,114,105,101,32,91,86,93,0,129,79,141,6,5,64,17,195,
  150,108,0,70,77,147,8,8,16,26,37,0,66,6,171,41,18,131,2,31,
  66,58,170,43,19,131,2,31,129,74,161,19,5,64,17,84,97,110,107,32,
  91,108,93,0,129,6,162,47,4,64,17,87,97,115,115,101,114,116,101,109,
  112,101,114,97,116,117,114,32,91,194,176,67,93,0,129,7,191,8,5,64,
  17,45,49,48,0,129,35,191,10,5,64,17,49,53,48,0,129,64,190,4,
  6,64,17,48,0,129,93,191,7,5,64,17,55,48,0 };

static RemoteXYData g_remotexy;
static uint8_t g_rx_buffer[REMOTEXY_RX_BUFFER_SIZE];
static size_t g_rx_len;
static bool g_connected;
static bool g_has_client_id;
static uint8_t g_last_client_id;
static bool g_output_dirty;
static bool g_notify_enabled;
static bool g_pending_conf;
static bool g_pending_output;
static bool g_session_ready;

static uint16_t g_input_len;
static uint16_t g_output_len;
static uint16_t g_conf_len;
static const uint8_t *g_conf_data;

static bool remotexy_send_output(uint8_t client_id);
static void remotexy_pack_outputs(uint8_t *buf, size_t buf_len);

static void remotexy_output_task(void *arg)
{
    (void)arg;
    while (true) {
        if (g_connected && g_has_client_id && g_session_ready && g_notify_enabled) {
            if (remotexy_send_output(g_last_client_id)) {
                g_output_dirty = false;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(5000));//?
    }
}

static void rxy_crc_init(uint16_t *crc)
{
    *crc = 0xffff;
}

static void rxy_crc_update(uint16_t *crc, uint8_t b)
{
    *crc ^= b;
    for (uint8_t i = 0; i < 8; ++i) {
        if ((*crc) & 1) {
            *crc = ((*crc) >> 1) ^ 0xA001;
        } else {
            *crc >>= 1;
        }
    }
}

static void remotexy_send_bytes(const uint8_t *data, size_t len)
{
    if (!g_notify_enabled) {
        return;
    }
    while (len > 0) {
        size_t chunk = len > REMOTEXY_BLE_CHUNK_SIZE ? REMOTEXY_BLE_CHUNK_SIZE : len;
        ble_app_notify(gatt_chr_val_handle_uart_tx, data, chunk);
        data += chunk;
        len -= chunk;
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

static void remotexy_send_packet(uint8_t command, uint8_t client_id,
                                 const uint8_t *payload, uint16_t payload_len)
{
    uint16_t total_len = (uint16_t)(payload_len + REMOTEXY_PACKAGE_MIN_LENGTH);
    uint16_t crc;

    uint8_t stack_buf[128];
    uint8_t *buf = stack_buf;
    size_t buf_size = sizeof(stack_buf);

    if (total_len > buf_size) {
        buf = (uint8_t *)malloc(total_len);
        if (buf == NULL) {
            return;
        }
    }

    buf[0] = REMOTEXY_PACKAGE_START_BYTE;
    buf[1] = (uint8_t)(total_len & 0xff);
    buf[2] = (uint8_t)((total_len >> 8) & 0xff);
    buf[3] = (uint8_t)(command | ((client_id & 0x07) << 1));
    if (payload_len > 0 && payload != NULL) {
        memcpy(&buf[4], payload, payload_len);
    }

    rxy_crc_init(&crc);
    for (uint16_t i = 0; i < (uint16_t)(4 + payload_len); ++i) {
        rxy_crc_update(&crc, buf[i]);
    }

    buf[4 + payload_len] = (uint8_t)(crc & 0xff);
    buf[5 + payload_len] = (uint8_t)((crc >> 8) & 0xff);

    remotexy_send_bytes(buf, total_len);

    if (buf != stack_buf) {
        free(buf);
    }
}

static void remotexy_send_empty(uint8_t command, uint8_t client_id)
{
    remotexy_send_packet(command, client_id, NULL, 0);
}

static void remotexy_send_conf(uint8_t client_id)
{
    if (!g_notify_enabled) {
        g_pending_conf = true;
        g_last_client_id = client_id;
        g_has_client_id = true;
        return;
    }
    uint8_t stack_buf[128];
    uint8_t *buf = stack_buf;
    size_t total_len = (size_t)g_conf_len + 1;

    if (total_len > sizeof(stack_buf)) {
        buf = (uint8_t *)malloc(total_len);
        if (buf == NULL) {
            return;
        }
    }

    buf[0] = REMOTEXY_PACKAGE_VERSION;
    if (g_conf_len > 0 && g_conf_data != NULL) {
        memcpy(&buf[1], g_conf_data, g_conf_len);
    }

    remotexy_send_packet(REMOTEXY_PACKAGE_COMMAND_GETCONF, client_id, buf, (uint16_t)total_len);

    if (buf != stack_buf) {
        free(buf);
    }
}

static bool remotexy_send_output(uint8_t client_id)
{
    if (!g_notify_enabled) {
        g_pending_output = true;
        g_last_client_id = client_id;
        g_has_client_id = true;
        return false;
    }
    size_t payload_len = (size_t)g_output_len + 1;
    uint8_t stack_buf[64];
    uint8_t *payload = stack_buf;

    if (payload_len > sizeof(stack_buf)) {
        payload = (uint8_t *)malloc(payload_len);
        if (payload == NULL) {
            return false;
        }
    }

    payload[0] = 0;
    remotexy_pack_outputs(&payload[1], g_output_len);
    remotexy_send_packet(REMOTEXY_PACKAGE_COMMAND_OUTPUTVAR, client_id, payload, (uint16_t)payload_len);

    if (payload != stack_buf) {
        free(payload);
    }
    return true;
}

static void remotexy_send_allvar(uint8_t client_id)
{
    uint8_t stack_buf[64];
    uint8_t *payload = stack_buf;

    if (g_output_len > sizeof(stack_buf)) {
        payload = (uint8_t *)malloc(g_output_len);
        if (payload == NULL) {
            return;
        }
    }

    remotexy_pack_outputs(payload, g_output_len);
    remotexy_send_packet(REMOTEXY_PACKAGE_COMMAND_ALLVAR, client_id, payload, g_output_len);

    if (payload != stack_buf) {
        free(payload);
    }
}

static void remotexy_handle_packet(uint8_t command, uint8_t client_id,
                                   const uint8_t *payload, uint16_t payload_len)
{
    g_last_client_id = client_id;
    g_has_client_id = true;
    ESP_LOGI(TAG_RXY, "RX cmd=0x%02x len=%u client=%u", command, payload_len, client_id);
    switch (command) {
    case REMOTEXY_PACKAGE_COMMAND_GETCONF:
        remotexy_send_conf(client_id);
        g_session_ready = true;
        break;
    case REMOTEXY_PACKAGE_COMMAND_PING:
        remotexy_send_empty(REMOTEXY_PACKAGE_COMMAND_PING, client_id);
        break;
    case REMOTEXY_PACKAGE_COMMAND_TIME:
    {
        int64_t ms = esp_timer_get_time() / 1000;
        remotexy_send_packet(REMOTEXY_PACKAGE_COMMAND_TIME, client_id, (uint8_t *)&ms, (uint16_t)sizeof(ms));
        break;
    }
    case REMOTEXY_PACKAGE_COMMAND_BOARD:
        remotexy_send_empty(REMOTEXY_PACKAGE_COMMAND_BOARD, client_id);
        break;
    case REMOTEXY_PACKAGE_COMMAND_ALLVAR:
        remotexy_send_allvar(client_id);
        break;
    case REMOTEXY_PACKAGE_COMMAND_INPUTVAR:
        if (payload_len == g_input_len) {
            remotexy_send_packet(REMOTEXY_PACKAGE_COMMAND_INPUTVAR, client_id, NULL, 0);
        }
        break;
    case REMOTEXY_PACKAGE_COMMAND_OUTPUTVAR:
        //remotexy_send_output(client_id);
        remotexy_send_empty(REMOTEXY_PACKAGE_COMMAND_PING, client_id);
        break;
    default:
        remotexy_send_empty(command, client_id);
        break;
    }
}

void remotexy_init(void)
{
    g_remotexy.turn_signal_w = 0;
    g_remotexy.high_beam_w = 0;
    g_remotexy.rpm_w = 0;
    g_remotexy.batt_volt_w = 0;
    g_remotexy.oil_w = 0;
    g_remotexy.water_temp_w = 0;
    g_remotexy.tank_level_w = 0;
    g_remotexy.connect_flag = 0;
    g_connected = false;
    g_has_client_id = false;
    g_last_client_id = 0;
    g_output_dirty = false;
    g_notify_enabled = false;
    g_pending_conf = false;
    g_pending_output = false;
    g_session_ready = false;

    if (k_remotexy_conf_full[0] >= 0xfe) {
        g_input_len = (uint16_t)(k_remotexy_conf_full[1] | (k_remotexy_conf_full[2] << 8));
        g_output_len = (uint16_t)(k_remotexy_conf_full[3] | (k_remotexy_conf_full[4] << 8));
        g_conf_len = (uint16_t)(k_remotexy_conf_full[5] | (k_remotexy_conf_full[6] << 8));
        g_conf_data = &k_remotexy_conf_full[7];
    } else {
        g_input_len = k_remotexy_conf_full[0];
        g_output_len = k_remotexy_conf_full[1];
        g_conf_len = k_remotexy_conf_full[2];
        g_conf_data = &k_remotexy_conf_full[3];
    }

    xTaskCreate(remotexy_output_task, "remotexy_output", 2048, NULL, 5, NULL);
}

void remotexy_set_connected(bool connected)
{
    g_remotexy.connect_flag = connected ? 1 : 0;
    g_connected = connected;
    if (!connected) {
        g_has_client_id = false;
    }
    g_session_ready = false;
    g_output_dirty = false;
    ESP_LOGI(TAG_RXY, "Connected=%d", connected ? 1 : 0);
}

void remotexy_set_notify_enabled(bool enabled)
{
    g_notify_enabled = enabled;
    ESP_LOGI(TAG_RXY, "Notify enabled=%d", enabled ? 1 : 0);
    if (g_notify_enabled && !g_has_client_id) {
        g_last_client_id = 0;
        g_has_client_id = true;
    }
    if (g_notify_enabled && g_has_client_id) {
        if (g_pending_conf) {
            g_pending_conf = false;
            remotexy_send_conf(g_last_client_id);
            g_session_ready = true;
        }
        if (g_pending_output) {
            g_pending_output = false;
            g_output_dirty = true;
        }
    }
}

void remotexy_set_outputs(uint8_t turn_signal_w, uint8_t high_beam_w, int16_t rpm_w,
                          float batt_volt_w, uint8_t oil_w,
                          int8_t water_temp_w, int8_t tank_level_w)
{
    bool changed = false;

    if (g_remotexy.turn_signal_w != turn_signal_w) {
        g_remotexy.turn_signal_w = turn_signal_w;
        changed = true;
    }
    if (g_remotexy.high_beam_w != high_beam_w) {
        g_remotexy.high_beam_w = high_beam_w;
        changed = true;
    }
    if (g_remotexy.rpm_w != rpm_w) {
        g_remotexy.rpm_w = rpm_w;
        changed = true;
    }
    if (g_remotexy.batt_volt_w != batt_volt_w) {
        g_remotexy.batt_volt_w = batt_volt_w;
        changed = true;
    }
    if (g_remotexy.oil_w != oil_w) {
        g_remotexy.oil_w = oil_w;
        changed = true;
    }
    if (g_remotexy.water_temp_w != water_temp_w) {
        g_remotexy.water_temp_w = water_temp_w;
        changed = true;
    }
    if (g_remotexy.tank_level_w != tank_level_w) {
        g_remotexy.tank_level_w = tank_level_w;
        changed = true;
    }

    if (!changed) {
        return;
    }

    g_output_dirty = true;
    if (g_connected && g_has_client_id && g_session_ready) {
        if (remotexy_send_output(g_last_client_id)) {
            g_output_dirty = false;
        }
    }
}

static void remotexy_pack_outputs(uint8_t *buf, size_t buf_len)
{
    size_t offset = 0;

    if (buf == NULL || buf_len < g_output_len) {
        return;
    }

    buf[offset++] = g_remotexy.turn_signal_w;
    buf[offset++] = g_remotexy.high_beam_w;
    memcpy(&buf[offset], &g_remotexy.rpm_w, sizeof(g_remotexy.rpm_w));
    offset += sizeof(g_remotexy.rpm_w);
    memcpy(&buf[offset], &g_remotexy.batt_volt_w, sizeof(g_remotexy.batt_volt_w));
    offset += sizeof(g_remotexy.batt_volt_w);
    buf[offset++] = g_remotexy.oil_w;
    memcpy(&buf[offset], &g_remotexy.water_temp_w, sizeof(g_remotexy.water_temp_w));
    offset += sizeof(g_remotexy.water_temp_w);
    memcpy(&buf[offset], &g_remotexy.tank_level_w, sizeof(g_remotexy.tank_level_w));
}

void remotexy_handle_rx(const uint8_t *data, size_t len)
{
    if (data == NULL || len == 0) {
        return;
    }

    for (size_t i = 0; i < len; ++i) {
        if (g_rx_len >= sizeof(g_rx_buffer)) {
            memmove(g_rx_buffer, g_rx_buffer + 1, sizeof(g_rx_buffer) - 1);
            g_rx_len = sizeof(g_rx_buffer) - 1;
        }
        g_rx_buffer[g_rx_len++] = data[i];

        while (g_rx_len >= REMOTEXY_PACKAGE_MIN_LENGTH) {
            if (g_rx_buffer[0] != REMOTEXY_PACKAGE_START_BYTE) {
                memmove(g_rx_buffer, g_rx_buffer + 1, g_rx_len - 1);
                g_rx_len--;
                continue;
            }

            uint16_t packet_len = (uint16_t)(g_rx_buffer[1] | (g_rx_buffer[2] << 8));
            if (packet_len < REMOTEXY_PACKAGE_MIN_LENGTH) {
                memmove(g_rx_buffer, g_rx_buffer + 1, g_rx_len - 1);
                g_rx_len--;
                continue;
            }

            if (packet_len > g_rx_len) {
                break;
            }

            uint16_t crc = 0;
            rxy_crc_init(&crc);
            for (uint16_t j = 0; j < packet_len; ++j) {
                rxy_crc_update(&crc, g_rx_buffer[j]);
            }


            if (crc == 0) {
                uint8_t cm = g_rx_buffer[3];
                if ((cm & 0x01) == 0) {
                    uint8_t command = cm & 0xf1;
                    uint8_t client_id = (cm >> 1) & 0x07;
                    uint16_t payload_len = (uint16_t)(packet_len - REMOTEXY_PACKAGE_MIN_LENGTH);
                    const uint8_t *payload = payload_len ? &g_rx_buffer[4] : NULL;
                    remotexy_handle_packet(command, client_id, payload, payload_len);
                } else {
                    ESP_LOGW(TAG_RXY, "RX fragment flag set cm=0x%02x len=%u", cm, packet_len);
                }
            } else {
                ESP_LOGW(TAG_RXY, "RX packet with invalid CRC len=%u", packet_len);
            }

            memmove(g_rx_buffer, g_rx_buffer + packet_len, g_rx_len - packet_len);
            g_rx_len -= packet_len;
        }
    }
}
