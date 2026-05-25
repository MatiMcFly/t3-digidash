#include "remotexy_protocol.h"

#include <stdlib.h>
#include <string.h>

#include "ble_app.h"
#include "gatt_db.h"

#define REMOTEXY_PACKAGE_VERSION 20
#define REMOTEXY_PACKAGE_START_BYTE 0x55
#define REMOTEXY_PACKAGE_MIN_LENGTH 6

#define REMOTEXY_PACKAGE_COMMAND_GETCONF 0x00
#define REMOTEXY_PACKAGE_COMMAND_PING 0x10
#define REMOTEXY_PACKAGE_COMMAND_ALLVAR 0x40
#define REMOTEXY_PACKAGE_COMMAND_INPUTVAR 0x80
#define REMOTEXY_PACKAGE_COMMAND_OUTPUTVAR 0xC0

#define REMOTEXY_BLE_CHUNK_SIZE 20
#define REMOTEXY_RX_BUFFER_SIZE 256

#pragma pack(push, 1)
typedef struct {
    int16_t instrument_01;
    uint8_t connect_flag;
} RemoteXYData;
#pragma pack(pop)

static const uint8_t k_remotexy_conf_full[] = {
    255, 0, 0, 2, 0, 45, 0, 19, 0, 0, 0, 0, 31, 1, 106, 200, 1, 1, 1, 0,
    71, 32, 71, 42, 42, 56, 16, 2, 24, 135, 0, 0, 0, 0, 0, 0, 200, 66, 0, 0,
    160, 65, 0, 0, 32, 65, 0, 0, 0, 64, 24, 0
};

static RemoteXYData g_remotexy;
static uint8_t g_rx_buffer[REMOTEXY_RX_BUFFER_SIZE];
static size_t g_rx_len;

static uint16_t g_input_len;
static uint16_t g_output_len;
static uint16_t g_conf_len;
static const uint8_t *g_conf_data;

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
    while (len > 0) {
        size_t chunk = len > REMOTEXY_BLE_CHUNK_SIZE ? REMOTEXY_BLE_CHUNK_SIZE : len;
        ble_app_notify(gatt_chr_val_handle_uart_tx, data, chunk);
        data += chunk;
        len -= chunk;
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

static void remotexy_send_conf(uint8_t client_id)
{
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

static void remotexy_send_output(uint8_t client_id)
{
    uint8_t payload[1 + sizeof(int16_t)];
    uint8_t flags = 0;
    payload[0] = flags;
    payload[1] = (uint8_t)(g_remotexy.instrument_01 & 0xff);
    payload[2] = (uint8_t)((g_remotexy.instrument_01 >> 8) & 0xff);
    remotexy_send_packet(REMOTEXY_PACKAGE_COMMAND_OUTPUTVAR, client_id, payload, (uint16_t)(1 + g_output_len));
}

static void remotexy_send_allvar(uint8_t client_id)
{
    uint8_t payload[sizeof(int16_t)];
    payload[0] = (uint8_t)(g_remotexy.instrument_01 & 0xff);
    payload[1] = (uint8_t)((g_remotexy.instrument_01 >> 8) & 0xff);
    remotexy_send_packet(REMOTEXY_PACKAGE_COMMAND_ALLVAR, client_id, payload, g_output_len);
}

static void remotexy_handle_packet(uint8_t command, uint8_t client_id,
                                   const uint8_t *payload, uint16_t payload_len)
{
    switch (command) {
    case REMOTEXY_PACKAGE_COMMAND_GETCONF:
        remotexy_send_conf(client_id);
        break;
    case REMOTEXY_PACKAGE_COMMAND_PING:
        remotexy_send_packet(REMOTEXY_PACKAGE_COMMAND_PING, client_id, NULL, 0);
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
        remotexy_send_output(client_id);
        break;
    default:
        break;
    }
}

void remotexy_init(void)
{
    g_remotexy.instrument_01 = 0;
    g_remotexy.connect_flag = 0;

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
}

void remotexy_set_connected(bool connected)
{
    g_remotexy.connect_flag = connected ? 1 : 0;
}

void remotexy_set_instrument(int16_t value)
{
    g_remotexy.instrument_01 = value;
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
                }
            }

            memmove(g_rx_buffer, g_rx_buffer + packet_len, g_rx_len - packet_len);
            g_rx_len -= packet_len;
        }
    }
}
