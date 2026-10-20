/*
    Cristian Alexei Romero Martínez
    16 de octubre del 2026
*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/irq.h"
#include "hardware/i2c.h"

#include "hardware/structs/sio.h"
#include "hardware/structs/timer.h"
#include "hardware/regs/intctrl.h"

#define LED_PIN 25
#define BUTTON_PIN 13
#define LED_MASK (1u << LED_PIN)
#define BUTTON_MASK (1u << BUTTON_PIN)

#define SDA 16 // GP16
#define SCL 17 // GP17
#define BUTTON_A 18 // GP18
#define BUTTON_B 19 // GP18
#define OLED_ADDR 0x3C // Mueve el cursor

#define LINE_BUFFER_SIZE 64
#define MAX_ARGS 4

typedef int (*command_handler_t)(int argc, char **argv);

typedef struct {
    const char *name;
    const char *help;
    command_handler_t handler;
} command_t;

// Primarias
static int cmd_help(int argc, char **argv);
static int cmd_on(int argc, char **argv);
static int cmd_off(int argc, char **argv);
static int cmd_toggle(int argc, char **argv);
static int cmd_read(int argc, char **argv);
static int cmd_sio(int argc, char **argv);
static int cmd_mask(int argc, char **argv);
static int cmd_hola(int argc, char **argv);
static int cmd_timer_on(int argc, char **argv);
static int cmd_timer_off(int argc, char **argv);
static int sack(int argc, char **argv);
// Secundarias
//void timer0_isr();

static const command_t commands[] = {
    {"help",        "Muestra la lista de comandos ",        cmd_help},
    {"on",          "Enciende el LED integrado GPIO25",     cmd_on},
    {"off",         "Apaga el LED integrado GPIO25",        cmd_off},
    {"toggle",      "Invierte el estado del LED",           cmd_toggle},
    {"read",        "Lee el boton conectado a GPIO13",      cmd_read},
    {"sio",         "Muestra GPIO_IN, GPIO_OUT y GPIO_OE",  cmd_sio},
    {"mask",        "Muestra la mascara para GPIO25",       cmd_mask},
    {"hola",        "Muestra Hola mundo",                   cmd_hola},
    {"timer_on",    "Enciende el Temporizador",             cmd_timer_on},
    {"timer_off",   "Apaga el Temporizador",                cmd_timer_off},
    {"game",        "Arranca el juego SACK",                sack}
};

#define COMMAND_COUNT (sizeof(commands) / sizeof(commands[0]))

static void print_binary32(uint32_t value) {
    for (int bit = 31; bit >= 0; --bit) {
        putchar((value & (1u << bit)) ? '1' : '0');
        if ((bit % 4) == 0 && bit != 0) putchar(' ');
    }
}

static void read_line(char *buffer, size_t size) {
    size_t pos = 0;

    while (true) {
        int c = getchar();

        if (c == '\r' || c == '\n') {
            putchar('\n');
            buffer[pos] = '\0';
            return;
        }

        if (c == '\b' || c == 127) {
            if (pos > 0) {
                pos--;
                printf("\b \b");
            }
            continue;
        }

        if (pos < size - 1) {
            buffer[pos++] = (char)c;
            putchar(c);
        }
    }
}

static int split_line(char *line, char **argv, int max_args) {
    int argc = 0;
    char *token = strtok(line, " \t");

    while (token != NULL && argc < max_args) {
        argv[argc++] = token;
        token = strtok(NULL, " \t");
    }
    return argc;
}

static void execute_command(int argc, char **argv) {
    if (argc == 0) return;

    for (size_t i = 0; i < COMMAND_COUNT; ++i) {
        if (strcmp(argv[0], commands[i].name) == 0) {
            commands[i].handler(argc, argv);
            return;
        }
    }

    printf("Comando desconocido: %s\n", argv[0]);
    printf("Escribe 'help'.\n");
}

static int cmd_help(int argc, char **argv) {
    (void)argc; (void)argv;
    printf("\nComandos disponibles:\n");
    for (size_t i = 0; i < COMMAND_COUNT; ++i)
        printf("  %-8s - %s\n", commands[i].name, commands[i].help);
    printf("\n");
    return 0;
}


static int cmd_on(int argc, char **argv) {
    (void)argc; (void)argv;
    sio_hw->gpio_set = LED_MASK;
    printf("LED GPIO%d = 1\n", LED_PIN);
    printf("sio_hw->gpio_set = 1u << %d;\n", LED_PIN);
    return 0;
}

static int cmd_off(int argc, char **argv) {
    (void)argc; (void)argv;
    sio_hw->gpio_clr = LED_MASK;
    printf("LED GPIO%d = 0\n", LED_PIN);
    printf("sio_hw->gpio_clr = 1u << %d;\n", LED_PIN);
    return 0;
}

static int cmd_toggle(int argc, char **argv) {
    (void)argc; (void)argv;
    sio_hw->gpio_togl = LED_MASK;
    printf("GPIO%d invertido con GPIO_OUT_XOR\n", LED_PIN);
    return 0;
}

static int cmd_read(int argc, char **argv) {
    (void)argc; (void)argv;
    uint32_t entradas = sio_hw->gpio_in;
    bool boton = (entradas & BUTTON_MASK) != 0;

    printf("GPIO_IN = 0x%08lx\n", (unsigned long)entradas);
    printf("GPIO%d = %d\n", BUTTON_PIN, boton ? 1 : 0);
    return 0;
}

static int cmd_sio(int argc, char **argv) {
    (void)argc; (void)argv;

    uint32_t gpio_in  = sio_hw->gpio_in;
    uint32_t gpio_out = sio_hw->gpio_out;
    uint32_t gpio_oe  = sio_hw->gpio_oe;

    printf("\nSIO_BASE = 0xD0000000\n");

    printf("GPIO_IN  = 0x%08lx  ", (unsigned long)gpio_in);
    print_binary32(gpio_in);
    printf("\n");

    printf("GPIO_OUT = 0x%08lx  ", (unsigned long)gpio_out);
    print_binary32(gpio_out);
    printf("\n");

    printf("GPIO_OE  = 0x%08lx  ", (unsigned long)gpio_oe);
    print_binary32(gpio_oe);
    printf("\n");

    return 0;
}

static int cmd_mask(int argc, char **argv) {
    (void)argc; (void)argv;

    printf("1u << %d\n", LED_PIN);
    printf("Hex: 0x%08lx\n", (unsigned long)LED_MASK);
    printf("Bin: ");
    print_binary32(LED_MASK);
    printf("\n");

    return 0;
}

static int cmd_hola(int argc, char **argv) {
    if (argc == 1) {
        printf("Hola Mundo\n");
    }
    else {
        int number = atoi(argv[1]);
        for (int i = 1; i <= number; i++) {
            printf("Hola mundo: %d",  i);
            printf("\n");
        }
    }
    return 0;
}

int time_span = 0;
void timer0_isr(void) {
    // Limpiar la interrupción de ALARM0
    timer_hw->intr = 1u << 0;

    // Imprime un mensaje
    printf("\n[TIMER] Han transcurrido %d segundos\n", time_span);

    // Programar nuevamente ALARM0 para dentro de 5 segundos
    timer_hw->alarm[0] = timer_hw->timerawl + time_span * 1000000u;
}

static int cmd_timer_on(int argc, char **argv) {
    if (argc != 2) {
        printf("Número de argumentos inválido. Uso correcto: timer_on [valor]\n\n");
        return 0;
    }
    // Programar ALARM0 para dentro de N segundos
    time_span = atoi(argv[1]);

    timer_hw->alarm[0] = timer_hw->timerawl + time_span * 1000000u;

    // Habilitar la interrupción de ALARM0
    timer_hw->inte = 1u << 0;

    // Asociar TIMER IRQ 0 con nuestra ISR
    irq_set_exclusive_handler(TIMER0_IRQ_0, timer0_isr);

    // Habilitar la interrupción en el NVIC
    irq_set_enabled(TIMER0_IRQ_0, true);

    return 0;
}

static int cmd_timer_off(int argc, char **argv) {
    // Deshabilitar la interrupción de ALARM0
    timer_hw->inte = 0;

    // Deshabilitar la IRQ en el NVIC
    irq_set_enabled(TIMER1_IRQ_0, false);

    return 0;
}

static void init_button() {
    gpio_init(BUTTON_A);
    gpio_set_dir(BUTTON_A, GPIO_IN);
    gpio_pull_up(BUTTON_A);
    gpio_init(BUTTON_B);
    gpio_set_dir(BUTTON_B, GPIO_IN);
    gpio_pull_up(BUTTON_B);
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
}
static void init_i2c() {
    i2c_init(i2c0, 400000); // 400 kHz es el estándar seguro
    gpio_set_function(SDA, GPIO_FUNC_I2C);
    gpio_pull_up(SDA);
    gpio_set_function(SCL, GPIO_FUNC_I2C);
    gpio_pull_up(SCL);
}

// Secuencia obligatoria de encendido e inicialización
static void oled_init_hardware() {
    const uint8_t init_cmds[] = {
        0x00, 0xAE, // Apagar pantalla
        0x00, 0xD5, 0x00, 0x80, // Reloj
        0x00, 0xA8, 0x3F, // Multiplex
        0x00, 0xD3, 0x00, // Offset
        0x00, 0x40, // Línea de inicio
        0x00, 0x8D, 0x14, // Charge Pump (¡Vital para que encienda!)
        0x00, 0x20, 0x00, // Modo horizontal
        0x00, 0xA1, // Mapeo de segmentos
        0x00, 0xC8, // Escaneo COM
        0x00, 0xDA, 0x12, // Pines COM
        0x00, 0x81, 0xCF, // Contraste
        0x00, 0xD9, 0xF1, // Pre-carga
        0x00, 0xDB, 0x40, // VCOMH
        0x00, 0xA4, // Pantalla desde RAM
        0x00, 0xA6, // Normal (no invertida)
        0x00, 0xAF  // ¡Encender pantalla!
    };
    i2c_write_blocking(i2c0, OLED_ADDR, init_cmds, sizeof(init_cmds), false);
}

static void hardware_init(void) {
    gpio_set_function(LED_PIN, GPIO_FUNC_SIO);
    sio_hw->gpio_clr = LED_MASK;
    sio_hw->gpio_oe_set = LED_MASK;

    gpio_set_function(BUTTON_PIN, GPIO_FUNC_SIO);
    sio_hw->gpio_oe_clr = BUTTON_MASK;
    gpio_pull_down(BUTTON_PIN);

    init_i2c();
    oled_init_hardware();
    init_button();

}

static bool beeper;
static bool repeating;

static int points;
static int player_pag;
static int enemigos[12];
const int enemy_offset = 22;

const static uint8_t sprite_player[] = { // Nave o carrito
    0b10111101,
    0b11111111,
    0b10111101,
    0b00111100,
    0b00111100,
    0b10111101,
    0b11111111,
    0b10011001
};
const static uint8_t sprite_enemy[8] = { // Asteroide o bache, o lo que sea.
    0b10001001,
    0b11011011,
    0b01111110,
    0b01111111,
    0b11111110,
    0b01111110,
    0b11011011,
    0b10010001
};
static bool oled_draw(struct repeating_timer *t) {
    if (points <= 0) return false;
    // Score Board o cabecera
    for (uint8_t pagina = 0; pagina < 2; pagina++) {
        static uint8_t cabecera[129]; // Contenido de dibujo
        cabecera[0] = 0x40; // Indicador de envío de datos, siempre como primer dato a enviar.
        cabecera[1] = 0xFF;
        cabecera[128] = 0xFF;
        uint8_t ventana[] = { // Define las coordenadas en la ventana
            0x00, 0x21, 0, 127, // Dimensiones de pantalla en X
            0x00, 0x22, pagina, pagina // Dimensiones de pantalla en Y por páginas
        };
        i2c_write_blocking(i2c0, OLED_ADDR, ventana, sizeof(ventana), false);

        uint8_t contorno = (pagina == 0) ? 0x01 : 0x80;
        uint8_t barra = (pagina == 0) ? 0xF0 : 0x0F;
        for (int i = 2; i <= 127; i++) {
            cabecera[i] = contorno | barra * (i >= 5 && i <= points + 4);
        }
        i2c_write_blocking(i2c0, OLED_ADDR, cabecera, 129, false); // Envío del bitstream
    }
    // Gamescreen o contenido
    for (uint x = 0; x < 12; x+=2) {
        enemigos[x] --;
        if (enemigos[x] == -8) {
            enemigos[x] = 128;
            enemigos[x+1] = (rand() % 6) + 2;
        }
    }
    for (uint8_t pagina = 2; pagina < 8; pagina++) { // Recorre las 8 páginas
        uint8_t contenido[129] = {0}; // Contenido de dibujo
        contenido[0] = 0x40; // Indicador de envío de datos, siempre como primer dato a enviar.
        const uint8_t ventana[] = { // Define las coordenadas en la ventana
            0x00, 0x21, 0, 127, // Dimensiones de pantalla en X
            0x00, 0x22, pagina, pagina // Dimensiones de pantalla en Y por páginas
        };
        i2c_write_blocking(i2c0, OLED_ADDR, ventana, sizeof(ventana), false);
        // Game Draw
        if (pagina == player_pag) {
            for (int i = 0; i < 8; i++) {
                contenido[i+5] = sprite_player[i];
            }
        }
        for (uint x = 0; x < 12; x+=2) {
            if (enemigos[x+1] == pagina) {
                for (int i = 0; i < 8; i++) {
                    int drawpos = enemigos[x] + i;
                    if (drawpos < 129 && drawpos > 0) contenido[drawpos] = sprite_enemy[i]; // añadir sprite
                }
                if (pagina == player_pag && enemigos[x]+3 >= 2 && enemigos[x]+3 <= 16) {
                    points -= 24;
                    player_pag += 1;
                    if (player_pag == 8) player_pag = 6;
                }
            }
        }
        i2c_write_blocking(i2c0, OLED_ADDR, contenido, 129, false); // Envío del bitstream
    }
    if (gpio_get(BUTTON_A) && player_pag < 7) player_pag++;
    if (gpio_get(BUTTON_B) && player_pag > 2) player_pag--;
    return true;
}
static uint8_t title_label[] = {
    0b11001110, 0b11011111, 0b11011011, 0b11011011, 0b11011011, 0b11011011, 0b11111011, 0b01110011, // S
    0x00,
    0b11111000, 0b11111100, 0b00110110, 0b00110011, 0b00110011, 0b00110110, 0b11111100, 0b11111000, // A
    0x00,
    0b00111100, 0b01111110, 0b11000011, 0b11000011, 0b11000011, 0b11000011, 0b11000011, 0b01100110, // C
    0x00,
    0b11111111, 0b11111111, 0b00111100, 0b00111100, 0b01100110, 0b01100110, 0b11000011, 0b11000011, // K
    0x00
};
static bool title(struct repeating_timer *t) {
    if (!repeating) return false;
    static uint8_t stream[129] = {0}; // Contenido de dibujo
    stream[0] = 0x40; // Indicador de envío de datos, siempre como primer dato a enviar.
    for (uint8_t pagina = 0; pagina < 8; pagina++) { // Recorre las 8 páginas
        uint8_t ventana[] = { // Define las coordenadas en la ventana
            0x00, 0x21, 0, 127, // Dimensiones de pantalla en X
            0x00, 0x22, pagina, pagina // Dimensiones de pantalla en Y por páginas
        };
        i2c_write_blocking(i2c0, OLED_ADDR, ventana, sizeof(ventana), false);
        if (pagina == 4) {
            for (int i = 2; i <= 127; i++) {
                if (i >= 46 && i < 46 + 36) {
                    stream[i] = (0xFF * beeper ^ title_label[i-46]);
                }
            }
        } else {
            for (int i = 2; i <= 127; i++) {
                stream[i] = 0xFF * beeper;
            }
        }
        i2c_write_blocking(i2c0, OLED_ADDR, stream, 129, false); // Envío del bitstream
    }
    beeper = !beeper;
    return true;
}
static void init_game() {
    beeper = false;
    repeating = true;
    srand(to_us_since_boot(get_absolute_time())); // Seed

    points = 120;
    player_pag = 4;
    enemigos[0] = 128 + enemy_offset*0;
    enemigos[1] = 2; // Enemigo 1
    enemigos[2] = 128 + enemy_offset*1;
    enemigos[3] = 7; // Enemigo 2
    enemigos[4] = 129 + enemy_offset*2;
    enemigos[5] = 2; // Enemigo 3
    enemigos[6] = 129 + enemy_offset*3;
    enemigos[7] = 7; // Enemigo 4
    enemigos[8] = 130 + enemy_offset*4;
    enemigos[9] = 2; // Enemigo 5
    enemigos[10] = 130 + enemy_offset*5;
    enemigos[11] = 7; // Enemigo 6
}

static int sack(int argc, char **argv) {
    stdio_init_all();
    hardware_init() ;
    init_game();
    struct repeating_timer timer;
    title(NULL);
    add_repeating_timer_ms(1000, title, NULL, &timer);
    while (!gpio_get(BUTTON_A) && !gpio_get(BUTTON_B)) {}
    repeating = false;
    struct repeating_timer timer2;
    add_repeating_timer_ms(25, oled_draw, NULL, &timer2);
    while (points > 0) {
        tight_loop_contents();
    }
    return 0;
}

int main(void) {
    stdio_init_all();
    hardware_init() ;

    printf("\n=====================================\n");
    printf(" Monitor didáctico RP2040 - SIO\n");
    printf("=====================================\n");
    printf("Escribe 'help' para comenzar.\n\n");

    char line[LINE_BUFFER_SIZE];
    char *argv[MAX_ARGS];

    while (true) {
        printf("\n RP2040> ");
        read_line(line, sizeof(line));
        int argc = split_line(line, argv, MAX_ARGS);
        execute_command(argc, argv);
    }
}