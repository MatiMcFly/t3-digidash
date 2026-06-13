#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

void ble_app_init(void);
void ble_app_notify(uint16_t attr_handle, const void *data, size_t len);
uint16_t ble_app_conn_handle_get(void);
bool ble_app_uart_notify_enabled(void);
