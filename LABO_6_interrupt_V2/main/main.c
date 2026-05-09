#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_random.h"
#include "esp_timer.h"
#include "driver/gptimer.h"

#define LED 5
#define BUTTON 6

gptimer_handle_t gptimer;
volatile bool button_pressed = false;
bool ready = false;
volatile bool too_early = false;

static void IRAM_ATTR button_isr_handler(void* arg) {
    if (!ready)
    {
        too_early = true;
    } 
    else 
    {
        button_pressed = true;
    }
}

void button_setup(int pin)
{
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_NEGEDGE,
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = 1ULL << pin,
        .pull_up_en = GPIO_PULLUP_ENABLE
    };
    gpio_config(&io_conf);

    gpio_install_isr_service(0);
    gpio_isr_handler_add(pin, button_isr_handler, NULL);
}

void led_setup(int pin)
{
    gpio_reset_pin(pin);
    gpio_set_direction(pin, GPIO_MODE_OUTPUT);
}

void timer_setup(void)
{
    gptimer_config_t timer_config = {
        .clk_src = GPTIMER_CLK_SRC_DEFAULT, // Select the default clock source
        .direction = GPTIMER_COUNT_UP,      // Counting direction is up
        .resolution_hz = 1 * 1000 * 1000,   // Resolution is 1 MHz, i.e., 1 tick equals 1 microsecond
    };
  
    gptimer_new_timer(&timer_config, &gptimer);                 // Create a timer instance 
    gptimer_enable(gptimer);            // Enable the timer
    gptimer_start(gptimer);             // Start the timer

}

void app_main(void)
{
    led_setup(LED);
    timer_setup();
    button_setup(BUTTON);

    while (1)
    {
        button_pressed = false;

        int getal = 2000 + esp_random() % 5000;
        vTaskDelay(getal / portTICK_PERIOD_MS);
        //printf("Getal: %d\n", getal);

        gpio_set_level(LED, 1);
        ready = true;

        gptimer_set_raw_count(gptimer, 0);

        while (!button_pressed)
        {
            vTaskDelay(1);
        }

        gpio_set_level(LED, 0);

        if (too_early)
        {
            printf("Te vroeg!\n");
            too_early = false;
            continue;
        }

        uint64_t tijd;
        gptimer_get_raw_count(gptimer, &tijd);

        printf("time: %lld milisec\n", tijd / 1000);
    }
}
