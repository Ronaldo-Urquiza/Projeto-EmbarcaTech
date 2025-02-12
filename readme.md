# 🌿 Projeto de Monitoramento de Vaso de Planta

Este projeto visa monitorar as condições de um vaso de planta utilizando um Raspberry Pi Pico. Os principais parâmetros monitorados são:

- **Umidade do Solo**: Medida através de um sensor de umidade conectado ao GPIO 28.
- **Temperatura do Solo**: Obtida por meio de um sensor DS18B20 conectado ao GPIO 19, utilizando o protocolo 1-Wire.
- **Luminosidade**: Monitorada por um LDR conectado ao GPIO 26.

Os dados coletados são exibidos em um display OLED SSD1306 via comunicação I2C.

## 📁 Estrutura do Projeto

- `Projeto-Final.c`: Arquivo principal que inicializa o hardware, lê os sensores e atualiza o display.
- `onewire.c` e `onewire.h`: Implementação das funções para comunicação com o sensor DS18B20 utilizando o protocolo 1-Wire.
- `ssd1306.c` e `ssd1306.h`: Biblioteca para controle do display OLED SSD1306.
- `onewire_library/`: Biblioteca auxiliar para comunicação 1-Wire.

## 🛠️ Configuração do Hardware

- **Sensor de Umidade do Solo**: Conectado ao GPIO 28 (ADC2).
- **Sensor DS18B20**: Conectado ao GPIO 19.
- **LDR**: Conectado ao GPIO 26 (ADC0).
- **Display OLED SSD1306**:
  - **SDA**: GPIO 14
  - **SCL**: GPIO 15

## 🖥️ Configuração do Software

- **CMakeLists.txt**: Configura o projeto para compilar os arquivos necessários e inclui as bibliotecas relevantes.

## 🚀 Como Compilar e Executar

1. Clone o repositório para o seu ambiente de desenvolvimento.
2. Certifique-se de que o SDK do Raspberry Pi Pico esteja corretamente configurado.
3. No terminal, navegue até o diretório do projeto e execute:

   ```bash
   mkdir build
   cd build
   cmake ..
   make
