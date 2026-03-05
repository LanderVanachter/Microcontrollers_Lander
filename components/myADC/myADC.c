#include <stdio.h>
#include "myADC.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "hal/adc_types.h"

#define ADC_CALIBRATION 1

adc_cali_handle_t calibratie_curve_adc1;
adc_cali_curve_fitting_config_t cali_config = {
    .unit_id = ADC_UNIT_1,
    .atten = ADC_ATTEN_DB_12,
    .bitwidth = ADC_BITWIDTH_DEFAULT,
};

adc_oneshot_unit_handle_t adc1_handle;
adc_oneshot_unit_init_cfg_t adc_config = {
    .unit_id = ADC_UNIT_1,
    .ulp_mode = ADC_ULP_MODE_DISABLE,
};
adc_oneshot_chan_cfg_t default_channel_config = {
    .bitwidth = ADC_BITWIDTH_DEFAULT, // dit is de max supported width en dat is 13
    .atten = ADC_ATTEN_DB_12,
};
void myADC_setup(int channel)
{
    adc_oneshot_new_unit(&adc_config, &adc1_handle);
    adc_oneshot_config_channel(adc1_handle, channel, &default_channel_config);

#if ADC_CALIBRATION
    adc_cali_create_scheme_curve_fitting(&cali_config, &calibratie_curve_adc1);
#endif
}

int myADC_getValue(int channel)
{
    int getal;
    adc_oneshot_read(adc1_handle, channel, &getal);
    return getal;
}

int myADC_getMiliVolt(int channel)
{
    int getal, voltage;
    adc_oneshot_read(adc1_handle, channel, &getal);
    adc_cali_raw_to_voltage(calibratie_curve_adc1, getal, &voltage);
    return voltage;
}