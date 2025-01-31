#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include <cstring>
#include "ssd1306.h"

// ----------------------------------------------------------------------------
// Pin-Definitionen für LEDs
// ----------------------------------------------------------------------------
#define GREEN_LED   GPIO_NUM_2
#define YELLOW_LED  GPIO_NUM_19
#define RED_LED     GPIO_NUM_18

// ----------------------------------------------------------------------------
// Pin-Definitionen für Buttons
// ----------------------------------------------------------------------------
#define BUTTON_GREEN    GPIO_NUM_5
#define BUTTON_YELLOW   GPIO_NUM_17
#define BUTTON_RED      GPIO_NUM_16
#define BUTTON_ON_OUT   GPIO_NUM_4  // Schaltet alle LEDs ein/aus

// ----------------------------------------------------------------------------
// I2C- und OLED-Definitionen
// ----------------------------------------------------------------------------
#define I2C_MASTER_SCL_IO 22  // GPIO für SCL
#define I2C_MASTER_SDA_IO 21  // GPIO für SDA
#define I2C_MASTER_NUM I2C_NUM_0
#define OLED_ADDR 0x3C        // I2C-Adresse des OLED-Displays

SSD1306_t dev;  // OLED-Display-Handle

// Speichert den letzten Zustand des OLED-Displays
char last_green_status[16] = "";
char last_yellow_status[16] = "";
char last_red_status[16] = "";

// ----------------------------------------------------------------------------
// GPIO-Konfiguration
// ----------------------------------------------------------------------------
void configure_gpio()
{
    gpio_config_t led_conf = {
        .pin_bit_mask = (1ULL << GREEN_LED) | (1ULL << YELLOW_LED) | (1ULL << RED_LED),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&led_conf);

    gpio_config_t button_conf = {
        .pin_bit_mask = (1ULL << BUTTON_GREEN) | (1ULL << BUTTON_YELLOW) | (1ULL << BUTTON_RED) | (1ULL << BUTTON_ON_OUT),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&button_conf);
}

// ----------------------------------------------------------------------------
// I2C und OLED-Initialisierung
// ----------------------------------------------------------------------------
void init_i2c()
{
    i2c_master_init(&dev, I2C_MASTER_SDA_IO, I2C_MASTER_SCL_IO, -1);
}

void init_oled()
{
    ssd1306_init(&dev, 128, 64);
    ssd1306_clear_screen(&dev, false);
    ssd1306_contrast(&dev, 0xFF);
}

// ----------------------------------------------------------------------------
// OLED aktualisieren, nur wenn nötig
// ----------------------------------------------------------------------------
void update_oled(const char *green_status, const char *yellow_status, const char *red_status)
{
    // Prüfen, ob sich etwas geändert hat
    if (strcmp(green_status, last_green_status) != 0 ||
        strcmp(yellow_status, last_yellow_status) != 0 ||
        strcmp(red_status, last_red_status) != 0)
    {
        // Neue Werte speichern
        strncpy(last_green_status, green_status, sizeof(last_green_status) - 1);
        strncpy(last_yellow_status, yellow_status, sizeof(last_yellow_status) - 1);
        strncpy(last_red_status, red_status, sizeof(last_red_status) - 1);

        last_green_status[sizeof(last_green_status) - 1] = '\0';
        last_yellow_status[sizeof(last_yellow_status) - 1] = '\0';
        last_red_status[sizeof(last_red_status) - 1] = '\0';

        // **Text richtig generieren**
        char green_text[16], yellow_text[16], red_text[16];
        snprintf(green_text, sizeof(green_text), "%s", green_status);
        snprintf(yellow_text, sizeof(yellow_text), "%s", yellow_status);
        snprintf(red_text, sizeof(red_text), "%s", red_status);

        green_text[sizeof(green_text) - 1] = '\0';
        yellow_text[sizeof(yellow_text) - 1] = '\0';
        red_text[sizeof(red_text) - 1] = '\0';

        // OLED aktualisieren
        ssd1306_display_text(&dev, 0, green_text, strlen(green_text), false);
        ssd1306_display_text(&dev, 1, yellow_text, strlen(yellow_text), false);
        ssd1306_display_text(&dev, 2, red_text, strlen(red_text), false);
    }
}

// ----------------------------------------------------------------------------
// Hauptprogramm (app_main)
// ----------------------------------------------------------------------------
extern "C" void app_main()
{
    configure_gpio();
    init_i2c();
    init_oled();

    bool green_led_on  = false;
    bool yellow_led_on = false;
    bool red_led_on    = false;
    bool leds_on       = true;

    while (true)
    {
        if (gpio_get_level(BUTTON_GREEN) == 0) {
            green_led_on = !green_led_on;
            vTaskDelay(pdMS_TO_TICKS(200));
        }

        if (gpio_get_level(BUTTON_YELLOW) == 0) {
            yellow_led_on = !yellow_led_on;
            vTaskDelay(pdMS_TO_TICKS(200));
        }

        if (gpio_get_level(BUTTON_RED) == 0) {
            red_led_on = !red_led_on;
            vTaskDelay(pdMS_TO_TICKS(200));
        }

        if (gpio_get_level(BUTTON_ON_OUT) == 0) {
            leds_on = !leds_on;
            vTaskDelay(pdMS_TO_TICKS(200));
        }

        gpio_set_level(GREEN_LED,  (leds_on && green_led_on)  ? 1 : 0);
        gpio_set_level(YELLOW_LED, (leds_on && yellow_led_on) ? 1 : 0);
        gpio_set_level(RED_LED,    (leds_on && red_led_on)    ? 1 : 0);

        // **Richtige Status-Texte für das Display**
        const char* green_status  = (leds_on && green_led_on)  ? "Green LED: ON"   : "Green LED: OFF";
        const char* yellow_status = (leds_on && yellow_led_on) ? "Yellow LED: ON"  : "Yellow LED: OFF";
        const char* red_status    = (leds_on && red_led_on)    ? "Red LED: ON"     : "Red LED: OFF";

        update_oled(green_status, yellow_status, red_status);

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}
