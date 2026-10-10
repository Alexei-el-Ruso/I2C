#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"

#define SDA 16 // GP16
#define SCL 17 // GP17
#define BUTTON 18 // GP18
#define OLED_ADDR 0x3C // Mueve el cursor

static void init_button() {
    gpio_init(BUTTON);
    gpio_set_dir(BUTTON, GPIO_IN);
    gpio_pull_up(BUTTON);
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
}

// Secuencia obligatoria de encendido e inicialización
static void oled_init_hardware() {
    uint8_t init_cmds[] = {
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

static uint8_t stream[129]; // Contenido de dibujo
static void init_i2c() {
    i2c_init(i2c0, 400000); // 400 kHz es el estándar seguro
    gpio_set_function(SDA, GPIO_FUNC_I2C);
    gpio_pull_up(SDA);
    gpio_set_function(SCL, GPIO_FUNC_I2C);
    gpio_pull_up(SCL);

    stream[0] = 0x40; // Indicador de envío de datos, siempre como primer dato a enviar.
}

void oled_draw() {
    for (uint8_t pagina = 0; pagina < 8; pagina++) { // Recorre las 8 páginas
        uint8_t ventana[] = { // Define las coordenadas en la ventana
            0x00, 0x21, 0, 127, // Dimensiones de pantalla en X
            0x00, 0x22, pagina, pagina // Dimensiones de pantalla en Y
        };
        i2c_write_blocking(i2c0, OLED_ADDR, ventana, sizeof(ventana), false);

        for (int i = 1; i <= 128; i++) {
            // Agrupamos las columnas de 8 en 8 para formar cuadros visibles
            int bloque_columna = (i - 1) / 8;

            if ((pagina + bloque_columna) % 2 == 0) {
                stream[i] = 0xFF * gpio_get(BUTTON); // Cuadro completamente lleno (8 píxeles verticales encendidos)
            } else {
                stream[i] = 0xFF * !gpio_get(BUTTON); // Cuadro completamente vacío
            }
        }

        i2c_write_blocking(i2c0, OLED_ADDR, stream, 129, false); // Envío del bitstream
    }
}

int main() {
    init_i2c();
    oled_init_hardware();
    init_button();

    while (true) {
        oled_draw();
        sleep_ms(200);
    }
    return 0;
}