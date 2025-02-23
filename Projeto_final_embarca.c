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
#define I2C_SDA 14
#define I2C_SCL 15
#define endereco 0x3C
#define WIDTH 128
#define HEIGHT 64

/* Variáveis Globais */
ssd1306_t display;
int horas = 15, minutos = 21, segundos = 0;
int dia = 23, mes = 02, ano = 2025;
bool ajuste = false;
bool formato_12h = false;

/* Função para limpar o display */
void ssd1306_clear(ssd1306_t *display) {
    memset(display->ram_buffer + 1, 0x00, display->bufsize - 1);
    ssd1306_send_data(display);
}

/* Salvamento de horário e data na memória Flash */
void horario_mem() {
    uint8_t time_data[5] = {horas, minutos, dia, mes, ano % 100};
    uint32_t ints = save_and_disable_interrupts();
    flash_range_erase(FLASH_TARGET_OFFSET, FLASH_SECTOR_SIZE);
    flash_range_program(FLASH_TARGET_OFFSET, time_data, sizeof(time_data));
    restore_interrupts(ints);
}

/* Carregar horário e data da memória Flash */
void carregar_horario() {
    const uint8_t *flash_target_ptr = (const uint8_t *)(XIP_BASE + FLASH_TARGET_OFFSET);
    horas = flash_target_ptr[0];
    minutos = flash_target_ptr[1];
    dia = flash_target_ptr[2];
    mes = flash_target_ptr[3];
    ano = 2000 + flash_target_ptr[4];
    if (horas > 23 || minutos > 59 || dia > 31 || mes > 12) {
        horas = 15; minutos = 21; dia = 23; mes = 02; ano = 2025;
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
                if (horas == 0) {
                    dia++;
                    if (dia > 31) { dia = 1; mes++; }
                    if (mes > 12) { mes = 1; ano++; }
                }
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
    char buffer[20];
    char data_buffer[20];
    
    int hora_display = formato_12h ? (horas % 12 == 0 ? 12 : horas % 12) : horas;
    char periodo[3] = "AM";
    if (formato_12h && horas >= 12) strcpy(periodo, "PM");
    
    ssd1306_fill(&display, false);
    ssd1306_rect(&display, 3, 3, 122, 58, true, false);
    sprintf(buffer, "%02d:%02d:%02d %s", hora_display, minutos, segundos, formato_12h ? periodo : "");
    ssd1306_draw_string(&display, buffer, 20, 25);
    sprintf(data_buffer, "%02d/%02d/%04d", dia, mes, ano);
    ssd1306_draw_string(&display, data_buffer, 30, 45);
    
    ssd1306_send_data(&display);
}

void display_relogio() {
    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
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
        
        if (!gpio_get(BOTAO_JOYSTICK)) {
            sleep_ms(200);
            formato_12h = !formato_12h;
        }
        
        if (ajuste) {
            int mov_x = leitura_joystick(JOY_X);
            if (mov_x == 1) horas = (horas + 1) % 24;
            else if (mov_x == -1) horas = (horas - 1 + 24) % 24;
            
            int mov_y = leitura_joystick(JOY_Y);
            if (mov_y == 1) minutos = (minutos + 1) % 60;
            else if (mov_y == -1) minutos = (minutos - 1 + 60) % 60;
            
            horario_mem();
        }
        
        atualizar_display();
        sleep_ms(200);
    }
}