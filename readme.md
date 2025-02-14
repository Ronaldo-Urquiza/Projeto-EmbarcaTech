# 🌿 Projeto de Monitoramento de Vaso de Planta

Este projeto visa monitorar as condições de um vaso de planta utilizando um **Raspberry Pi Pico**. Ele coleta dados ambientais essenciais e os exibe em um display OLED, além de enviar periodicamente as leituras para o **ThingSpeak**, permitindo o monitoramento remoto.

---

## 📋 Principais Funcionalidades

- **Monitoramento de Umidade do Solo**  
  - Sensor conectado ao **GPIO 28** (ADC2).  
  - Mede a umidade do solo com base na tensão lida e compara com um limiar pré-definido.

- **Leitura da Temperatura do Solo**  
  - Sensor **DS18B20** conectado ao **GPIO 19** via protocolo **1-Wire**.  
  - Conversão da leitura para graus Celsius.

- **Detecção de Luminosidade**  
  - Sensor **LDR** conectado ao **GPIO 20**.  
  - Verifica a presença ou ausência de luz.

- **Controle de Irrigação**  
  - Relé conectado ao **GPIO 16** para ativação/desativação da irrigação.  
  - Controle manual via botões nos **GPIO 5 e 6**.

- **Exibição Local e Remota**  
  - Dados exibidos em um display **OLED SSD1306** (via I2C: **SDA: GPIO 14**, **SCL: GPIO 15**).  
  - Envio periódico dos dados para **ThingSpeak**.

- **Feedback Visual**  
  - Uma matriz de LEDs exibe ícones indicando o estado da planta (feliz/triste).

---

## 🛠️ Configuração do Hardware

### Conexões dos Componentes

| Componente                | GPIO                  | Descrição |
|---------------------------|----------------------|-----------|
| **Sensor de Umidade**     | GPIO 28 (ADC2)       | Mede umidade do solo |
| **Sensor DS18B20**        | GPIO 19              | Mede temperatura via 1-Wire |
| **Sensor LDR**            | GPIO 20              | Detecta luminosidade |
| **Relé (Irrigação)**      | GPIO 16              | Ativa/desativa irrigação |
| **Botão A**               | GPIO 5               | Liga irrigação manualmente |
| **Botão B**               | GPIO 6               | Desliga irrigação manualmente |
| **Display OLED SSD1306**  | SDA: GPIO 14, SCL: GPIO 15 | Comunicação I2C com o display |

---

## 🌐 Configuração do Wi-Fi e ThingSpeak

Este projeto utiliza **Wi-Fi** para enviar os dados ao **ThingSpeak**.

### Credenciais e Configuração

- **Wi-Fi**  
  - **SSID:** `"Ronaldinho & Tati"`  
  - **Senha:** `"amora2023"`  
  - *Edite `Projeto-Final.c` para usar sua rede:*  
  
  ```c
  #define WIFI_SSID "SUA_REDE"
  #define WIFI_PASS "SUA_SENHA"
  ```

- **ThingSpeak**  
  - **Host:** `api.thingspeak.com`
  - **Porta:** `80`
  - **API Key:** `"HJNSE0EKYOW06C7Q"`  
  - *Edite `Projeto-Final.c` para inserir sua chave:*  
  
  ```c
  #define API_KEY "SUA_CHAVE_DO_THINGSPEAK"
  ```

### Dados Enviados

Os seguintes dados são enviados a cada **30 segundos**:

| Field   | Descrição |
|---------|--------------------------------|
| **Field1** | Temperatura do solo (°C) |
| **Field2** | Umidade do solo (1 = Úmido, 0 = Seco) |
| **Field3** | Luminosidade (1 = Escuro, 0 = Claro) |
| **Field4** | Estado da irrigação (1 = Ativada, 0 = Desativada) |
| **Field5** | Estado da planta (1 = Feliz, 0 = Triste) |

---

## 🖥️ Configuração do Software

### Requisitos

- **Raspberry Pi Pico SDK** configurado.
- **Ferramentas de compilação:** CMake, Make e um compilador ARM.

### Compilação

1. **Clone o repositório:**

   ```bash
   git clone https://github.com/seu-usuario/projeto-monitoramento-planta.git
   cd projeto-monitoramento-planta
   ```

2. **Crie a pasta de compilação:**

   ```bash
   mkdir build
   cd build
   ```

3. **Configure o projeto:**

   ```bash
   cmake ..
   ```

4. **Compile o código:**

   ```bash
   make
   ```

5. **Envie o código para o Raspberry Pi Pico:**

   - Conecte o Pico segurando **BOOTSEL** e conecte via USB.
   - Copie o arquivo `.uf2` para a unidade montada.

---

## 📈 Monitoramento e Visualização

- **Serial Monitor:** Exibe leituras dos sensores e status do sistema.
- **Display OLED:** Exibe informações locais da planta.
- **Matriz de LEDs:** Mostra "carinha feliz" ou "carinha triste".
- **ThingSpeak:** Armazena e exibe gráficos com os dados coletados.

---

## 📚 Documentação e Contribuição

- **Comentários no Código:** Consulte os arquivos `.c` e `.h` para explicações detalhadas.

---

Happy coding! 🚀🌱
