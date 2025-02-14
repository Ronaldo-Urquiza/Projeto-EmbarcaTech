//-----------------------------------------------------------------------------------------------------
// Bloco 1: Bibliotecas e Definições de Pinos
//-----------------------------------------------------------------------------------------------------
#include <stdio.h>                        // Biblioteca padrão para entrada/saída
#include "pico/stdlib.h"                  // Biblioteca da Raspberry Pi Pico para funcionalidades básicas
#include "hardware/adc.h"                 // Biblioteca para leitura de ADC (conversor analógico-digital)
#include "ssd1306.h"                      // Biblioteca para controle do display OLED SSD1306
#include "onewire.h"                      // Biblioteca para comunicação 1-Wire
#include "onewire_library.h"              // Biblioteca auxiliar do protocolo 1-Wire
#include "ds18b20.h"                      // Biblioteca específica para o sensor de temperatura DS18B20
#include "ow_rom.h"                       // Biblioteca auxiliar para dispositivos 1-Wire
#include "matriz_led.h"                   // Biblioteca para controle da matriz de LEDs
#include "hardware/timer.h"               // Biblioteca para gerenciamento de temporizadores de hardware

// Definições dos pinos de sensores e atuadores
#define SENSOR_UMIDADE_GPIO 28  // GPIO para o sensor de umidade do solo (Analógico)
#define SENSOR_LDR_GPIO 20      // GPIO para o sensor de luz (Digital)
#define DS18B20_GPIO 19         // GPIO para o sensor de temperatura do solo (Digital)
#define RELAY_GPIO 16           // GPIO para o relé de irrigação (Digital)
#define BUTTON_A 5              // GPIO do Botão A (Digital)
#define BUTTON_B 6              // GPIO do Botão B (Digital)

// Definições do barramento I2C para o display OLED
#define I2C_SDA 14
#define I2C_SCL 15

//-----------------------------------------------------------------------------------------------------
// Bloco 2: Configuração do ThingSpeak e Wi-Fi
//-----------------------------------------------------------------------------------------------------
#include "pico/cyw43_arch.h"   // Biblioteca de controle do Wi-Fi
#include "lwip/pbuf.h"         // Biblioteca do protocolo de rede TCP/IP
#include "lwip/tcp.h"          // Biblioteca para conexões TCP
#include "lwip/dns.h"          // Biblioteca para resolução de DNS
#include "lwip/init.h"         // Inicialização da pilha de rede

// Configuração da rede Wi-Fi
#define WIFI_SSID "Ronaldinho & Tati"  
#define WIFI_PASS "amora2023" 
#define THINGSPEAK_HOST "api.thingspeak.com"
#define THINGSPEAK_PORT 80

// Chave de API do ThingSpeak
#define API_KEY "HJNSE0EKYOW06C7Q"

// Estruturas para conexão TCP
struct tcp_pcb *tcp_client_pcb;
ip_addr_t server_ip;

//-----------------------------------------------------------------------------------------------------
// Bloco 3: Variáveis Globais
//-----------------------------------------------------------------------------------------------------
ssd1306_t oled;  // Estrutura para controle do display OLED
PIO pio = pio0;  // PIO utilizado para comunicação 1-Wire

uint offset;   // Variável usada para armazenar o deslocamento do programa 1-Wire no PIO (Programável I/O).
               // Esse valor será definido quando adicionarmos o programa 1-Wire na memória do PIO.

OW ow;         // Estrutura de dados usada para gerenciar a comunicação 1-Wire.
               // Essa estrutura é necessária para interagir com dispositivos 1-Wire, como o sensor DS18B20 de temperatura.

const float LIMIAR_UMIDADE = 1.5; // Definição do limiar de umidade do solo.
                                  // Se a tensão do sensor de umidade for maior ou igual a esse valor,
                                  // o solo será considerado "úmido". Caso contrário, será "seco".

