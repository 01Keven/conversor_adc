#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"
#include "hardware/adc.h"
#include "hardware/i2c.h"
#include "inc/ssd1306.h"
#include "inc/font.h"

// Definições dos pinos e parâmetros
#define I2C_PORT i2c1  // Porta I2C utilizada
#define I2C_SDA 14     // Pino SDA do I2C
#define I2C_SCL 15     // Pino SCL do I2C
#define SSD1306_ADDR 0x3C  // Endereço do display SSD1306
#define BUTTON_A 5     // Pino do botão A
#define BUTTON_B 6     // Pino do botão B
#define BUTTON_JOY 22  // Pino do botão do joystick
#define LED_GREEN 11   // Pino do LED verde
#define LED_BLUE 12    // Pino do LED azul
#define LED_RED 13   // Pino do LED vermelho
#define JOYSTICK_X_PIN 26   // Pino do eixo X do joystick
#define JOYSTICK_Y_PIN 27   // Pino do eixo Y do joystick
#define JOYSTICK_DEADZONE 40  // Zona morta do joystick

// Ajuste do centro do joystick
#define JOYSTICK_CENTER_X 1939 // Valor de centro do eixo X
#define JOYSTICK_CENTER_Y 2180 // Valor de centro do eixo Y

// Parâmetros da tela e quadrado
#define SQUARE_SIZE 8    // Tamanho do quadrado
#define SCREEN_WIDTH 128 // Largura da tela
#define SCREEN_HEIGHT 64 // Altura da tela

// Posição inicial do quadrado (centro da tela)
int square_pos_x = SCREEN_WIDTH / 2 - SQUARE_SIZE / 2;
int square_pos_y = SCREEN_HEIGHT / 2 - SQUARE_SIZE / 2;

// Variáveis globais
static volatile bool pwm_enabled = true;  // Flag para controlar o PWM
static volatile uint32_t last_button_a_time = 0;  // Controle de debounce do botão A
static volatile uint32_t last_button_joy_time = 0;  // Controle de debounce do botão do joystick
static volatile bool is_border_thick = false;  // Alterna a espessura da borda
static volatile bool is_square_red = true;    // Controle da cor do quadrado

// Prototipos das funções de interrupção
static void gpio_irq_handler(uint gpio, uint32_t events);

// Inicialização do PWM
void pwm_setup(uint pin) {
    gpio_set_function(pin, GPIO_FUNC_PWM);  // Configura o pino como PWM
    uint slice = pwm_gpio_to_slice_num(pin); // Obtém o slice do pino
    pwm_set_clkdiv(slice, 4.0);  // Configura o divisor de clock do PWM
    pwm_set_wrap(slice, 65535);  // Define o valor máximo do contador PWM
    pwm_set_gpio_level(pin, 0);  // Inicializa o nível do PWM em 0 (desligado)
    pwm_set_enabled(slice, true);// Habilita o PWM no slice
}

// Define o brilho do LED com base no valor do ADC
void set_led_brightness(uint pin, uint16_t adc_value, int16_t center_value) {
    if (!pwm_enabled) {  
        pwm_set_gpio_level(pin, 0);  // Se PWM estiver desativado, desliga o LED
        return;
    }

    int16_t offset = adc_value - center_value; // Calcula o offset do valor do ADC em relação ao centro
    if (offset > -JOYSTICK_DEADZONE && offset < JOYSTICK_DEADZONE) {
        pwm_set_gpio_level(pin, 0);  // Se dentro da zona morta, desliga o LED
        return;
    }

    uint16_t max_offset = (offset > 0) ? (4095 - center_value) : (center_value - 0);
    uint16_t pwm_value = ((abs(offset) - JOYSTICK_DEADZONE) * 65535) / (max_offset - JOYSTICK_DEADZONE);
    
    pwm_set_gpio_level(pin, pwm_value); // Ajusta o brilho do LED com base no valor calculado
}

// Lê um valor do ADC
uint16_t read_adc(uint channel) {
    adc_select_input(channel);  // Seleciona o canal do ADC
    return adc_read();  // Lê o valor do ADC
}

// Mapeia os valores do ADC para a tela SSD1306
int map_adc_to_screen(int adc_value, int center_value, int screen_max) {
    int range_min = center_value;     // Valor mínimo da faixa de mapeamento
    int range_max = 4095 - center_value;  // Valor máximo da faixa de mapeamento
    
    int offset = adc_value - center_value; // Calcula o deslocamento do valor do ADC

    int mapped_value;
    if (offset < 0) {
        mapped_value = ((offset * (screen_max / 2)) / range_min) + (screen_max / 2); // Mapeia valores negativos
    } else {
        mapped_value = ((offset * (screen_max / 2)) / range_max) + (screen_max / 2); // Mapeia valores positivos
    }

    if (mapped_value < 0) mapped_value = 0; // Limita o valor mínimo
    if (mapped_value > screen_max) mapped_value = screen_max; // Limita o valor máximo

    return mapped_value;
}

// Atualiza a posição do quadrado no display com base no joystick
void update_square_position() {
    uint16_t adc_x = read_adc(1);  // Lê o valor do eixo X do joystick
    uint16_t adc_y = read_adc(0);  // Lê o valor do eixo Y do joystick

    int border_offset = is_border_thick ? 3 : 1;  // Ajusta a borda com base na espessura

    // Mapeia os valores do joystick para a posição do quadrado na tela
    square_pos_x = map_adc_to_screen(adc_x, JOYSTICK_CENTER_X, SCREEN_WIDTH - SQUARE_SIZE - border_offset);
    square_pos_y = SCREEN_HEIGHT - SQUARE_SIZE - map_adc_to_screen(adc_y, JOYSTICK_CENTER_Y, SCREEN_HEIGHT - SQUARE_SIZE - border_offset);

    // Impede que o quadrado saia da borda da tela
    if (square_pos_x < border_offset) square_pos_x = border_offset;
    if (square_pos_y < border_offset) square_pos_y = border_offset;
}

