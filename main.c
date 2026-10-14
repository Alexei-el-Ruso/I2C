#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"

#define SDA 16 // GP16
#define SCL 17 // GP17
#define BUTTON_A 18 // GP18
#define BUTTON_B 19 // GP18
#define OLED_ADDR 0x3C // Mueve el cursor

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

static void init_i2c() {
    i2c_init(i2c0, 400000); // 400 kHz es el estándar seguro
    gpio_set_function(SDA, GPIO_FUNC_I2C);
    gpio_pull_up(SDA);
    gpio_set_function(SCL, GPIO_FUNC_I2C);
    gpio_pull_up(SCL);
}

static bool beeper;
static bool repeating;

static int points;
static int player_pag;
static int enemigos[12];
const int enemy_offset = 22;
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
                if (pagina == player_pag && enemigos[x]+3 >= 2 && enemigos[x]+3 <= 17) {
                    points -= 24;
                    if (points <= 0) return false;
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

int main() {
    init_i2c();
    oled_init_hardware();
    init_button();
    while (true) {
        init_game();
        struct repeating_timer timer;
        title(NULL);
        add_repeating_timer_ms(1000, title, NULL, &timer);
        while (!gpio_get(BUTTON_A) && !gpio_get(BUTTON_B)) {}
        repeating = false;
        struct repeating_timer timer2;
        add_repeating_timer_ms(100, oled_draw, NULL, &timer2);
        while (points > 0) {
            tight_loop_contents();
        }
        oled_draw(NULL);
        sleep_ms(5000);
    }
    return 0;
}