#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "ws2818b.pio.h"

#define LED_COUNT 25
#define LED_PIN 7

struct pixel_t {
  uint8_t G, R, B;
};
typedef struct pixel_t pixel_t;
typedef pixel_t npLED_t;

npLED_t leds[LED_COUNT];

PIO np_pio;
uint sm;

// Variável de controle de brilho (0 a 255)
uint8_t brightness = 128;

void npInit(uint pin) {
  uint offset = pio_add_program(pio0, &ws2818b_program);
  np_pio = pio0;
  sm = pio_claim_unused_sm(np_pio, false);
  if (sm < 0) {
    np_pio = pio1;
    sm = pio_claim_unused_sm(np_pio, true);
  }
  ws2818b_program_init(np_pio, sm, offset, pin, 800000.f);

  for (uint i = 0; i < LED_COUNT; ++i) {
    leds[i].R = 0;
    leds[i].G = 0;
    leds[i].B = 0;
  }
}

void npSetLED(const uint index, const uint8_t r, const uint8_t g, const uint8_t b) {
  leds[index].R = r;
  leds[index].G = g;
  leds[index].B = b;
}

void npClear() {
  for (uint i = 0; i < LED_COUNT; ++i)
    npSetLED(i, 0, 0, 0);
}

void npWrite() {
  for (uint i = 0; i < LED_COUNT; ++i) {
    pio_sm_put_blocking(np_pio, sm, (leds[i].G * brightness) / 255);
    pio_sm_put_blocking(np_pio, sm, (leds[i].R * brightness) / 255);
    pio_sm_put_blocking(np_pio, sm, (leds[i].B * brightness) / 255);
  }
  sleep_us(100);
}

int main_matriz() {
  stdio_init_all();
  npInit(LED_PIN);
  npClear();

  // Ligando todos os LEDs com a cor branca
  for (int i = 0; i < LED_COUNT; i++) {
    npSetLED(i, 255, 255, 255); // LEDs brancos
  }

  npWrite();

  while (true) {
    // Variação do brilho entre 0 e 255
    for (brightness = 0; brightness <= 255; brightness++) {
      npWrite();
      sleep_ms(1000);
    }
  //  for (brightness = 255; brightness > 0; brightness--) {
   //   npWrite();
   //   sleep_ms(200);
   // }
  }
}