//-----------------------------------------------------------------------------------------------------
// Bloco 4: Função de Configuração de Hardware
// Objetivo: Inicializar e configurar todos os sensores, atuadores e periféricos necessários.
//-----------------------------------------------------------------------------------------------------
void configurar_hardware() {
    stdio_init_all(); // Inicializa a comunicação serial para depuração via USB

    // **Configuração do ADC (Conversor Analógico-Digital) para leitura do sensor de umidade**
    adc_init(); // Inicializa o módulo ADC
    adc_gpio_init(SENSOR_UMIDADE_GPIO); // Habilita o GPIO 28 para entrada analógica (sensor de umidade)
    adc_select_input(2); // Define o ADC2 como entrada, que corresponde ao GPIO 28

    // **Configuração do sensor LDR (Sensor de Luz)**
    gpio_init(SENSOR_LDR_GPIO); // Inicializa o GPIO do LDR
    gpio_set_dir(SENSOR_LDR_GPIO, GPIO_IN); // Define o pino como entrada (para leitura do sensor)

    // **Configuração do Relé (para controle da irrigação)**
    gpio_init(RELAY_GPIO); // Inicializa o GPIO do relé
    gpio_set_dir(RELAY_GPIO, GPIO_OUT); // Define o pino como saída (para ativar/desativar a irrigação)
    gpio_put(RELAY_GPIO, false); // Garante que o relé esteja desligado inicialmente

    // **Configuração dos Botões**
    gpio_init(BUTTON_A); // Inicializa o GPIO do Botão A
    gpio_set_dir(BUTTON_A, GPIO_IN); // Define o Botão A como entrada
    gpio_pull_up(BUTTON_A); // Habilita resistor de pull-up interno (evita estados flutuantes)

    gpio_init(BUTTON_B); // Inicializa o GPIO do Botão B
    gpio_set_dir(BUTTON_B, GPIO_IN); // Define o Botão B como entrada
    gpio_pull_up(BUTTON_B); // Habilita resistor de pull-up interno (evita estados flutuantes)

    // **Configuração do barramento I2C (para comunicação com o display OLED)**
    i2c_init(i2c1, 1000000); // Inicializa o barramento I2C na velocidade de 1 MHz
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C); // Define o pino SDA como função I2C
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C); // Define o pino SCL como função I2C
    gpio_pull_up(I2C_SDA); // Habilita pull-up no pino SDA
    gpio_pull_up(I2C_SCL); // Habilita pull-up no pino SCL

    // **Inicialização do display OLED SSD1306**
    ssd1306_init(&oled, 128, 64, 0x3C, i2c1); // Configura o display com resolução 128x64 no endereço I2C 0x3C

    // **Inicialização do sensor de temperatura DS18B20 usando o protocolo 1-Wire**
    if (pio_can_add_program(pio, &onewire_program)) { // Verifica se o programa 1-Wire pode ser adicionado ao PIO
        offset = pio_add_program(pio, &onewire_program); // Adiciona o programa 1-Wire no PIO

        // Inicializa a comunicação 1-Wire para o sensor DS18B20
        if (!ow_init(&ow, pio, offset, DS18B20_GPIO)) {
            printf("Não foi possível inicializar o driver 1-Wire.\n"); // Exibe erro caso a inicialização falhe
        }
    } else {
        printf("Não foi possível adicionar o programa 1-Wire ao PIO.\n"); // Exibe erro caso não consiga adicionar o programa
    }
}

//-----------------------------------------------------------------------------------------------------
// Bloco 5: Funções de Leitura de Sensores
//-----------------------------------------------------------------------------------------------------

// Função que lê o estado do sensor LDR (Sensor de Luz)
// Retorna verdadeiro (1) se não houver luz e falso (0) se estiver claro.
bool ler_estado_ldr() {
    return gpio_get(SENSOR_LDR_GPIO); // Obtém o valor do pino do LDR (HIGH = escuro, LOW = claro)
}

// Função que lê a tensão do sensor de umidade do solo
// Retorna um valor em volts correspondente à umidade do solo.
float ler_tensao_umidade() {
    uint16_t valor_adc = adc_read(); // Lê o valor do ADC (de 0 a 4095)
    float tensao = (valor_adc * 3.3f) / 4095.0f; // Converte para tensão (0V a 3.3V)
    return tensao; // Retorna a tensão medida pelo sensor de umidade
}

