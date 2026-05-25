#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

void remotexy_init(void);
void remotexy_set_connected(bool connected);
void remotexy_set_notify_enabled(bool enabled);
void remotexy_set_instrument(int16_t value);
void remotexy_handle_rx(const uint8_t *data, size_t len);
