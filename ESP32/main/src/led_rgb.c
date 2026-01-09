#include "led_rgb.h"

#include "led_strip.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

/* Fallback si tu utilises tes macros ULOG* ailleurs */
#ifndef ULOGI
  #define ULOGI  ESP_LOGI
  #define ULOGW  ESP_LOGW
  #define ULOGE  ESP_LOGE
#endif

/* ============================ état du module ============================= */
static const char *TAG = "led";

static led_strip_handle_t s_led = NULL;
static volatile led_mode_t s_mode = LED_WIFI_DOWN;

/* Durées de blink par défaut (ms) */
#define LED_ON_MS   500
#define LED_OFF_MS  200

/* ============================== helpers ================================== */
static inline void set_rgb(uint8_t r, uint8_t g, uint8_t b)
{
    if (!s_led) return;
    if (led_strip_set_pixel(s_led, 0, r, g, b) == ESP_OK) {
        (void)led_strip_refresh(s_led);
    }
}

static inline void all_off(void)
{
    if (!s_led) return;
    (void)led_strip_clear(s_led);
    (void)led_strip_refresh(s_led);
}

/* ================================ tâche ================================== */
static void led_task(void *arg)
{
    const TickType_t on_ticks  = pdMS_TO_TICKS(LED_ON_MS);
    const TickType_t off_ticks = pdMS_TO_TICKS(LED_OFF_MS);

    for (;;) {
        uint8_t r = 0, g = 0, b = 0;

        switch (s_mode) {
            case LED_WIFI_DOWN:     r = 255; g = 180; b = 0;   break; 
            case LED_AP_UP:         r = 0;   g = 255; b = 255;   break; 
            case LED_STA_CONNECTED: r = 0;   g = 255; b = 0;   break;
            default: break;
        }

        set_rgb(r, g, b); vTaskDelay(on_ticks);
        all_off();        vTaskDelay(off_ticks);
    }
}

/* ================================ API ==================================== */
void led_set_mode(led_mode_t m)
{
    s_mode = m; /* suffisant ici : lecture/écriture atomiques sur enum */
}

void led_init(int gpio)
{

    led_strip_config_t strip_cfg = {
        .strip_gpio_num = gpio,
        .max_leds       = 1, 
    };
    led_strip_rmt_config_t rmt_cfg = {
        .clk_src           = RMT_CLK_SRC_DEFAULT,
        .resolution_hz     = 10 * 1000 * 1000, /* 10 MHz */
        .mem_block_symbols = 0,
        .flags = { .with_dma = false }
    };

    if (led_strip_new_rmt_device(&strip_cfg, &rmt_cfg, &s_led) != ESP_OK || !s_led) {
        ULOGE(TAG, "led_strip_new_rmt_device() failed (gpio=%d)", gpio);
        return;
    }

    all_off();

    if (xTaskCreate(led_task, "led_task", 2048, NULL, 3, NULL) != pdPASS) {
        ULOGE(TAG, "xTaskCreate(led_task) failed");
        return;
    }

    ULOGI(TAG, "LED ready on GPIO%d", gpio);
}

