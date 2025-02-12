#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "ssd1306.h"
#include "onewire.h"

// Definições dos pinos
#define SENSOR_UMIDADE_GPIO 28 // GPIO 28 corresponds to ADC2
#define I2C_SDA 14
#define I2C_SCL 15

// Declare an ssd1306_t structure
ssd1306_t oled;

void configurar_hardware() {
    stdio_init_all();

    adc_init();
    adc_gpio_init(SENSOR_UMIDADE_GPIO);
    adc_select_input(2); // GPIO 28 corresponds to ADC2

    // Initialize the I2C interface
    i2c_init(i2c1, 1000000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    // Initialize the SSD1306 display
    ssd1306_init(&oled, 128, 64, 0x3C, i2c1);
}

float ler_tensao_umidade() {
    uint16_t valor_adc = adc_read();
    float tensao = (valor_adc * 3.3f) / 4095.0f;
    return tensao;
}

int main() {
    configurar_hardware();

    while (1) {
        float tensao_umidade = ler_tensao_umidade();
        float temperatura_solo = ler_temperatura_solo();
        
        printf("Tensão do sensor de umidade: %f \n", tensao_umidade);
        printf("Temperatura do solo: %f\n", temperatura_solo);

        // Clear the display
        ssd1306_clear(&oled);
        
        // Exibe a tensão de umidade no display
        char buffer[32];
        snprintf(buffer, sizeof(buffer), "Umidade: %.2f V", tensao_umidade);
        ssd1306_draw_string(&oled, 0, 0, 1, buffer);

        // Exibe a temperatura do solo no display
        snprintf(buffer, sizeof(buffer), "Temp Solo: %.2f C", temperatura_solo);
        ssd1306_draw_string(&oled, 0, 32, 1, buffer);

        // Update the display
        ssd1306_show(&oled);

        // Wait 2 seconds before the next reading
        sleep_ms(2000);
    }
}
