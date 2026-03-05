idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS "."
                    REQUIRES myADC
                    esp_driver_gpio)
