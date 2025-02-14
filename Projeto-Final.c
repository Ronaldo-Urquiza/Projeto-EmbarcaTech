#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "ssd1306.h"
#include "onewire.h"
#include "onewire_library.h"
#include "ds18b20.h"
#include "ow_rom.h"
#include "matriz_led.h"
#include "hardware/timer.h" // Inclui a biblioteca para gerenciamento de temporizadores de hardware.

// Definições dos pinos
#define SENSOR_UMIDADE_GPIO 28  // GPIO para o sensor de umidade do solo (Analógcio)
#define SENSOR_LDR_GPIO 20      // GPIO para o sensor de luz (Digital)
#define DS18B20_GPIO 19         // GPIO para o sensor de temperatura (Digital)
#define RELAY_GPIO 16           // GPIO para o relé (Digital)
#define BUTTON_A 5              // GPIO conectado ao Botão A
#define BUTTON_B 6              // GPIO conectado ao Botão B

#define I2C_SDA 14
#define I2C_SCL 15

//Código para ThingsSpeaks----------------------------------------------------------------------------
#include "pico/cyw43_arch.h"
#include "lwip/pbuf.h"
#include "lwip/tcp.h"
#include "lwip/dns.h"
#include "lwip/init.h"

#define WIFI_SSID "Ronaldinho & Tati"
#define WIFI_PASS "amora2023"
#define THINGSPEAK_HOST "api.thingspeak.com"
#define THINGSPEAK_PORT 80

#define API_KEY "HJNSE0EKYOW06C7Q"  // Chave de escrita do ThingSpeak

struct tcp_pcb *tcp_client_pcb;
ip_addr_t server_ip;
//-----------------------------------------------------------------------------------------------------

// Estrutura do display SSD1306
ssd1306_t oled;

// Variáveis globais para o sensor DS18B20
PIO pio = pio0;
uint offset;
OW ow;

//Valor de tensão do sensor de umidade de solo definido para alcançar estado "úmido"
const float LIMIAR_UMIDADE = 1.5;

