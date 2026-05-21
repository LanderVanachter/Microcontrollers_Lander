#include <stdio.h>
#include "string.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali_scheme.h"
#include "driver/i2c.h"
#include "font6x8.h"

#define I2C_MASTER_SCL_IO           5
#define I2C_MASTER_SDA_IO           6
#define I2C_MASTER_PORT             I2C_NUM_0
#define I2C_MASTER_FREQ_HZ          100000
#define OLED_ADDR                   0x3C

#define MQ3_ADC_GPIO      8
#define MQ3_ADC_CHANNEL   ADC_CHANNEL_7   // GPIO 8
#define MQ3_ADC_UNIT      ADC_UNIT_1      // Altijd ADC_UNIT_1 of ADC_UNIT_2

adc_oneshot_unit_handle_t adc1_handle;
adc_cali_handle_t cali_handle;
bool calibrated = false;
extern const uint8_t font6x8[][6];


void adc_calibration_init(void)
{
    // 1) ADC unit configureren
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = MQ3_ADC_UNIT,
    };
    adc_oneshot_new_unit(&init_config, &adc1_handle);

    // 2) ADC kanaal configureren
    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_12,
        .atten = ADC_ATTEN_DB_12,
    };
    adc_oneshot_config_channel(adc1_handle, MQ3_ADC_CHANNEL, &config);

    // 3) Calibratie
    adc_cali_curve_fitting_config_t cali_config = {
        .unit_id = MQ3_ADC_UNIT,
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_12,
    };
    adc_cali_create_scheme_curve_fitting(&cali_config, &cali_handle);
}

void i2c_master_init(void)
{
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };
    i2c_param_config(I2C_MASTER_PORT, &conf);
    i2c_driver_install(I2C_MASTER_PORT, conf.mode, 0, 0, 0);
}

//Stuurt commando's naar de OLED display (0x00 = commando)
void i2c_OLED_send_cmd(uint8_t cmd)
{
    uint8_t data[2] = {0x00, cmd};
    i2c_master_write_to_device(I2C_NUM_0, 0x3C, data, 2, pdMS_TO_TICKS(100));
}

//Stuurt data naar de OLED display (0x40 = data/pixels)
void i2c_OLED_send_data(uint8_t data)
{
    uint8_t buf[2] = {0x40, data};
    i2c_master_write_to_device(I2C_NUM_0, 0x3C, buf, 2, pdMS_TO_TICKS(100));
}

void i2c_OLED_clear_display(void)
{
    for (int page = 0; page < 8; page++)
    {
        i2c_OLED_send_cmd(0xB0 + page); // Set page address
        i2c_OLED_send_cmd(0x00);        // Set lower column address
        i2c_OLED_send_cmd(0x10);        // Set higher column address

        for (int col = 0; col < 128; col++)
            i2c_OLED_send_data(0x00);   // Clear pixel data
    }
}

void OLED_init(void)
{
    i2c_OLED_send_cmd(0xAE); // Set display OFF		
 //Display moet eerst uit voor je shit kunt aanpassen

    i2c_OLED_send_cmd(0x20); // Set Memory Addressing Mode
    i2c_OLED_send_cmd(0x00); // Horizontal Addressing Mode

    i2c_OLED_send_cmd(0xD4); // Set Display Clock Divide Ratio / OSC Frequency
    i2c_OLED_send_cmd(0x80); // Display Clock Divide Ratio / OSC Frequency 
 //Deze bepalen de interne klok van de display

    i2c_OLED_send_cmd(0xA8); // Set Multiplex Ratio
    i2c_OLED_send_cmd(0x3F); // Multiplex Ratio for 128x64 (64-1)
 //Zegt gwn dat de display 128x64 is

    i2c_OLED_send_cmd(0xD3); // Set Display Offset
    i2c_OLED_send_cmd(0x00); // Display Offset
 //Zodat er geen vertical verschuivingen zijn

    i2c_OLED_send_cmd(0x40); // Set Display Start Line
 //Bepaalt welke rij bovenaan staat

    i2c_OLED_send_cmd(0x8D); // Set Charge Pump
    i2c_OLED_send_cmd(0x14); // Charge Pump (0x10 External, 0x14 Internal DC/DC)
 //Ok dit maakt van u 3v3 spanning een 7v-9v spanning want das wa u display nodig heeft

    i2c_OLED_send_cmd(0xA1); // Set Segment Re-Map
    i2c_OLED_send_cmd(0xC8); // Set Com Output Scan Direction
 //Spiegelt de display zodat ie niet ondersteboven staat (horizontaal en verticaal)

    i2c_OLED_send_cmd(0xDA); // Set COM Hardware Configuration
    i2c_OLED_send_cmd(0x12); // COM Hardware Configuration
 //COM-lijn = vertical pixels, SEG-lijn = horizontal pixels, de 0x12 is voor 128x64 displays

    i2c_OLED_send_cmd(0x81); // Set Contrast
    i2c_OLED_send_cmd(0xCF); // Contrast
// Bepaalt hoe helder de pixels zijn. Hangt af van u voeding.
 
    i2c_OLED_send_cmd(0xD9); // Set Pre-Charge Period
    i2c_OLED_send_cmd(0xF1); // Set Pre-Charge Period (0x22 External, 0xF1 Internal)
 // Bepaalt de tijd dat de pixel opgeladen wordt voordat ie aan gaat. Hangt ook af van u voeding.

    i2c_OLED_send_cmd(0xDB); // Set VCOMH Deselect Level
    i2c_OLED_send_cmd(0x40); // VCOMH Deselect Level
//VCOMH = Voltage for COmmon pin High level. Dit is de spanning waartegen de OLED zijn pixelspanning vergelijkt.
 
    i2c_OLED_send_cmd(0xA4); // Set all pixels OFF
    i2c_OLED_send_cmd(0xA6); // Set display not inverted
    i2c_OLED_send_cmd(0xAF); // Set display On
//DISPLAY MAG PAS OP HET EINDE TERUG AAN!!
    i2c_OLED_clear_display();
}

