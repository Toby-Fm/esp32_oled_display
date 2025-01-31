#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include <string.h>
#include "ssd1306.h"

// ----------------------------------------------------------------------------
// I2C- und OLED-Definitionen
// ----------------------------------------------------------------------------
#define I2C_MASTER_SCL_IO 22
#define I2C_MASTER_SDA_IO 21
#define I2C_MASTER_NUM I2C_NUM_0
#define OLED_ADDR 0x3C

// ----------------------------------------------------------------------------
// Strukturen für LEDs und Buttons
// ----------------------------------------------------------------------------
typedef struct {
    gpio_num_t pin;
    bool status;
} LED;

typedef struct {
    gpio_num_t pin;
    bool lastState;
} Button;

// ----------------------------------------------------------------------------
// OLED Display Struktur
// ----------------------------------------------------------------------------
typedef struct {
    SSD1306_t dev;
    char last_green_status[16];
    char last_yellow_status[16];
    char last_red_status[16];
} OLED_Display;

// ----------------------------------------------------------------------------
// LED Funktionen
// ----------------------------------------------------------------------------
void led_init(LED* led, gpio_num_t pin) {
    led->pin = pin;
    led->status = false;
    gpio_config_t led_conf = {};
    led_conf.pin_bit_mask = (1ULL << pin);
    led_conf.mode = GPIO_MODE_OUTPUT;
    led_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    led_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    gpio_config(&led_conf);
}

void led_on(LED* led) {
    led->status = true;
    gpio_set_level(led->pin, 1);
}

void led_off(LED* led) {
    led->status = false;
    gpio_set_level(led->pin, 0);
}

void led_toggle(LED* led) {
    led->status = !led->status;
    gpio_set_level(led->pin, led->status ? 1 : 0);
}

bool led_is_on(LED* led) {
    return led->status;
}

// ----------------------------------------------------------------------------
// Button Funktionen (mit Debouncing)
// ----------------------------------------------------------------------------
void button_init(Button* button, gpio_num_t pin) {
    button->pin = pin;
    button->lastState = false;
    gpio_config_t button_conf = {};
    button_conf.pin_bit_mask = (1ULL << pin);
    button_conf.mode = GPIO_MODE_INPUT;
    button_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    button_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    gpio_config(&button_conf);
}

bool button_is_pressed(Button* button) {
    bool currentState = (gpio_get_level(button->pin) == 0);
    if (currentState && !button->lastState) {
        vTaskDelay(pdMS_TO_TICKS(50));  // Debounce
        button->lastState = true;
        return true;
    } else if (!currentState) {
        button->lastState = false;
    }
    return false;
}

// ----------------------------------------------------------------------------
// OLED Funktionen
// ----------------------------------------------------------------------------
void oled_init(OLED_Display* display) {
    i2c_master_init(&display->dev, I2C_MASTER_SDA_IO, I2C_MASTER_SCL_IO, -1);
    ssd1306_init(&display->dev, 128, 64);
    ssd1306_clear_screen(&display->dev, false);
    ssd1306_contrast(&display->dev, 0xFF);
    memset(display->last_green_status, 0, sizeof(display->last_green_status));
    memset(display->last_yellow_status, 0, sizeof(display->last_yellow_status));
    memset(display->last_red_status, 0, sizeof(display->last_red_status));
}

void oled_update(OLED_Display* display, const char* green_status, const char* yellow_status, const char* red_status) {
    if (strcmp(green_status, display->last_green_status) != 0 ||
        strcmp(yellow_status, display->last_yellow_status) != 0 ||
        strcmp(red_status, display->last_red_status) != 0) {

        strncpy(display->last_green_status, green_status, sizeof(display->last_green_status) - 1);
        strncpy(display->last_yellow_status, yellow_status, sizeof(display->last_yellow_status) - 1);
        strncpy(display->last_red_status, red_status, sizeof(display->last_red_status) - 1);

        display->last_green_status[sizeof(display->last_green_status) - 1] = '\0';
        display->last_yellow_status[sizeof(display->last_yellow_status) - 1] = '\0';
        display->last_red_status[sizeof(display->last_red_status) - 1] = '\0';

        char green_text[16], yellow_text[16], red_text[16];
        strncpy(green_text, green_status, sizeof(green_text) - 1);
        strncpy(yellow_text, yellow_status, sizeof(yellow_text) - 1);
        strncpy(red_text, red_status, sizeof(red_text) - 1);

        green_text[sizeof(green_text) - 1] = '\0';
        yellow_text[sizeof(yellow_text) - 1] = '\0';
        red_text[sizeof(red_text) - 1] = '\0';

        ssd1306_display_text(&display->dev, 0, green_text, strlen(green_text), false);
        ssd1306_display_text(&display->dev, 1, yellow_text, strlen(yellow_text), false);
        ssd1306_display_text(&display->dev, 2, red_text, strlen(red_text), false);
    }
}

// ----------------------------------------------------------------------------
// Hauptprogramm (app_main)
// ----------------------------------------------------------------------------
void app_main() {
    LED greenLED, yellowLED, redLED;
    Button greenButton, yellowButton, redButton, onOffButton;
    OLED_Display display;

    led_init(&greenLED, GPIO_NUM_2);
    led_init(&yellowLED, GPIO_NUM_19);
    led_init(&redLED, GPIO_NUM_18);

    button_init(&greenButton, GPIO_NUM_5);
    button_init(&yellowButton, GPIO_NUM_17);
    button_init(&redButton, GPIO_NUM_16);
    button_init(&onOffButton, GPIO_NUM_4);

    oled_init(&display);

    bool leds_on = true;

    while (1) {
        if (button_is_pressed(&greenButton)) {
            led_toggle(&greenLED);
        }

        if (button_is_pressed(&yellowButton)) {
            led_toggle(&yellowLED);
        }

        if (button_is_pressed(&redButton)) {
            led_toggle(&redLED);
        }

        if (button_is_pressed(&onOffButton)) {
            leds_on = !leds_on;
            if (!leds_on) {
                led_off(&greenLED);
                led_off(&yellowLED);
                led_off(&redLED);
            }
        }

        gpio_set_level(GPIO_NUM_2, (leds_on && led_is_on(&greenLED)) ? 1 : 0);
        gpio_set_level(GPIO_NUM_19, (leds_on && led_is_on(&yellowLED)) ? 1 : 0);
        gpio_set_level(GPIO_NUM_18, (leds_on && led_is_on(&redLED)) ? 1 : 0);

        const char* green_status = (leds_on && led_is_on(&greenLED)) ? "Green LED: ON" : "Green LED: OFF";
        const char* yellow_status = (leds_on && led_is_on(&yellowLED)) ? "Yellow LED: ON" : "Yellow LED: OFF";
        const char* red_status = (leds_on && led_is_on(&redLED)) ? "Red LED: ON" : "Red LED: OFF";

        oled_update(&display, green_status, yellow_status, red_status);

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}
