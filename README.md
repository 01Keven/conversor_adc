# Coversor ADC BitDogLab

Este projeto foi desenvolvido para controlar um display OLED SSD1306, um joystick, e LEDs utilizando um microcontrolador Raspberry Pi Pico. O código realiza a leitura de um joystick analógico, o controle do brilho de LEDs via PWM, a movimentação de um quadrado na tela, e o controle de uma borda no display. Além disso, oferece funcionalidades como alternância de espessura de borda e ativação/desativação de PWM. O projeto é configurado para ser utilizado com o Raspberry Pi Pico W.

## Vídeo Explicativo
https://drive.google.com/file/d/1BHY_ywPqlqkGndiYy8qINnLnzzpUTCX0/view?usp=sharing

---

## Como Clonar o Repositório

1. **Clonar o Repositório**: Após instalar o Git, abra o terminal e execute o comando abaixo para clonar o repositório para sua máquina local:

   ```bash
   git clone https://github.com/01Keven/conversor_adc.git
   ```

## Descrição do Código

### 1. **Configuração do I2C e Display SSD1306**

O código utiliza a comunicação **I2C** para controlar o display OLED **SSD1306**. Ele é inicializado com as funções da biblioteca `ssd1306.h` para configurar e atualizar a tela com as informações do joystick e LEDs.

### 2. **Controle de LEDs via PWM**

A função `pwm_setup()` é usada para configurar os LEDs com controle PWM (Pulse Width Modulation). O PWM permite controlar o brilho dos LEDs com base nos valores lidos dos eixos do joystick.

- O LED vermelho e o LED azul são controlados via PWM.
- A função `set_led_brightness()` ajusta o brilho dos LEDs com base nos valores lidos dos eixos do joystick.

### 3. **Leitura do Joystick**

Os valores dos eixos **X** e **Y** do joystick são lidos usando o **ADC** (Conversor Analógico-Digital) do Raspberry Pi Pico. As leituras são mapeadas para coordenadas de tela para controlar a posição do quadrado no display SSD1306.

A função `map_adc_to_screen()` mapeia os valores lidos do ADC para a área visível do display, permitindo que o quadrado se mova de acordo com o movimento do joystick.

### 4. **Movimentação do Quadrado**

O quadrado, desenhado no display, é movido com base nos valores lidos do joystick. A função `update_square_position()` atualiza a posição do quadrado conforme o movimento do joystick.

### 5. **Controle de Borda**

A espessura da borda do display pode ser alternada entre fina e grossa. O botão do joystick é usado para alternar entre esses estados, e o LED verde é ativado ou desativado para indicar a mudança.

### 6. **Interrupções de Botões**

- **Botão A**: Alterna o estado do PWM (ligado/desligado). Quando pressionado, o PWM é ativado ou desativado, o que afeta o brilho dos LEDs.
- **Botão do Joystick**: Alterna a espessura da borda no display.

Ambos os botões têm controle de debounce para evitar múltiplas leituras rápidas quando pressionados.

### 7. **Funções de Interrupção**

As interrupções de GPIO são configuradas para os botões usando as funções `gpio_set_irq_enabled_with_callback()`. Quando os botões são pressionados, as funções de interrupção correspondentes (`gpio_irq_handler()`) são chamadas para alternar o estado do PWM e a espessura da borda.

### 8. **Funções de Exibição**

O display é atualizado a cada ciclo de execução do `while(true)` com a função `update_display()`, que redesenha a tela com o quadrado na nova posição e a borda ajustada.

### Estrutura de Funções

- **Função `pwm_setup()`**: Configura o PWM para o LED especificado.
- **Função `set_led_brightness()`**: Ajusta o brilho do LED com base na leitura do ADC.
- **Função `read_adc()`**: Lê o valor de um canal do ADC.
- **Função `map_adc_to_screen()`**: Mapeia os valores do ADC para coordenadas na tela.
- **Função `update_square_position()`**: Atualiza a posição do quadrado no display com base no joystick.
- **Função `draw_border()`**: Desenha a borda no display com espessura ajustável.
- **Função `update_display()`**: Atualiza a tela com o quadrado e a borda.
- **Função `gpio_irq_handler()`**: Função de interrupção para alternar o PWM e a espessura da borda.

## Explicação do que foi feito:
- Arquivo ssd1306.c: Implementação das funções básicas de inicialização e controle do display SSD1306.
- Arquivo ssd1306.h: Cabeçalho que define as funções e parâmetros utilizados para interação com o display.
- Configurações CMakeLists.txt: As configurações de hardware para PWM, ADC, clocks e I2C são ativadas para garantir o funcionamento correto do display.

## Dependências

- **pico-sdk**: SDK do Raspberry Pi Pico.
- **Bibliotecas SSD1306**: Para controlar o display OLED.
- **Bibliotecas de hardware**: Para controle de GPIO, PWM, ADC e I2C no Raspberry Pi Pico.

### Explicação do Código

#### 1. **Inclusões de Bibliotecas e Definições de Pinos**