//de print plaatst 1 karakter.
void OLED_print(int x, int page, const char *text)
{
    // Cursor zetten
    i2c_OLED_send_cmd(0xB0 + page);
    i2c_OLED_send_cmd(0x00 + (x & 0x0F));
    i2c_OLED_send_cmd(0x10 + (x >> 4));

    // Tekst tekenen
    while (*text)
    {
        uint8_t index = (uint8_t)*text;

        if (index < 0x20 || index > 0x7E)
            index = 0x20; // onbekend karakter = spatie

        const uint8_t *c = font6x8[index - 0x20];

        for (int i = 0; i < 6; i++)
            i2c_OLED_send_data(c[i]);

        text++;
    }
    // Forceer cursor reset zodat volgende writes niet breken
    i2c_OLED_send_cmd(0x00);
    i2c_OLED_send_cmd(0x10);
}

//De print line plaats een hele regel tekst.
void OLED_print_line(int page, const char *text)
{
    //Zet cursor naar begin van de regel
    i2c_OLED_send_cmd(0xB0 + page);
    i2c_OLED_send_cmd(0x00);
    i2c_OLED_send_cmd(0x10);
    //Clear de hele regel
    for (int i = 0; i < 90; i++)
        i2c_OLED_send_data(0x00);

    // Zet cursor terug naar begin
    i2c_OLED_send_cmd(0xB0 + page);
    i2c_OLED_send_cmd(0x00);
    i2c_OLED_send_cmd(0x10);

    // Print nieuwe tekst
    OLED_print(0, page, text);
}

//page = kies waar je wilt, value = alcohollevel, max = 2000
void OLED_draw_bar(int page, float value, float max)
{
    if (value < 0) value = 0;
    if (value > max) value = max;

    int width = (int)((value / max) * 100);

    //Zet cursor naar begin
    i2c_OLED_send_cmd(0xB0 + page);
    i2c_OLED_send_cmd(0x00);
    i2c_OLED_send_cmd(0x10);
    //Clear de hele regel
    for (int i = 0; i < 100; i++)
        i2c_OLED_send_data(0x00);

    // Cursor terug naar begin
    i2c_OLED_send_cmd(0xB0 + page);
    i2c_OLED_send_cmd(0x00);
    i2c_OLED_send_cmd(0x10);

    // Teken de bar
    for (int i = 0; i < width; i++)
        i2c_OLED_send_data(0xFF);
}


void app_main(void)
{
    vTaskDelay(10 / portTICK_PERIOD_MS);

    float maxValue = 1383; //Waarde adem zonder alcohol

    vTaskDelay(10 / portTICK_PERIOD_MS);
    i2c_master_init();
    vTaskDelay(10 / portTICK_PERIOD_MS);
    adc_calibration_init();
    vTaskDelay(10 / portTICK_PERIOD_MS);
    OLED_init();
    vTaskDelay(10 / portTICK_PERIOD_MS);
    i2c_OLED_clear_display();
    vTaskDelay(10 / portTICK_PERIOD_MS);
    printf("MQ-3 warming up...\n");
    vTaskDelay(pdMS_TO_TICKS(5000));
  
    while (1)
    {
        int raw = 0;
        adc_oneshot_read(adc1_handle, MQ3_ADC_CHANNEL, &raw);

        float voltage = 0;
        if (calibrated)
        {
            int mv = 0;
            adc_cali_raw_to_voltage(cali_handle, raw, &mv);
            voltage = mv / 1000.0;
        }
        else {
            voltage = raw * (3.3 / 4095.0);
        }

        float alcohol = (raw - 200) * (500.0 / (3000 - 200));
        if (alcohol < 0) alcohol = 0;
        if (alcohol > 500) alcohol = 500;

        float alcoholLevel = raw; //- MaxValue;
        if (alcoholLevel < 0)
        {
            alcoholLevel = 0;
        }

        printf("Raw: %d | Voltage: %.3f V | Alcohol level: %.1f\n", raw, voltage, alcohol);

        char buffer1[32];

        sprintf(buffer1, "Alcohol: %.1f ppm", alcohol);

        OLED_print_line(0, buffer1);
        OLED_draw_bar(6, alcoholLevel, 2000.0);

        vTaskDelay(pdMS_TO_TICKS(1200));
    }
}