// Desenha a borda no display
void draw_border(ssd1306_t *ssd) {
    int thickness = is_border_thick ? 3 : 1; // Define a espessura da borda

    // Desenha as linhas horizontais e verticais para a borda
    for (int i = 0; i < thickness; i++) {
        ssd1306_hline(ssd, 0, SCREEN_WIDTH - 2, i, true);
        ssd1306_hline(ssd, 0, SCREEN_WIDTH - 2, SCREEN_HEIGHT - 2 - i, true);
        ssd1306_vline(ssd, i, 0, SCREEN_HEIGHT - 2, true);
        ssd1306_vline(ssd, SCREEN_WIDTH - 2 - i, 0, SCREEN_HEIGHT - 2, true);
    }
}

// Atualiza o display SSD1306
void update_display(ssd1306_t *ssd) {
    ssd1306_fill(ssd, false);  // Limpa a tela
    draw_border(ssd);  // Desenha a borda

    // Desenha o quadrado com base na cor definida
    if (is_square_red) {
        ssd1306_rect(ssd, square_pos_y, square_pos_x, SQUARE_SIZE, SQUARE_SIZE, true, false); // Quadrado vermelho
    } else {
        ssd1306_rect(ssd, square_pos_y, square_pos_x, SQUARE_SIZE, SQUARE_SIZE, false, true); // Quadrado azul
    }

    ssd1306_send_data(ssd);  // Envia os dados para o display
}

int main() {   
    stdio_init_all();  // Inicializa a comunicação padrão

    pwm_setup(LED_RED);  // Configura o PWM para o LED vermelho
    pwm_setup(LED_BLUE); // Configura o PWM para o LED azul

    gpio_init(LED_GREEN); // Inicializa o LED verde
    gpio_set_dir(LED_GREEN, GPIO_OUT);
    gpio_put(LED_GREEN, 0);  // Inicializa o LED verde apagado

    gpio_init(BUTTON_A);  // Inicializa o botão A
    gpio_set_dir(BUTTON_A, GPIO_IN);
    gpio_pull_up(BUTTON_A);  // Habilita o pull-up para o botão A
    gpio_set_irq_enabled_with_callback(BUTTON_A, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);  // Configura a interrupção para o botão A

    gpio_init(BUTTON_JOY); // Inicializa o botão do joystick
    gpio_set_dir(BUTTON_JOY, GPIO_IN);
    gpio_pull_up(BUTTON_JOY);  // Habilita o pull-up para o botão do joystick
    gpio_set_irq_enabled(BUTTON_JOY, GPIO_IRQ_EDGE_FALL, true);  // Configura a interrupção para o botão do joystick

    adc_init();  // Inicializa o ADC
    adc_gpio_init(JOYSTICK_X_PIN);  // Inicializa o pino do eixo X do joystick
    adc_gpio_init(JOYSTICK_Y_PIN);  // Inicializa o pino do eixo Y do joystick

    i2c_init(I2C_PORT, 400 * 1000);  // Inicializa a comunicação I2C
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);  // Configura os pinos SDA e SCL para I2C
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    ssd1306_t ssd;  // Declaração da estrutura do display SSD1306
    ssd1306_init(&ssd, 128, 64, false, SSD1306_ADDR, I2C_PORT);  // Inicializa o display SSD1306
    ssd1306_config(&ssd);  // Configura o display
    ssd1306_send_data(&ssd);  // Envia os dados de configuração para o display
    ssd1306_fill(&ssd, false);  // Limpa a tela do display
    ssd1306_send_data(&ssd);  // Envia os dados da tela limpa

    printf("Sistema Inicializado\n");  // Mensagem de inicialização

    while (true) {
        update_square_position();  // Atualiza a posição do quadrado com base no joystick
        update_display(&ssd);  // Atualiza o display

        // Ajusta o brilho dos LEDs com base no valor do ADC
        set_led_brightness(LED_BLUE, read_adc(0), JOYSTICK_CENTER_X);
        set_led_brightness(LED_RED, read_adc(1), JOYSTICK_CENTER_Y);

        printf("X: %d | Y: %d | PWM: %s\n", read_adc(0), read_adc(1), pwm_enabled ? "ligado" : "desligado");
        sleep_ms(50);  // Aguarda 50 ms antes de atualizar novamente
    }
}

// Função de interrupção para os botões
static void gpio_irq_handler(uint gpio, uint32_t events) {
    uint32_t now = time_us_32();  // Captura o timestamp do evento

    // Se o botão A for pressionado, alterna o estado do PWM
    if (gpio == BUTTON_A) {
        if (now - last_button_a_time < 200000) return;  // Verifica se o debounce foi respeitado
        last_button_a_time = now;
        
        pwm_enabled = !pwm_enabled;  // Alterna o estado do PWM
        printf("BUTTON A pressionado... PWM %s\n", pwm_enabled ? "Ativado" : "Desativado");
    }

    // Se o botão do joystick for pressionado, alterna a espessura da borda
    if (gpio == BUTTON_JOY) {
        if (now - last_button_joy_time < 200000) return;  // Verifica o debounce
        last_button_joy_time = now;

        is_border_thick = !is_border_thick;  // Alterna a espessura da borda
        gpio_put(LED_GREEN, !gpio_get(LED_GREEN));  // Alterna o LED verde
    }
}
