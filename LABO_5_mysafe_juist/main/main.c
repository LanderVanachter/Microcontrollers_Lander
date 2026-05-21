#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"

#define LED_RED 4
#define LED_GREEN 5
#define LED_YELLOW 6
#define BUTTON_PIN 7
#define ADC_CHANNEL ADC_CHANNEL_8

void init_adc(adc_oneshot_unit_handle_t *adc_handle, adc_cali_handle_t *cali_handle) {
    //ADC unit config
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT_1
    };
    adc_oneshot_new_unit(&init_config, adc_handle);

    //channel config
    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_12,
        .atten = ADC_ATTEN_DB_12,
    };
    adc_oneshot_config_channel(*adc_handle, ADC_CHANNEL, &config);

    //calibration
    adc_cali_curve_fitting_config_t cali_config = {
        .unit_id = ADC_UNIT_1,
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_12,
    };
    adc_cali_create_scheme_curve_fitting(&cali_config, cali_handle);
}

void app_main(void)
{
    gpio_set_direction(LED_RED, GPIO_MODE_OUTPUT);
    gpio_set_direction(LED_GREEN, GPIO_MODE_OUTPUT);
    gpio_set_direction(LED_YELLOW, GPIO_MODE_OUTPUT);

    gpio_set_direction(BUTTON_PIN, GPIO_MODE_INPUT);
    gpio_set_pull_mode(BUTTON_PIN, GPIO_PULLDOWN_ONLY);


    adc_oneshot_unit_handle_t adc_handle;
    adc_cali_handle_t cali_handle;
    init_adc(&adc_handle, &cali_handle);

    int code[4] = {3, 7, 1, 9};
    int index = 0;
    bool pogingCorrect = true;

while (1) {

    int raw = 0;
    adc_oneshot_read(adc_handle, ADC_CHANNEL, &raw);

    int mv = 0;
    adc_cali_raw_to_voltage(cali_handle, raw, &mv);

    int getal = (mv * 10) / 2800;
    if (getal > 9) 
    {
        getal = 9;
    }
    printf("Getal: %d\n", getal);
    //printf("Index: %d, Ingevoerd: %d, Verwacht: %d\n", index, getal, code[index]); CONTROLE

    if (gpio_get_level(BUTTON_PIN) == 1) {

        if (getal == code[index]) {
            gpio_set_level(LED_GREEN, 1);
        } else {
            gpio_set_level(LED_RED, 1);
            pogingCorrect = false;
        }

        vTaskDelay(300 / portTICK_PERIOD_MS);

        gpio_set_level(LED_GREEN, 0);
        gpio_set_level(LED_RED, 0);

        index++;

        if (index == 4) {
            if (pogingCorrect)
            {
                printf("Code correct!\n");
                gpio_set_level(LED_YELLOW, 1);
            }
            else {
                printf("Code incorrect!\n");
            }

            index = 0;
            pogingCorrect = true;

            vTaskDelay(500 / portTICK_PERIOD_MS);
        }

        vTaskDelay(300 / portTICK_PERIOD_MS);
    }

    vTaskDelay(50 / portTICK_PERIOD_MS);
}

}
