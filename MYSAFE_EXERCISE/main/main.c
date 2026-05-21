#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali_scheme.h"

#define LED_ROOD 5
#define LED_ORANJE 6
#define LED_GROEN 7
#define LED_BLAUW 10
#define LED_JUIST 12

#define BUTTON_PIN 11

#define ADC_CHANNEL ADC_CHANNEL_7
#define ADC_GPIO 8
#define ADC_UNIT ADC_UNIT_1

adc_oneshot_unit_handle_t adc1_handle;
adc_cali_handle_t cali_handle;

int code[4] = {8, 6, 2, 0};
int input[4];
int index1 = 0;

volatile bool buttonPressed = false;
volatile uint32_t lastInterruptTime = 0;

/*void BUTTON_init(void)
{
    gpio_reset_pin(BUTTON_PIN);
    gpio_set_direction(BUTTON_PIN, GPIO_MODE_INPUT);
    gpio_set_pull_mode(BUTTON_PIN, GPIO_PULLUP_ONLY);
}*/

static void IRAM_ATTR button_isr_handler(void* arg)
{
    uint32_t now = xTaskGetTickCountFromISR();

    // debounce: alleen triggeren als er minstens 150 ms voorbij is
    if (now - lastInterruptTime > pdMS_TO_TICKS(150))
    {
        buttonPressed = true;
        lastInterruptTime = now;
    }
}

void BUTTON_init(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = 1ULL << BUTTON_PIN,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .intr_type = GPIO_INTR_NEGEDGE   // knop ingedrukt (actief laag)
    };
    gpio_config(&io_conf);

    gpio_install_isr_service(0);
    gpio_isr_handler_add(BUTTON_PIN, button_isr_handler, NULL);
}

void LED_init(void)
{
    gpio_reset_pin(LED_ROOD);
    gpio_set_direction(LED_ROOD, GPIO_MODE_OUTPUT);

    gpio_reset_pin(LED_ORANJE);
    gpio_set_direction(LED_ORANJE, GPIO_MODE_OUTPUT);

    gpio_reset_pin(LED_GROEN);
    gpio_set_direction(LED_GROEN, GPIO_MODE_OUTPUT);

    gpio_reset_pin(LED_BLAUW);
    gpio_set_direction(LED_BLAUW, GPIO_MODE_OUTPUT);

    gpio_reset_pin(LED_JUIST);
    gpio_set_direction(LED_JUIST, GPIO_MODE_OUTPUT);
}

void adc_calibration_init(void)
{
    // 1) ADC unit configureren
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT,
    };
    adc_oneshot_new_unit(&init_config, &adc1_handle);

    // 2) ADC kanaal configureren
    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_12,
        .atten = ADC_ATTEN_DB_12,
    };
    adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL, &config);

    // 3) Calibratie
    adc_cali_curve_fitting_config_t cali_config = {
        .unit_id = ADC_UNIT,
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_12,
    };
    adc_cali_create_scheme_curve_fitting(&cali_config, &cali_handle);
}

void app_main(void)
{
    adc_calibration_init();
    LED_init();
    BUTTON_init();

    while (1)
    {
        int raw = 0;
        adc_oneshot_read(adc1_handle, ADC_CHANNEL, &raw);
        int mV = 0;
        adc_cali_raw_to_voltage(cali_handle, raw, &mV);
        int number = (mV * 9 ) / 2300;

        printf("Huidige getal %d\n", number);
        vTaskDelay(100);

        int button = gpio_get_level(BUTTON_PIN);

        // Detecteer *rising edge* (knop losgelaten → ingedrukt)
        if (buttonPressed)
        {
            buttonPressed = false;

            printf("Invoer geregistreerd: %d\n", number);

            input[index1] = number;

            bool correct = (input[index1] == code[index1]);

            if (correct)
            {
                switch (index1)
                {
                    case 0:
                        gpio_set_level(LED_ROOD, 1);
                        break;
                    case 1:
                        gpio_set_level(LED_ORANJE, 1);
                        break;
                    case 2:
                        gpio_set_level(LED_GROEN, 1);
                        break;
                    case 3:
                        gpio_set_level(LED_BLAUW, 1);
                        break;
                }
            }
            else {
                gpio_set_level(LED_ROOD, 0);
                gpio_set_level(LED_ORANJE, 0);
                gpio_set_level(LED_GROEN, 0);
                gpio_set_level(LED_BLAUW, 0);

                printf("Fout cijfer op positie %d\n", index1);

                index1 = 0;
                vTaskDelay(1000);
                continue;
            }

            index1++;

            if (index1 == 4)
            {
                printf("Slot open\n");
                gpio_set_level(LED_JUIST, 1);
                vTaskDelay(pdMS_TO_TICKS(2000));

                index1 = 0;

                gpio_set_level(LED_ROOD, 0);
                gpio_set_level(LED_ORANJE, 0);
                gpio_set_level(LED_GROEN, 0);
                gpio_set_level(LED_BLAUW, 0);
            }
        }
    }
}
