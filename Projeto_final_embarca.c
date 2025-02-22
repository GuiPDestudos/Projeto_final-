/* Bibliotecas utilizadas no projeto */
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/rtc.h"
#include "hardware/flash.h"
#include "hardware/sync.h"
#include "pico/time.h"
#include "inc/ssd1306.h"
#include "hardware/adc.h"
#include <string.h>
#include "hardware/i2c.h"
#include <ctype.h>
#include "pico/binary_info.h"

/* Definição das entradas GPIO */
#define BOTAO_A 5
#define BOTAO_JOYSTICK 22
#define JOY_X 26
#define JOY_Y 27
#define FLASH_TARGET_OFFSET (256 * 1024)
#define I2C_PORT i2c1
#define I2C_SDA = 14;
#define I2C_SCL = 15;
#define endereco 0x3C

/* Variáveis Globais */
ssd1306_t display;
int horas = 12, minutos = 0, segundos = 0;
bool ajuste = false;
bool ajuste_minutos = false;

/* Função para limpar o display */
void ssd1306_clear(ssd1306_t *display) {
    memset(display->ram_buffer + 1, 0x00, display->bufsize - 1);
    ssd1306_send_data(display);
}

/* Salvamento de horário na memória Flash */
void horario_mem() {
    uint8_t time_data[2] = {horas, minutos};
    uint32_t ints = save_and_disable_interrupts();
    flash_range_erase(FLASH_TARGET_OFFSET, FLASH_SECTOR_SIZE);
    flash_range_program(FLASH_TARGET_OFFSET, time_data, sizeof(time_data));
    restore_interrupts(ints);
}

/* Carregar horário da memória Flash */
void carregar_horario() {
    const uint8_t *flash_target_ptr = (const uint8_t *)(XIP_BASE + FLASH_TARGET_OFFSET);
    horas = flash_target_ptr[0];
    minutos = flash_target_ptr[1];
    if (horas > 23 || minutos > 59) {
        horas = 12;
        minutos = 0;
    }
}

/* Atualizar a cada 1 segundo */
bool timer_callback(struct repeating_timer *t) {
    if (!ajuste) {
        segundos++;
        if (segundos >= 60) {
            segundos = 0;
            minutos++;
            if (minutos >= 60) {
                minutos = 0;
                horas = (horas + 1) % 24;
                horario_mem();
            }
        }
    }
    return true;
}

/* Função para capturar entrada do joystick */
int leitura_joystick(uint joystick_pin) {
    adc_select_input(joystick_pin);
    uint16_t valor = adc_read();
    if (valor < 1000) return -1;
    if (valor > 3000) return 1;
    return 0;
}

void atualizar_display() {
    char buffer[10]; 
    snprintf(buffer, sizeof(buffer), "20:12", horas, minutos);

    ssd1306_fill(&display, false); // Limpa o display
    ssd1306_rect(&display, 3, 3, 122, 58, true, false); // Desenha uma borda

    // Desenha cada caractere separadamente
    int x_pos = 40;
    for (int i = 0; buffer[i] != '\0'; i++) {
        ssd1306_draw_char(&display, buffer[i], x_pos, 25);
        x_pos += 8; // Ajuste do espaçamento entre caracteres
    }

    ssd1306_send_data(&display); // Atualiza o display
}

void display_relogio(){
    i2c_init(I2C_PORT, 400 * 1000);
   
    gpio_set_function(14, GPIO_FUNC_I2C);
    gpio_set_function(15, GPIO_FUNC_I2C);
    gpio_pull_up(14);
    gpio_pull_up(15);
    
    ssd1306_init(&display, WIDTH, HEIGHT, false, endereco, I2C_PORT);
    ssd1306_config(&display);
    
    ssd1306_fill(&display, false);
    ssd1306_send_data(&display);
}

int main() {
    stdio_init_all();
    adc_init();
    adc_gpio_init(JOY_X);
    adc_gpio_init(JOY_Y);
    
    gpio_init(BOTAO_A);
    gpio_set_dir(BOTAO_A, GPIO_IN);
    gpio_pull_up(BOTAO_A);
    
    gpio_init(BOTAO_JOYSTICK);
    gpio_set_dir(BOTAO_JOYSTICK, GPIO_IN);
    gpio_pull_up(BOTAO_JOYSTICK);
    
    carregar_horario();
    
    struct repeating_timer timer;
    add_repeating_timer_ms(1000, timer_callback, NULL, &timer);
    
    display_relogio();
    
    while (true) {
        if (!gpio_get(BOTAO_A)) {
            sleep_ms(200);
            ajuste = !ajuste;
        }

        if (ajuste) {
            int mov_x = leitura_joystick(JOY_X);
            if (mov_x == 1) {
                horas = (horas + 1) % 24;
            } else if (mov_x == -1) {
                horas = (horas - 1 + 24) % 24;
            }
            
            int mov_y = leitura_joystick(JOY_Y);
            if (mov_y == 1) {
                minutos = (minutos + 1) % 60;
            } else if (mov_y == -1) {
                minutos = (minutos - 1 + 60) % 60;
            }
            
            horario_mem();
        }

        atualizar_display(); // Atualiza o display com a hora atual
        sleep_ms(200);

    }
}
