#include "ble_app.h"
#include "remotexy_protocol.h"
#include "uart_task.h"

void app_main(void)
{
    ble_app_init();
    remotexy_init();
    uart_task_start();
}
