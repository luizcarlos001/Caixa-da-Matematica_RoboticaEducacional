# VilaNova01 — Jogo de Matemática com Raspberry Pi Pico W

Projeto educacional para a **BitDogLab** com Raspberry Pi Pico W. O jogador resolve operações matemáticas (adição, multiplicação e divisão) usando um teclado matricial 4×4. A cada 5 acertos, um servo motor libera um prêmio e o buzzer toca uma fanfarra. O progresso é exibido em tempo real via interface web acessada por Wi-Fi.

---

## Sumário

- [Hardware necessário](#hardware-necessário)
- [Diagrama de conexões](#diagrama-de-conexões)
- [Servo Motor (GPIO 28)](#servo-motor-gpio-28)
- [Buzzer passivo (GPIO 21)](#buzzer-passivo-gpio-21)
- [Teclado matricial 4×4](#teclado-matricial-4x4)
- [Wi-Fi / Servidor Web](#wi-fi--servidor-web)
- [Mapa do teclado](#mapa-do-teclado)
- [Como o jogo funciona](#como-o-jogo-funciona)
- [Estrutura do projeto](#estrutura-do-projeto)
- [Como compilar e gravar](#como-compilar-e-gravar)

---

## Hardware necessário

| Componente | Quantidade |
|---|---|
| Raspberry Pi Pico W (BitDogLab) | 1 |
| Servo motor (ex: SG90) | 1 |
| Buzzer passivo | 1 (embutido na BitDogLab, GPIO 21) |
| Teclado matricial 4×4 | 1 |
| Cabos jumper | conforme necessário |

---

## Diagrama de conexões

```
Raspberry Pi Pico W
┌──────────────────────────────────┐
│                                  │
│  GPIO 28 ──────────── Servo (PWM)│
│  GPIO 21 ──────────── Buzzer PWM │
│                                  │
│  Teclado Matricial 4×4           │
│  GPIO 20 ──── Linha 0 (ROW 0)   │
│  GPIO 19 ──── Linha 1 (ROW 1)   │
│  GPIO 18 ──── Linha 2 (ROW 2)   │
│  GPIO 17 ──── Linha 3 (ROW 3)   │
│                                  │
│  GPIO 16 ──── Coluna 0 (COL 0)  │
│  GPIO  4 ──── Coluna 1 (COL 1)  │
│  GPIO  9 ──── Coluna 2 (COL 2)  │
│  GPIO  8 ──── Coluna 3 (COL 3)  │
│                                  │
└──────────────────────────────────┘
```

---

## Servo Motor (GPIO 28)

O servo é controlado por **PWM de 50 Hz** (período de 20 ms), padrão para servos RC.

| Configuração | Valor |
|---|---|
| Pino | GPIO 28 |
| Função | `GPIO_FUNC_PWM` |
| Clock divisor (`clkdiv`) | 125 → resolução de 1 µs por tick |
| `wrap` (período) | 20.000 ticks = 20 ms = 50 Hz |
| Posição fechada | 0° → pulso de ~500 µs |
| Posição aberta | 100° → pulso de ~1.611 µs |

**Fórmula do pulso:**
```
pulso (µs) = 500 + (angulo × 2000) / 180
```

**Estados:**
- `fechar_servo()` → 0° — posição padrão (prêmio bloqueado)
- `abrir_servo()` → 100° — libera o prêmio após 5 acertos

---

## Buzzer passivo (GPIO 21)

O buzzer passivo é integrado à BitDogLab e controlado por **PWM variável** para gerar diferentes frequências (notas musicais).

| Configuração | Valor |
|---|---|
| Pino | GPIO 21 |
| Função | `GPIO_FUNC_PWM` |
| Clock divisor (`clkdiv`) | 125 → 1 MHz de resolução |
| Fórmula do `wrap` | `1.000.000 / frequencia_hz` |
| Duty cycle | 50% (`wrap / 2`) — volume máximo |

**Sons do jogo:**

| Evento | Notas | Descrição |
|---|---|---|
| Acerto | C5 → E5 → G5 → C6 | Melodia alegre ascendente |
| Erro | 300 Hz → 200 Hz | Dois bipes graves descendentes |
| Prêmio | C5 → E5 → G5 → C6 → E6 → resolução | Fanfarra festiva |
| Tick (cronômetro) | A5 (880 Hz), 40 ms | Um bipe por segundo na contagem regressiva |

---

## Teclado matricial 4×4

O teclado usa varredura por linhas (**row scanning**): cada linha é ativada (LOW) uma por vez e as colunas são lidas com pull-up interno.

### Pinagem

| Sinal | GPIO |
|---|---|
| Linha 0 (ROW 0) | GPIO 20 |
| Linha 1 (ROW 1) | GPIO 19 |
| Linha 2 (ROW 2) | GPIO 18 |
| Linha 3 (ROW 3) | GPIO 17 |
| Coluna 0 (COL 0) | GPIO 16 |
| Coluna 1 (COL 1) | GPIO 4 |
| Coluna 2 (COL 2) | GPIO 9 |
| Coluna 3 (COL 3) | GPIO 8 |

### Configuração

- **Linhas (ROW):** saída digital, idle em HIGH
- **Colunas (COL):** entrada digital com `gpio_pull_up`
- **Debounce:** 20 ms de espera após detecção + aguarda soltar a tecla

---

## Mapa do teclado

```
     COL0   COL1   COL2   COL3
      (16)   (4)    (9)    (8)
ROW0  (20)  [ D ]  [ C ]  [ B ]  [ A ]
ROW1  (19)  [ # ]  [ 9 ]  [ 6 ]  [ 3 ]
ROW2  (18)  [ 0 ]  [ 8 ]  [ 5 ]  [ 2 ]
ROW3  (17)  [ * ]  [ 7 ]  [ 4 ]  [ 1 ]
```

### Teclas especiais no jogo

| Tecla | Função |
|---|---|
| `0`–`9` | Digita dígitos da resposta |
| `*` | Apaga a entrada atual |
| `#` | Confirma a resposta |
| `A`, `B`, `C`, `D` | Ignoradas pelo jogo |

---

## Wi-Fi / Servidor Web

O Pico W cria um **Access Point (AP)** próprio e sobe um servidor HTTP que exibe o estado do jogo em tempo real.

| Parâmetro | Valor |
|---|---|
| SSID | `projeto_matematica` |
| Senha | `123456789` |
| Segurança | WPA2-AES |
| IP do Pico W | `192.168.4.1` |
| URL de acesso | `http://192.168.4.1` |
| IPs distribuídos (DHCP) | `192.168.4.16` a `192.168.4.23` |

O DNS server redireciona qualquer domínio para `192.168.4.1`, funcionando como um **captive portal** simples.

A página web é atualizada pela função `web_update_data()`, que recebe:
- A conta atual (`num1 op num2`)
- Quantidade de acertos
- Dígitos já digitados
- Mensagem de feedback
- Evento (`normal`, `acerto`, `erro`, `premio`)
- Estado e tempo da contagem regressiva do prêmio

---

## Como o jogo funciona

1. O Pico W inicializa o Wi-Fi AP, o servidor HTTP e os periféricos.
2. Uma operação aleatória (adição, multiplicação ou divisão) é gerada e exibida no terminal serial e na página web.
3. O jogador digita a resposta no teclado matricial e pressiona `#` para confirmar.
4. Se **correto**: toca melodia de acerto, incrementa o contador de acertos e gera nova conta.
5. Se **errado**: toca bipes de erro, limpa a entrada e repete a mesma conta.
6. Ao atingir **5 acertos**: o servo abre (prêmio), toca a fanfarra, inicia contagem regressiva de 10 s com bipes de tick. Ao final, o servo fecha e o jogo reinicia.

### Tipos de operação sorteadas

| Tipo | Operandos | Resultado máximo |
|---|---|---|
| Adição (`+`) | 1–10 + 1–10 | 20 |
| Multiplicação (`x`) | 1–5 × 1–5 | 25 |
| Divisão (`/`) | divisor 1–5, quociente 1–5 | sempre inteiro exato |

---

## Estrutura do projeto

```
VilaNova01/
├── VilaNova01.c          # Lógica principal: jogo, servo, buzzer, teclado
├── CMakeLists.txt        # Build do projeto
├── pico_sdk_import.cmake
├── lwipopts.h            # Configurações do lwIP (TCP, memória, buffers)
├── inc/
│   ├── wifi_ap.h
│   └── http_server.h
├── src/
│   ├── wifi_ap.c         # Configura AP Wi-Fi + DHCP + DNS
│   └── http_server.c     # Servidor HTTP + atualização da página web
├── dhcpserver/
│   ├── dhcpserver.c
│   └── dhcpserver.h
└── dnsserver/
    ├── dnsserver.c
    └── dnsserver.h
```

---

## Como compilar e gravar

```bash
# Clonar / abrir o projeto e entrar na pasta
mkdir build && cd build

# Configurar com o SDK do Pico
cmake .. -DPICO_SDK_PATH=/caminho/para/pico-sdk

# Compilar
make -j4

# Gravar: segurar BOOTSEL no Pico, conectar USB, soltar BOOTSEL
# Copiar o .uf2 para a unidade RPI-RP2 que aparecer
cp VilaNova01.uf2 /media/$USER/RPI-RP2/
```

Após a gravação, o Pico reinicia automaticamente. Conecte-se à rede `projeto_matematica` e acesse `http://192.168.4.1` no navegador para acompanhar o jogo.
