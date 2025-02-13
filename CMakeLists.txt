# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.1.0)
set(toolchainVersion 13_3_Rel1)
set(picotoolVersion 2.1.0)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico CACHE STRING "Board type")

# Definição da versão mínima do CMake necessária
cmake_minimum_required(VERSION 3.13)

# Definição do padrão das linguagens C e C++
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Inicialização do SDK do Pico
include(pico_sdk_import.cmake)

# Nome do projeto e linguagens utilizadas
project(Projeto-Final C CXX ASM)

# Inicialização do SDK do Pico
pico_sdk_init()

# Adição do subdiretório da biblioteca OneWire
add_subdirectory(onewire_library)

# Criação do executável com os arquivos fonte necessários
add_executable(Projeto-Final
    Projeto-Final.c
    ssd1306.c
)

# Definição do nome e versão do programa
pico_set_program_name(Projeto-Final "Projeto-Final")
pico_set_program_version(Projeto-Final "0.1")

# Configuração de entrada/saída padrão (desabilita UART e habilita USB)
pico_enable_stdio_uart(Projeto-Final 0)
pico_enable_stdio_usb(Projeto-Final 1)

# Inclusão dos diretórios de cabeçalhos necessários
target_include_directories(Projeto-Final PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/onewire_library
)

# Vinculação das bibliotecas necessárias ao executável
target_link_libraries(Projeto-Final
    pico_stdlib
    hardware_i2c
    hardware_pio
    hardware_clocks
    hardware_adc
    onewire_library
)

# Geração de arquivos binários adicionais (UF2, bin, etc.)
pico_add_extra_outputs(Projeto-Final)
