#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "pico/stdio_usb.h"
#include "pico/cyw43_arch.h"

#include "wifi_ap.h"
#include "http_server.h"

// ================= PINOS =================
#define SERVO_PIN  28
#define BUZZER_PIN 21   // Buzzer passivo PWM da BitDogLab

const uint ROW_PINS[4] = {20, 19, 18, 17};
const uint COL_PINS[4] = {16, 4, 9, 8};

const char KEYMAP[4][4] = {
    {'D', 'C', 'B', 'A'},
    {'#', '9', '6', '3'},
    {'0', '8', '5', '2'},
    {'*', '7', '4', '1'}
};

// ================= SERVO =================
void servo_init(uint pin) {
    gpio_set_function(pin, GPIO_FUNC_PWM);
    uint slice = pwm_gpio_to_slice_num(pin);
    pwm_set_clkdiv(slice, 125.0f);
    pwm_set_wrap(slice, 20000);
    pwm_set_enabled(slice, true);
    pwm_set_gpio_level(pin, 1500);
}

void servo_angle(int angle) {
    uint16_t pulse = 500 + (angle * 2000) / 180;
    pwm_set_gpio_level(SERVO_PIN, pulse);
}

void abrir_servo() { servo_angle(100); }
void fechar_servo() { servo_angle(0); }

// ================= BUZZER =================
/*
 * O buzzer passivo da BitDogLab e controlado por PWM.
 * Configuramos o slice do GPIO 21 com clock de 1 MHz (clkdiv=125),
 * entao: wrap = 1.000.000 / frequencia_hz
 * O duty cycle de 50% (wrap/2) da o volume maximo no buzzer passivo.
 */

static uint buzzer_slice;

void buzzer_init() {
    gpio_set_function(BUZZER_PIN, GPIO_FUNC_PWM);
    buzzer_slice = pwm_gpio_to_slice_num(BUZZER_PIN);
    pwm_set_clkdiv(buzzer_slice, 125.0f);  // 1 MHz de resolucao
    pwm_set_enabled(buzzer_slice, false);  // comeca desligado
}

// Toca uma nota por 'ms' milissegundos
void buzzer_nota(uint freq_hz, uint ms) {
    if (freq_hz == 0) {
        sleep_ms(ms);
        return;
    }
    uint wrap = 1000000 / freq_hz;
    pwm_set_wrap(buzzer_slice, wrap);
    pwm_set_gpio_level(BUZZER_PIN, wrap / 2);  // 50% duty
    pwm_set_enabled(buzzer_slice, true);
    sleep_ms(ms);
    pwm_set_enabled(buzzer_slice, false);
}

// Pausa entre notas
void buzzer_pausa(uint ms) {
    pwm_set_enabled(buzzer_slice, false);
    sleep_ms(ms);
}

/*
 * Som de ACERTO: melodia alegre ascendente (Do-Mi-Sol-Do)
 * Notas: C5=523 E5=659 G5=784 C6=1047
 */
void som_acerto() {
    buzzer_nota(523, 100);  // Do
    buzzer_pausa(30);
    buzzer_nota(659, 100);  // Mi
    buzzer_pausa(30);
    buzzer_nota(784, 100);  // Sol
    buzzer_pausa(30);
    buzzer_nota(1047, 250); // Do alto
}

/*
 * Som de ERRO: dois bipes graves descendentes
 */
void som_erro() {
    buzzer_nota(300, 150);
    buzzer_pausa(60);
    buzzer_nota(200, 250);
}

/*
 * Som de PREMIO: fanfarra festiva
 * Toca uma sequencia ascendente e termina com nota longa
 */
void som_premio() {
    // Sequencia ascendente rapida
    buzzer_nota(523,  100); buzzer_pausa(20);  // Do
    buzzer_nota(659,  100); buzzer_pausa(20);  // Mi
    buzzer_nota(784,  100); buzzer_pausa(20);  // Sol
    buzzer_nota(1047, 100); buzzer_pausa(20);  // Do6
    buzzer_nota(1319, 100); buzzer_pausa(20);  // Mi6
    // Resolucao festiva
    buzzer_nota(1047, 150); buzzer_pausa(40);
    buzzer_nota(1319, 400);                    // nota longa final
}

/*
 * Som de CRONOMETRO: um bipe curto a cada segundo durante a contagem
 * Chamado uma vez por iteracao da contagem regressiva do premio
 */
void som_tick() {
    buzzer_nota(880, 40);  // La5 - bipe seco e agudo
}

// ================= TECLADO =================
void keypad_init() {
    for (int i = 0; i < 4; i++) {
        gpio_init(ROW_PINS[i]);
        gpio_set_dir(ROW_PINS[i], GPIO_OUT);
        gpio_put(ROW_PINS[i], 1);
    }
    for (int i = 0; i < 4; i++) {
        gpio_init(COL_PINS[i]);
        gpio_set_dir(COL_PINS[i], GPIO_IN);
        gpio_pull_up(COL_PINS[i]);
    }
}

char get_key() {
    for (int r = 0; r < 4; r++) {
        for (int i = 0; i < 4; i++) gpio_put(ROW_PINS[i], 1);
        gpio_put(ROW_PINS[r], 0);
        sleep_us(50);
        for (int c = 0; c < 4; c++) {
            if (gpio_get(COL_PINS[c]) == 0) {
                sleep_ms(20);
                if (gpio_get(COL_PINS[c]) == 0) {
                    while (gpio_get(COL_PINS[c]) == 0) sleep_ms(10);
                    return KEYMAP[r][c];
                }
            }
        }
    }
    return '\0';
}

