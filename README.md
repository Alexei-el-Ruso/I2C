# SACK - Minijuego para Raspberry Pi Pico y Pantalla OLED (I2C)

#### *Alexei Romero y Marco Beltrán*

Un minijuego retro desarrollado en **C puro** diseñado específicamente
para ejecutarse en una **Raspberry Pi Pico**, utilizando una pantalla
OLED con controlador (protocolo I2C) y dos botones físicos de control.

---

## Características Principales

* **Control de Hardware Directo:** Configuración a bajo nivel del bus I2C (a 400 kHz) y pines GPIO mediante el SDK oficial de la Raspberry Pi Pico.
* **Gestión de Pantalla OLED:** Secuencia de inicialización manual a nivel de comandos de hardware (Charge Pump, contraste, multiplexación, mapeo de segmentos y direccionamiento horizontal).
* **Gráficos por Sprites y Páginas:** Renderizado basado en páginas verticales de la pantalla OLED (8 páginas en total) para dibujar la cabecera de puntuación y los elementos dinámicos del juego.
* **Gestión de Entidades y Colisiones:** Sistema dinámico de generación de enemigos mediante vectores de posición, control de colisiones y penalización de puntos.
* **Pantalla de Título Interactiva:** Animación de bienvenida con parpadeo controlado por temporizadores (`repeating_timer`) y espera de pulsación para iniciar.

---

## Conexiones y Hardware Necesario

* **Microcontrolador:** Raspberry Pi Pico (o Pico W).
* **Pantalla OLED:** 128x64 píxeles (Controlador SSD1306 típico) conectada por I2C.
* **Botones:** 2 botones pulsadores para el movimiento del jugador.

### Mapeo de Pines (GPIO)

| Componente | Pin Raspberry Pi Pico | Descripción |
| :--- | :--- | :--- |
| **OLED SDA** | GP16 | Línea de datos I2C |
| **OLED SCL** | GP17 | Línea de reloj I2C |
| **Botón A** | GP18 | Movimiento hacia arriba / Selección |
| **Botón B** | GP19 | Movimiento hacia abajo |
| **LED Interno** | `PICO_DEFAULT_LED_PIN` | Indicador de estado |

---

## Requisitos de Compilación

Para compilar y flashear este código en tu Raspberry Pi Pico, necesitas:

1. El [Raspberry Pi Pico SDK](https://github.com/raspberry-pi/pico-sdk)
instalado y configurado en tu entorno de desarrollo.
2. Un compilador de C para arquitecturas ARM (como `arm-none-eabi-gcc`).
3. CMake para la gestión del proyecto de compilación.

---

## Instrucciones de Uso y Dinámica del Juego

1. **Encendido:** Al arrancar el dispositivo, la pantalla mostrará la
**Pantalla de Título** intermitente con el logotipo del juego ("SACK").
2. **Inicio:** Presiona cualquiera de los dos botones
(**Botón A** o **Botón B**) para saltar al juego principal.
3. **Controles:**
   - **Botón A:** Mueve al jugador hacia arriba (incrementa la página de posición).
   - **Botón B:** Mueve al jugador hacia abajo (decrementa la página de posición).
4. **Objetivo:** Esquiva los obstáculos/enemigos que se desplazan
de derecha a izquierda en la pantalla. Chocar contra ellos reducirá
tus puntos de vida/barra superior hasta agotar la partida (`points <= 0`).
Al terminar, el juego se reiniciará automáticamente tras una breve pausa.