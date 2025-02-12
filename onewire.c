// onewire.c
#include "onewire.h"
#include <stdio.h>
#include "pico/stdlib.h"
#include "onewire_library.h"
#include "ds18b20.h"
#include "ow_rom.h"

PIO pio = pio0;
uint gpio = 19;
float temperatura = 0.0;

float ler_temperatura_solo(void) {

    OW ow;
    uint offset;

    if (pio_can_add_program(pio, &onewire_program)) {
        offset = pio_add_program(pio, &onewire_program);
        if (ow_init(&ow, pio, offset, gpio)) {
            ow_reset(&ow);
            ow_send(&ow, OW_SKIP_ROM);
            ow_send(&ow, DS18B20_CONVERT_T);
            sleep_ms(750);
            ow_reset(&ow);
            ow_send(&ow, OW_SKIP_ROM);
            ow_send(&ow, DS18B20_READ_SCRATCHPAD);
            uint8_t temp_lsb = ow_read(&ow);
            uint8_t temp_msb = ow_read(&ow);
            int16_t temp = (temp_msb << 8) | temp_lsb;
            temperatura = temp / 16.0;
        } else {
            printf("Não foi possível inicializar o driver 1-Wire.\n");
        }
    } else {
        printf("Não foi possível adicionar o programa 1-Wire ao PIO.\n");
    }
    return temperatura;
}
