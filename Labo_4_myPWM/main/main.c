#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "myPWM.h"

#define CHANNEL 4
#define LED 5

#define LEDC_TIMER LEDC_TIMER_0
#define LEDC_MODE LEDC_LOW_SPEED_MODE
#define LEDC_CHANNEL LEDC_CHANNEL_0
#define LEDC_DUTY_RES LEDC_TIMER_13_BIT
#define PWM_MAX_DUTY ((1 << 13) - 1)
#define LEDC_FREQUENCY 400
#define ADC_MAX_MV 3300

static int clamp_init(int value, int min, int max)
{
    if (value < min)
    {
        return min;
    }
    if (value > max)
    {
        return max;
    }
    return value;
}

static esp_adc_cal_characteristics_t adc_chars;

static void pwm_init_led(void)
{
    ledc_timer_config_t ledc_timer = {
        .speed_mode = LEDC_MODE,
        .timer_num = LEDC_TIMER,
        .duty_resolution = LEDC_DUTY_RES,
        .freq_hz = LEDC_FREQUENCY, // Set output frequency at 4 kHz
        .clk_cfg = LEDC_AUTO_CLK};
    ledc_timer_config(&ledc_timer);

    ledc_channel_config_t ledc_channel = {
        .speed_mode = LEDC_MODE,
        .channel = LEDC_CHANNEL,
        .timer_sel = LEDC_TIMER,
        .intr_type = LEDC_INTR_DISABLE,
        .gpio_num = LED,
        .duty = 0, // Set duty to 0%
        .hpoint = 0};
    ledc_channel_config(&ledc_channel);
}

static void adc_init(void)
{
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(CHANNEL, ADC_ATTEN_DB_12);
    esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_12, ADC_WIDTH_BIT_12, 1100, &adc_chars);
}

static void set_led_cycle(int mv)
{
    int duty = (clamp_init(mv, 0, ADC_MAX_MV) * PWM_MAX_DUTY) / ADC_MAX_MV;
    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, duty);
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL);
    printf("Licht: %4d mV | Duty: %4d/%d (%3d%%)\n", mv, duty, PWM_MAX_DUTY, duty * 100 / PWM_MAX_DUTY);
}

void app_main(void)
{
    pwm_init_led();
    adc_init();

    while (1)
    {
        int raw = adc1_get_raw(CHANNEL);
        int mv = esp_adc_cal_raw_to_voltage(raw, &adc_chars);
        set_led_cycle(mv);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}