// ================= JOGO =================
int num1, num2, resposta;
char op;
int acertos = 0;

void gerar_conta() {
    /*
     * Adicao:        parcelas de 1 a 10
     * Multiplicacao: fatores de 1 a 5
     * Divisao:       divisor de 1 a 5, quociente de 1 a 5 → dividendo max 25
     * Tipo sorteado aleatoriamente entre os tres.
     */
    int tipo = rand() % 3;  // 0 = adicao, 1 = multiplicacao, 2 = divisao

    if (tipo == 0) {
        op = '+';
        num1 = (rand() % 10) + 1;  // 1..10
        num2 = (rand() % 10) + 1;  // 1..10
        resposta = num1 + num2;
    }
    else if (tipo == 1) {
        op = 'x';
        num1 = (rand() % 5) + 1;   // 1..5
        num2 = (rand() % 5) + 1;   // 1..5
        resposta = num1 * num2;
    }
    else {
        op = '/';
        int divisor   = (rand() % 5) + 1;  // 1..5
        int quociente = (rand() % 5) + 1;  // 1..5
        num1    = divisor * quociente;      // sempre exato, max 25
        num2    = divisor;
        resposta = quociente;
    }

    printf("\n-------------------------\n");
    printf("Acertos: %d/5\n", acertos);
    printf("Resolva: %d %c %d = ?\n", num1, op, num2);
    printf("* apaga | # confirma\n");
    printf("-------------------------\n");
    fflush(stdout);

    web_update_data(num1, op, num2, acertos, "", "Nova conta! Vamos resolver!", "normal", 0, 0);
}

// ================= MAIN =================
int main() {
    stdio_init_all();

    absolute_time_t timeout = make_timeout_time_ms(10000);
    while (!stdio_usb_connected() && absolute_time_diff_us(get_absolute_time(), timeout) > 0) {
        sleep_ms(100);
    }

    printf("SERIAL OK!\n");
    fflush(stdout);

    if (cyw43_arch_init()) {
        printf("Erro ao iniciar Wi-Fi\n");
        return -1;
    }

    wifi_ap_init();
    http_server_init();

    servo_init(SERVO_PIN);
    buzzer_init();
    keypad_init();
    fechar_servo();

    srand(time_us_32());

    char buffer[10];
    int idx = 0;
    memset(buffer, 0, sizeof(buffer));

    gerar_conta();

    while (true) {
        cyw43_arch_poll();

        char key = get_key();

        if (key != '\0') {
            if (key == 'A' || key == 'B' || key == 'C' || key == 'D') continue;

            printf("Tecla: %c\n", key);
            fflush(stdout);

            if (key >= '0' && key <= '9') {
                if (idx < (int)sizeof(buffer) - 1) {
                    buffer[idx++] = key;
                    buffer[idx] = '\0';
                }
                printf("Digitado: %s\n", buffer);
                fflush(stdout);
                web_update_data(num1, op, num2, acertos, buffer, "Digitando resposta...", "normal", 0, 0);
            }
            else if (key == '*') {
                idx = 0;
                memset(buffer, 0, sizeof(buffer));
                printf("Apagado\n");
                fflush(stdout);
                web_update_data(num1, op, num2, acertos, "", "Apagado! Digite novamente.", "normal", 0, 0);
            }
            else if (key == '#') {
                if (idx == 0) {
                    web_update_data(num1, op, num2, acertos, "", "Digite uma resposta antes de confirmar!", "normal", 0, 0);
                    continue;
                }

                int valor = atoi(buffer);

                if (valor == resposta) {
                    acertos++;
                    printf("CORRETO!\n");
                    fflush(stdout);

                    web_update_data(num1, op, num2, acertos, buffer, "Muito bem! Resposta correta!", "acerto", 0, 0);
                    som_acerto();  // << melodia de acerto

                    if (acertos == 5) {
                        printf("PREMIO!\n");
                        fflush(stdout);

                        abrir_servo();
                        som_premio();  // << fanfarra do premio

                        // Contagem regressiva com bipe de cronometro a cada segundo
                        for (int t = 10; t >= 0; t--) {
                            web_update_data(num1, op, num2, acertos, buffer, "Parabens! Desafio concluido!", "premio", 1, t);

                            // Cada segundo = 10 iteracoes de 100ms + bipe no inicio
                            som_tick();  // bipe do cronometro
                            for (int i = 0; i < 9; i++) {
                                cyw43_arch_poll();
                                sleep_ms(100);
                            }
                            cyw43_arch_poll();
                        }

                        fechar_servo();

                        acertos = 0;
                        idx = 0;
                        memset(buffer, 0, sizeof(buffer));

                        printf("JOGO RESETADO!\n");
                        fflush(stdout);

                        web_update_data(num1, op, num2, acertos, "", "Jogo reiniciado. Nova rodada!", "normal", 0, 0);
                        sleep_ms(500);
                        gerar_conta();

                    } else {
                        idx = 0;
                        memset(buffer, 0, sizeof(buffer));
                        gerar_conta();
                    }

                } else {
                    printf("ERRADO!\n");
                    fflush(stdout);

                    idx = 0;
                    memset(buffer, 0, sizeof(buffer));

                    web_update_data(num1, op, num2, acertos, "", "Ops! Resposta errada. Tente novamente!", "erro", 0, 0);
                    som_erro();  // << bipes de erro
                }
            }
        }

        sleep_ms(20);
    }
}