#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "esp_random.h"
#include "string.h"

#define UART_NUM UART_NUM_0
int getal, gok;

void read_message(char *buf)
{
    int pos = 0;
    while (1)
    {
        char temp;

        uart_read_bytes(UART_NUM, &temp, 1, portMAX_DELAY);
        printf("%s\n", buf);
        switch (temp)
        {
            case '\r':

            case '\n':
                buf[pos++] = temp;
                pos = 0;
                return;
            default:
                buf[pos++] = temp;
        }
    }
}

void app_main(void)
{
    const uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };
    uart_param_config(UART_NUM_0, &uart_config);
    uart_driver_install(UART_NUM_0, 1024, 0, 0, NULL, 0);

    getal = esp_random() % 10;

    char data[10];
    printf("Wat denk je dat het getal is?\n");

    int run = 1;

    while (run)
    {
        read_message(data);
        sscanf(data, "%d", &gok);

        if (gok == getal)
        {
            printf("Goed geraden!\n");
            run = 0;
        }
        else
        {
            printf("Loser\n");
        }
    }
}
