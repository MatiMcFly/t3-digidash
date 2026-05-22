idf_component_register(SRCS "app_main.c"
                       "ble_app.c"
                       "gatt_db.c"
                       "car_signals.c"
                       "uart_task.c"
                       "uart_parser.c"
                       INCLUDE_DIRS "."
                       REQUIRES nvs_flash bt esp_driver_uart esp_driver_gpio)