// Função que lê a temperatura do solo utilizando o sensor DS18B20
// Retorna um valor em graus Celsius.
float ler_temperatura_solo() {
    float temperatura = 0.0; // Inicializa a variável de temperatura como 0.0

    // Reinicializa a comunicação 1-Wire para garantir que o sensor DS18B20 esteja pronto
    if (ow_reset(&ow)) {
        ow_send(&ow, OW_SKIP_ROM);        // Envia comando para ignorar o ROM (único sensor no barramento)
        ow_send(&ow, DS18B20_CONVERT_T);  // Comando para iniciar a conversão de temperatura
        sleep_ms(750); // Aguarda 750ms para garantir que a conversão seja concluída

        ow_reset(&ow);                     // Reinicia o barramento 1-Wire
        ow_send(&ow, OW_SKIP_ROM);         // Ignora ROM novamente
        ow_send(&ow, DS18B20_READ_SCRATCHPAD); // Comando para ler os dados de temperatura do sensor

        uint8_t temp_lsb = ow_read(&ow); // Lê o byte menos significativo da temperatura
        uint8_t temp_msb = ow_read(&ow); // Lê o byte mais significativo da temperatura

        // Combina os dois bytes lidos em um número inteiro de 16 bits
        int16_t temp = (temp_msb << 8) | temp_lsb;

        // Converte o valor para graus Celsius (cada unidade equivale a 1/16 °C)
        temperatura = temp / 16.0;
    } else {
        // Caso a comunicação com o sensor DS18B20 falhe, exibe uma mensagem no console
        printf("Falha na comunicação com o sensor DS18B20.\n");
    }
    
    return temperatura; // Retorna o valor da temperatura lida do sensor
}


//-----------------------------------------------------------------------------------------------------
// Bloco 6: Funções auxiliares dos sensores
// Objetivo: Criar variáveis de verificação baseadas nas leituras dos sensores
//-----------------------------------------------------------------------------------------------------

// Função que verifica se o solo está úmido com base na tensão lida do sensor de umidade
// Retorna verdadeiro (true) se a umidade for suficiente e falso (false) se o solo estiver seco.
bool getBoolUmidadeSolo(float tensao_umidade){
    bool umidade_solo = false; // Inicializa a variável como "seco"

    if (tensao_umidade >= LIMIAR_UMIDADE) { // Compara a tensão com o limiar de umidade
        umidade_solo = true;  // Se for maior ou igual ao limiar, considera o solo "úmido"
    } else {
        umidade_solo = false; // Caso contrário, considera o solo "seco"
    }

    return umidade_solo; // Retorna o estado do solo (úmido ou seco)
}


bool decidir_estado_plantinha(float umidade_solo, float temperatura_solo, bool ldr_ativo) {
    // Verifica se todas as condições para a felicidade da plantinha estão sendo atendidas
    if (umidade_solo == true &&                // O solo está úmido?
        temperatura_solo >= 20 &&              // A temperatura está acima de 20°C?
        temperatura_solo <= 35 &&              // A temperatura está abaixo de 35°C?
        ldr_ativo == false) {                  // Existe luz?
        
        return true;  // A plantinha está feliz !
    } 
    else {
        return false; // A plantinha não está feliz !
    }
}

//-----------------------------------------------------------------------------------------------------
// Bloco 7: Comunicação com ThingSpeak
//-----------------------------------------------------------------------------------------------------

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

// Callback quando a conexão TCP é estabelecida
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

//-----------------------------------------------------------------------------------------------------
// Bloco 8: Temporizador para Envio de Dados
//-----------------------------------------------------------------------------------------------------

//Função de callback do temporizador para envio de dados para o ThingSpeaks
bool repeating_timer_callback(struct repeating_timer *t) {
    printf("Enviando dados para o ThingSpeak...\n");
    dns_gethostbyname(THINGSPEAK_HOST, &server_ip, dns_callback, NULL);
    return true;  // Mantém o temporizador ativo
}

