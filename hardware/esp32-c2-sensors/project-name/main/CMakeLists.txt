idf_component_register(SRCS "station_example_main.c"
                      INCLUDE_DIRS "."
                      REQUIRES dht
                              esp_adc
                              esp_wifi
                              nvs_flash
                              esp_netif
                              libcoap)  # Presupunem că libcoap este componenta