# Projeto_final 
 Projeto final do embarcatech
# Descrição do Projeto
Este projeto implementa um relógio digital utilizando um Raspberry Pi Pico, um display OLED SSD1306 e um joystick para ajustes. O código gerencia a exibição do tempo, ajuste manual e armazenamento dos dados na memória flash.

## Componentes Utilizados
- **Raspberry Pi Pico W**
- **Display OLED SSD1306** (I2C)
- **Joystick analógico** (eixo X e Y)
- **Botão para ajustes**

## Funcionamento do Código

### 1. Inicialização de Hardware
O código inicia configurando os periféricos:
- Configura os pinos GPIO para os botões.
- Inicializa o ADC para capturar valores do joystick.
- Configura a comunicação I2C para o display OLED.
- Carrega o horário salvo na memória flash.

### 2. Atualização do Relógio
O tempo é atualizado a cada segundo através de um **timer repetitivo**. O temporizador incrementa segundos e, quando necessário, ajusta minutos, horas, dias, meses e anos. Se houver mudança na hora, a nova configuração é salva na memória flash.

### 3. Exibição no Display OLED
A função `atualizar_display()` atualiza o display OLED com:
- **Hora atual** (formato 24h ou 12h com AM/PM)
- **Data atual** (DD/MM/AAAA)

### 4. Ajuste de Hora e Formato
- **Botão A:** Alterna entre modo ajuste e normal.
- **Botão do Joystick:** Alterna entre formato 12h e 24h.
- **Joystick (Eixo X):** Ajusta as horas.
- **Joystick (Eixo Y):** Ajusta os minutos.

As alterações são salvas na memória flash para serem recuperadas após reinicialização.

## Como Compilar e Executar
1. Compile o código utilizando o SDK do Raspberry Pi Pico.
2. Carregue o firmware no Raspberry Pi Pico.
3. Conecte o hardware corretamente.
4. O relógio iniciará automaticamente.