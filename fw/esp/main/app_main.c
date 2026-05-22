#include "ble_app.h"
#include "uart_task.h"

void app_main(void)
{
    ble_app_init();
    uart_task_start();
}