```c
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"
#include "hardware/adc.h"
#include "hardware/i2c.h"
#include "inc/ssd1306.h"
#include "inc/font.h"
```
- As bibliotecas `pico/stdlib.h`, `hardware/pwm.h`, `hardware/clocks.h`, `hardware/adc.h`, e `hardware/i2c.h` fornecem funções para controle do Raspberry Pi Pico, como PWM, controle de relógios, ADC, e I2C.
- `ssd1306.h` e `font.h` são responsáveis por controlar o display OLED SSD1306 e desenhar texto.

#### 2. **Definições dos Pinos e Parâmetros**
```c
#define I2C_PORT i2c1  // Porta I2C utilizada
#define I2C_SDA 14     // Pino SDA do I2C
#define I2C_SCL 15     // Pino SCL do I2C
#define SSD1306_ADDR 0x3C  // Endereço do display SSD1306
//...
```
- Os pinos e parâmetros são definidos para I2C, botões, LEDs e joystick. O I2C é configurado para os pinos `SDA` e `SCL`, e o endereço do display SSD1306 é configurado como `0x3C`.

#### 3. **Variáveis Globais**
```c
static volatile bool pwm_enabled = true;  // Flag para controlar o PWM
static volatile uint32_t last_button_a_time = 0;  // Controle de debounce do botão A
//...
```
- Variáveis globais são utilizadas para controlar o estado do sistema, como o estado do PWM, debounce dos botões e outras configurações dinâmicas como a espessura da borda do display e a cor do quadrado.

#### 4. **Função `pwm_setup`**
```c
void pwm_setup(uint pin) {
    gpio_set_function(pin, GPIO_FUNC_PWM);  // Configura o pino como PWM
    //...
}
```
- Esta função configura um pino para gerar sinais PWM, com um divisor de clock e valor máximo do contador para controle de intensidade de LEDs.

#### 5. **Função `set_led_brightness`**
```c
void set_led_brightness(uint pin, uint16_t adc_value, int16_t center_value) {
    //...
    pwm_set_gpio_level(pin, pwm_value); // Ajusta o brilho do LED
}
```
- Controla o brilho de um LED com base no valor lido do ADC, mapeando os valores do joystick para o controle de intensidade do LED.

#### 6. **Função `read_adc`**
```c
uint16_t read_adc(uint channel) {
    adc_select_input(channel);  // Seleciona o canal do ADC
    return adc_read();  // Lê o valor do ADC
}
```
- Lê um valor de um canal específico do ADC.

#### 7. **Função `map_adc_to_screen`**
```c
int map_adc_to_screen(int adc_value, int center_value, int screen_max) {
    //...
    return mapped_value;
}
```
- Mapeia o valor lido do ADC para um valor correspondente na tela, para que a posição do quadrado seja ajustada de acordo com o movimento do joystick.

#### 8. **Função `update_square_position`**
```c
void update_square_position() {
    uint16_t adc_x = read_adc(1);  // Lê o valor do eixo X do joystick
    uint16_t adc_y = read_adc(0);  // Lê o valor do eixo Y do joystick
    //...
}
```
- Atualiza a posição de um quadrado na tela com base nos valores lidos do joystick, mapeando os valores dos eixos X e Y para coordenadas da tela.

#### 9. **Função `draw_border`**
```c
void draw_border(ssd1306_t *ssd) {
    int thickness = is_border_thick ? 3 : 1; // Define a espessura da borda
    // Desenha as linhas horizontais e verticais para a borda
}
```
- Desenha uma borda ao redor da tela SSD1306. A espessura da borda pode ser alterada com um botão.

#### 10. **Função `update_display`**
```c
void update_display(ssd1306_t *ssd) {
    ssd1306_fill(ssd, false);  // Limpa a tela
    draw_border(ssd);  // Desenha a borda
    //...
    ssd1306_send_data(ssd);  // Envia os dados para o display
}
```
- Atualiza o display com a posição do quadrado e a borda. A cor do quadrado alterna entre vermelho e azul.

#### 11. **Função `main`**
```c
int main() {   
    stdio_init_all();  // Inicializa a comunicação padrão
    pwm_setup(LED_RED);  // Configura o PWM para o LED vermelho
    //...
    while (true) {
        update_square_position();  // Atualiza a posição do quadrado
        update_display(&ssd);  // Atualiza o display
        //...
    }
}
```
- A função principal inicializa os periféricos, configura LEDs, botões, ADC e I2C, e então entra em um loop contínuo onde a posição do quadrado e o display são atualizados com base na entrada do joystick.

#### 12. **Função de Interrupção `gpio_irq_handler`**
```c
static void gpio_irq_handler(uint gpio, uint32_t events) {
    uint32_t now = time_us_32();  // Captura o timestamp do evento
    //...
}
```
- Esta função é chamada quando ocorre uma interrupção nos botões. Ela trata o debounce e alterna o estado de configurações, como o PWM e a espessura da borda.

---

## Como Executar

1. **Compilar o código** usando o Raspberry Pi Pico SDK.
2. **Carregar o binário compilado** no Raspberry Pi Pico.
3. **Conectar o display SSD1306 e o joystick** aos pinos correspondentes no Raspberry Pi Pico.
4. **Executar o código** e observar a interação com o display e LEDs.

