#include <stdio.h>
#include "myADC.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define LED1 38
#define LED2 37 
#define LED3 36 

#define CHANNEL 5

static void leds_init(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << LED1) | (1ULL << LED2) | (1ULL << LED3),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    gpio_config(&io_conf);
    gpio_set_level(LED1, 0);
    gpio_set_level(LED2, 0);
    gpio_set_level(LED3, 0);
}

static void zet_kleur(float temperatuur_c)
{
    if (temperatuur_c > 19.0f)
    {
        gpio_set_level(LED1, 0);
        gpio_set_level(LED2, 1);
        gpio_set_level(LED3, 0);
    }
    else if (temperatuur_c >= 18.0f)
    {
        gpio_set_level(LED1, 1);
        gpio_set_level(LED2, 0);
        gpio_set_level(LED3, 0);
    }
    else
    {
        gpio_set_level(LED1, 0);
        gpio_set_level(LED2, 0);
        gpio_set_level(LED3, 1);
    }
}

void app_main(void)
{
    leds_init();
    myADC_setup(CHANNEL);

    while (1)
    {
        int spanning_mv = myADC_getMiliVolt(CHANNEL);

        float temperatuur_raw_c = spanning_mv / 10.0f;

        zet_kleur(temperatuur_raw_c);

        printf("Spanning: %d mV | Temp raw: %.1f C | Temp gecorrigeerd: %.1f C\n", spanning_mv, temperatuur_raw_c);

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}