//-----------------------------------------------------------------------------------------------------
// Bloco 9: Função Principal
//-----------------------------------------------------------------------------------------------------
int main() {

    npInit(7);  // Inicializa a matriz de LEDs

    configurar_hardware();  // Chama a função que configura todos os periféricos e sensores

    // Variáveis que armazenam os estados da plantinha e da irrigação
    bool plantinha_feliz = false;  
    bool irrigacao_rele = false;  

    // Inicializa o Wi-Fi
    if (cyw43_arch_init()) {  // Tenta iniciar o módulo Wi-Fi
        printf("Falha ao iniciar Wi-Fi\n");  // Exibe mensagem de erro se falhar
        return 1;  // Encerra o programa com erro
    }

    cyw43_arch_enable_sta_mode();  // Configura o Wi-Fi no modo cliente
    printf("Conectando ao Wi-Fi...\n");

    // Tenta conectar à rede Wi-Fi
    if (cyw43_arch_wifi_connect_blocking(WIFI_SSID, WIFI_PASS, CYW43_AUTH_WPA2_MIXED_PSK)) {
        printf("Falha ao conectar ao Wi-Fi\n");  // Exibe erro se a conexão falhar
        return 1;  // Encerra o programa com erro
    }

    printf("Wi-Fi conectado!\n");  // Exibe mensagem de sucesso

    // **Configuração do temporizador para enviar dados ao ThingSpeak a cada 30 segundos**
    struct repeating_timer timer;
    add_repeating_timer_ms(30000, repeating_timer_callback, NULL, &timer);

    // **Loop infinito para monitoramento da plantinha**
    while (1) {

        // **Leitura do estado dos botões**
        bool button_a_state = gpio_get(BUTTON_A);  // HIGH = solto, LOW = pressionado
        bool button_b_state = gpio_get(BUTTON_B);  // HIGH = solto, LOW = pressionado

        // **Verifica se o Botão A foi pressionado**
        if (!button_a_state) {  // Se o botão A for pressionado...
            gpio_put(RELAY_GPIO, true);  // Ativa o relé (liga a irrigação)
            printf("Relé ativado pelo Botão A\n");  // Exibe mensagem no console
            irrigacao_rele = true;  // Atualiza variável de estado da irrigação
        }

        // **Verifica se o Botão B foi pressionado**
        if (!button_b_state) {  // Se o botão B for pressionado...
            gpio_put(RELAY_GPIO, false);  // Desativa o relé (desliga a irrigação)
            printf("Relé desativado pelo Botão B\n");  // Exibe mensagem no console
            irrigacao_rele = false;  // Atualiza variável de estado da irrigação
        }

        // **Atualiza os dados dos sensores**
        float tensao_umidade = ler_tensao_umidade();  // Lê a tensão do sensor de umidade
        float temperatura_solo = ler_temperatura_solo();  // Lê a temperatura do solo
        bool ldr_ativo = ler_estado_ldr();  // Lê o estado do sensor de luz

        // **Verifica o nível de umidade do solo**
        bool umidade_solo = getBoolUmidadeSolo(tensao_umidade);

        // **Decide se a plantinha está feliz ou não**
        plantinha_feliz = decidir_estado_plantinha(umidade_solo, temperatura_solo, ldr_ativo);

        // **Exibe os dados no monitor serial**
        printf("Tensão do sensor de umidade: %.2fV\n", tensao_umidade);
        printf("Umidade do solo: %s\n", umidade_solo ? "Úmido" : "Seco");
        printf("Temperatura do solo: %.2f°C\n", temperatura_solo);
        printf("Luz na plantinha?: %s\n", ldr_ativo ? "Não" : "Sim");
        printf("Irrigação: %s\n", irrigacao_rele ? "Ativada" : "Desativada");
        printf("Plantinha feliz: %s\n", plantinha_feliz ? "Sim" : "Não");

        // **Atualiza o display OLED**
        ssd1306_clear(&oled);  // Limpa a tela do display antes de atualizar os dados

        // **Exibe a umidade do solo no display**
        char buffer[32];  // Buffer para armazenar strings formatadas
        snprintf(buffer, sizeof(buffer), "Umidade solo: %s\n", umidade_solo ? "Umido" : "Seco");
        ssd1306_draw_string(&oled, 0, 0, 1, buffer);

        // **Exibe a temperatura do solo no display**
        snprintf(buffer, sizeof(buffer), "Temp. Solo: %.2f C", temperatura_solo);
        ssd1306_draw_string(&oled, 0, 16, 1, buffer);

        // **Exibe o estado da luz no display**
        snprintf(buffer, sizeof(buffer), "Luz: %s", ldr_ativo ? "Ausente" : "Detectada");
        ssd1306_draw_string(&oled, 0, 32, 1, buffer);

        // **Exibe o estado da irrigação no display**
        snprintf(buffer, sizeof(buffer), "Irrigacao: %s", irrigacao_rele ? "Ativada" : "Desativada");
        ssd1306_draw_string(&oled, 0, 48, 1, buffer);

        // **Atualiza o display OLED**
        ssd1306_show(&oled);

        // **Mostra a carinha feliz ou triste na matriz de LEDs**
        if (plantinha_feliz) {
            npCarinhaFeliz();  // Mostra carinha feliz na matriz de LEDs
        } else {
            npCarinhaTriste(); // Mostra carinha triste na matriz de LEDs
        }

        // **Aguarda 500 milissegundos antes da próxima leitura**
        sleep_ms(500);
    }

    // **Remove o programa 1-Wire do PIO antes de encerrar o código**
    pio_remove_program(pio, &onewire_program, offset);

    return 0;  // Retorna 0 indicando execução bem-sucedida
}
