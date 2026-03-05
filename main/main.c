#include <stdio.h>
#include "myADC.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define LED1 38 // GROEN
#define LED2 37 // ROOD
#define LED3 36 // BLAUW

#define CHANNEL 5
#define TEMP_OFFSET_C -1.0f /// verwijderen misschien

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
        // Boven 19°C -> ROOD (LED2)
        gpio_set_level(LED1, 0);
        gpio_set_level(LED2, 1);
        gpio_set_level(LED3, 0);
    }
    else if (temperatuur_c >= 18.0f)
    {
        // Tussen 18°C en 19°C -> GROEN (LED1)
        gpio_set_level(LED1, 1);
        gpio_set_level(LED2, 0);
        gpio_set_level(LED3, 0);
    }
    else
    {
        // Onder 18°C -> BLAUW (LED3)
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

        if (spanning_mv < 0 || spanning_mv > 1100)
        {
            gpio_set_level(LED1, 0);
            gpio_set_level(LED2, 0);
            gpio_set_level(LED3, 1);
            printf("Sensorfout: %d mV (check LM35 bedrading: V+, OUT, GND)\n", spanning_mv);
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }

        if (spanning_mv > 500)
        {
            gpio_set_level(LED1, 0);
            gpio_set_level(LED2, 0);
            gpio_set_level(LED3, 1);
            printf("Onrealistische LM35-waarde: %d mV (controleer ADC-kanaal en OUT-pin)\n", spanning_mv);
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }

        float temperatuur_raw_c = spanning_mv / 10.0f;
        float temperatuur_c = temperatuur_raw_c + TEMP_OFFSET_C; // verwijderen misschien

        zet_kleur(temperatuur_c);

        printf("Spanning: %d mV | Temp raw: %.1f C | Temp gecorrigeerd: %.1f C\n", spanning_mv, temperatuur_raw_c, temperatuur_c);

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}