void configurar_hardware() {
    stdio_init_all();

    // Configuração do ADC para o sensor de umidade
    adc_init();
    adc_gpio_init(SENSOR_UMIDADE_GPIO);
    adc_select_input(2); // GPIO 28 corresponde ao ADC2

    // Configuração do pino do sensor LDR como entrada
    gpio_init(SENSOR_LDR_GPIO);
    gpio_set_dir(SENSOR_LDR_GPIO, GPIO_IN);

    // Configuração do pino do relé como saída
    gpio_init(RELAY_GPIO);
    gpio_set_dir(RELAY_GPIO, GPIO_OUT);
    gpio_put(RELAY_GPIO, false); // Garante que o relé esteja desligado inicialmente

    // Inicialização do GPIO do Botão A como entrada com pull-up interno
    gpio_init(BUTTON_A);
    gpio_set_dir(BUTTON_A, GPIO_IN);
    gpio_pull_up(BUTTON_A);

    // Inicialização do GPIO do Botão B como entrada com pull-up interno
    gpio_init(BUTTON_B);
    gpio_set_dir(BUTTON_B, GPIO_IN);
    gpio_pull_up(BUTTON_B);
    

    // Inicialização da interface I2C
    i2c_init(i2c1, 1000000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    // Inicialização do display SSD1306
    ssd1306_init(&oled, 128, 64, 0x3C, i2c1);

    // Inicialização do programa 1-Wire no PIO
    if (pio_can_add_program(pio, &onewire_program)) {
        offset = pio_add_program(pio, &onewire_program);
        if (!ow_init(&ow, pio, offset, DS18B20_GPIO)) {
            printf("Não foi possível inicializar o driver 1-Wire.\n");
        }
    } else {
        printf("Não foi possível adicionar o programa 1-Wire ao PIO.\n");
    }
}

bool ler_estado_ldr() {
    // Leitura do estado do LDR
    return gpio_get(SENSOR_LDR_GPIO);
}

bool getBoolUmidadeSolo(float tensao_umidade){
    bool umidade_solo = false;

    if (tensao_umidade >= LIMIAR_UMIDADE) {
        umidade_solo = true;
    } else {
        umidade_solo = false;
    }

    return umidade_solo;
}

float ler_tensao_umidade() {
    // Leitura da tensão do sensor de umidade
    uint16_t valor_adc = adc_read();
    float tensao = (valor_adc * 3.3f) / 4095.0f;
    return tensao;
}

float ler_temperatura_solo() {
    // Leitura da temperatura do solo usando o sensor DS18B20
    float temperatura = 0.0;
    if (ow_reset(&ow)) {
        ow_send(&ow, OW_SKIP_ROM);
        ow_send(&ow, DS18B20_CONVERT_T);
        sleep_ms(750); // Aguarda a conversão da temperatura
        ow_reset(&ow);
        ow_send(&ow, OW_SKIP_ROM);
        ow_send(&ow, DS18B20_READ_SCRATCHPAD);
        uint8_t temp_lsb = ow_read(&ow);
        uint8_t temp_msb = ow_read(&ow);

        int16_t temp = (temp_msb << 8) | temp_lsb;
        temperatura = temp / 16.0;
    } else {
        printf("Falha na comunicação com o sensor DS18B20.\n");
    }
    return temperatura;
}

bool decidir_estado_plantinha(float umidade_solo, float temperatura_solo, bool ldr_ativo){
    if(umidade_solo == true && temperatura_solo >= 20 && temperatura_solo <= 35 && ldr_ativo == false){
        return true;
    }
    else{
        return false;
    }
}

// Callback quando recebe resposta do ThingSpeak
static err_t http_recv_callback(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    if (p == NULL) {
        tcp_close(tpcb);
        return ERR_OK;
    }
    printf("Resposta do ThingSpeak: %.*s\n", p->len, (char *)p->payload);
    pbuf_free(p);
    return ERR_OK;
}

// **Callback quando a conexão TCP é estabelecida**
static err_t http_connected_callback(void *arg, struct tcp_pcb *tpcb, err_t err) {
    if (err != ERR_OK) {
        printf("Erro na conexão TCP\n");
        return err;
    }

    printf("Conectado ao ThingSpeak!\n");

    // **Leitura dos sensores**
    float temperatura_solo = ler_temperatura_solo();
    float tensao_umidade = ler_tensao_umidade();
    bool umidade_solo_bool = getBoolUmidadeSolo(tensao_umidade);
    bool ldr_ativo_bool = ler_estado_ldr();  // 1 = Escuro, 0 = Claro
    bool irrigacao_rele_bool = gpio_get(RELAY_GPIO);
    bool plantinha_feliz_bool = decidir_estado_plantinha(umidade_solo_bool, temperatura_solo, ldr_ativo_bool);

    // **Converte os valores booleanos para inteiros (ThingSpeak aceita apenas números)**
    int umidade_solo = umidade_solo_bool ? 1 : 0;
    int ldr_ativo = ldr_ativo_bool ? 0 : 1;
    int irrigacao_rele = irrigacao_rele_bool ? 1 : 0;
    int plantinha_feliz = plantinha_feliz_bool ? 1 : 0;

    // **Monta a requisição HTTP com apenas 5 fields**
    char request[512];
    snprintf(request, sizeof(request),
        "GET /update?api_key=%s"
        "&field1=%.2f"  // Temperatura do solo
        "&field2=%d"    // Umidade do solo (1 = Úmido, 0 = Seco)
        "&field3=%d"    // Luz (1 = Escuro, 0 = Claro)
        "&field4=%d"    // Irrigação ativa (1 = Ativada, 0 = Desativada)
        "&field5=%d"    // Plantinha feliz (1 = Sim, 0 = Não)
        " HTTP/1.1\r\n"
        "Host: %s\r\n"
        "Connection: close\r\n"
        "\r\n",
        API_KEY, temperatura_solo, umidade_solo, ldr_ativo, irrigacao_rele, plantinha_feliz,
        THINGSPEAK_HOST);

    tcp_write(tpcb, request, strlen(request), TCP_WRITE_FLAG_COPY);
    tcp_output(tpcb);
    tcp_recv(tpcb, http_recv_callback);

    return ERR_OK;
}

// Resolver DNS e conectar ao servidor
static void dns_callback(const char *name, const ip_addr_t *ipaddr, void *callback_arg) {
    if (ipaddr) {
        printf("Endereço IP do ThingSpeak: %s\n", ipaddr_ntoa(ipaddr));
        tcp_client_pcb = tcp_new();
        tcp_connect(tcp_client_pcb, ipaddr, THINGSPEAK_PORT, http_connected_callback);
    } else {
        printf("Falha na resolução de DNS\n");
    }
}

//Função de callback do temporizador
bool repeating_timer_callback(struct repeating_timer *t) {
    printf("Enviando dados para o ThingSpeak...\n");
    dns_gethostbyname(THINGSPEAK_HOST, &server_ip, dns_callback, NULL);
    return true;  // Mantém o temporizador ativo
}

int main() {

    npInit(7);
    configurar_hardware();
    
    bool plantinha_feliz = false;
    bool irrigacao_rele = false;

    if (cyw43_arch_init()) {
        printf("Falha ao iniciar Wi-Fi\n");
        return 1;
    }

    cyw43_arch_enable_sta_mode();
    printf("Conectando ao Wi-Fi...\n");

    if (cyw43_arch_wifi_connect_blocking(WIFI_SSID, WIFI_PASS, CYW43_AUTH_WPA2_MIXED_PSK)) {
        printf("Falha ao conectar ao Wi-Fi\n");
        return 1;
    }

    printf("Wi-Fi conectado!\n");

    // **Configuração do temporizador para enviar dados a cada 30 segundos**
    struct repeating_timer timer;
    add_repeating_timer_ms(30000, repeating_timer_callback, NULL, &timer);

    while (1) {

        // Leitura do estado dos botões
        bool button_a_state = gpio_get(BUTTON_A);  // HIGH = solto, LOW = pressionado
        bool button_b_state = gpio_get(BUTTON_B);  // HIGH = solto, LOW = pressionado

        // Verifica se o Botão A foi pressionado
        if (!button_a_state) {
            gpio_put(RELAY_GPIO, true);  // Ativa o relé
            printf("Relé ativado pelo Botão A\n");
            irrigacao_rele = true;
        }

        // Verifica se o Botão B foi pressionado
        if (!button_b_state) {
            gpio_put(RELAY_GPIO, false);  // Desativa o relé
            printf("Relé desativado pelo Botão B\n");
            irrigacao_rele = false;
        }
                
        // Atualiza dados em tempo real
        float tensao_umidade = ler_tensao_umidade();
        float temperatura_solo = ler_temperatura_solo();
        bool ldr_ativo = ler_estado_ldr();

        bool umidade_solo = getBoolUmidadeSolo(tensao_umidade);
        plantinha_feliz = decidir_estado_plantinha(umidade_solo, temperatura_solo, ldr_ativo);

        // Imprime no monitor serial para acompanhamento via USB
        printf("Tensão do sensor de umidade: %.2f\n", tensao_umidade);
        printf("Umidade do solo: %s\n", umidade_solo? "Umido" : "Seco");
        printf("Temperatura do solo: %.2f°C\n", temperatura_solo);
        printf("Luz na plantinha?: %s\n", ldr_ativo ? "Não" : "Sim");
        printf("Irrigição: %s\n", irrigacao_rele ? "Ativada" : "Desativada");
        printf("Plantinha feliz: %s\n", plantinha_feliz ? "Sim" : "Não");

        // Limpa o display
        ssd1306_clear(&oled);

        // Exibe a tensão de umidade no display
        char buffer[32];
        snprintf(buffer, sizeof(buffer), "Umidade solo: %s\n", umidade_solo? "Umido" : "Seco");
        ssd1306_draw_string(&oled, 0, 0, 1, buffer);

        // Exibe a temperatura do solo no display
        snprintf(buffer, sizeof(buffer), "Temp. Solo: %.2f C", temperatura_solo);
        ssd1306_draw_string(&oled, 0, 16, 1, buffer);

        // Exibe o estado do LDR no display
        snprintf(buffer, sizeof(buffer), "Luz: %s", ldr_ativo ? "Ausente" : "Detectada");
        ssd1306_draw_string(&oled, 0, 32, 1, buffer);

        // Exibe o estado do LDR no display
        snprintf(buffer, sizeof(buffer), "Irrigicao: %s", irrigacao_rele ? "Ativada" : "Desativada");
        ssd1306_draw_string(&oled, 0, 48, 1, buffer);

        // Atualiza o display
        ssd1306_show(&oled);

        if(plantinha_feliz){
            npCarinhaFeliz();
        }else{
            npCarinhaTriste();
        }

        // Aguarda 500 milissegundos antes da próxima leitura
        sleep_ms(500);
    }

    // Remove o programa 1-Wire do PIO antes de encerrar
    pio_remove_program(pio, &onewire_program, offset);
    return 0;
}
