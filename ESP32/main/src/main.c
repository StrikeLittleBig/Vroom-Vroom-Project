#include "main.h"

#include "sdkconfig.h"
#include "esp_err.h"
#include "nvs_flash.h"
#include "esp_mac.h"

#include "user_log_setup.h"
#include "led_rgb.h"
#include "wifi_ap.h"
#include "spi_link.h"
#include "udp_server.h"
#include "gp_proto.h" 

#if CONFIG_USER_UART_ENABLE
#include "user_uart.h"
#endif

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include <string.h>

#include "esp_log.h"
#ifndef ULOGI
  #define ULOGI  ESP_LOGI
  #define ULOGW  ESP_LOGW
  #define ULOGE  ESP_LOGE
#endif


void app_main(void)
{
#if CONFIG_USER_UART_ENABLE
    user_uart_init();
#endif
    user_log_setup();
    ULOGI("boot", "Boot start");

    /* ---- NVS ---- */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    ULOGI("boot", "NVS ready");

    /* ---- LED statut ---- */
    led_init(APP_LED_COUNT);
    ULOGI("led", "LED task started");

    /* ---- Wi-Fi SoftAP ---- */
    wifi_ap_start();
    ULOGI("wifi_ap", "SoftAP init requested");

    /* Log MAC SoftAP pour debug réseau (facultatif) */
    uint8_t mac[6] = {0};
    if (esp_read_mac(mac, ESP_MAC_WIFI_SOFTAP) == ESP_OK) {
        ULOGI("wifi_ap", "SoftAP MAC %02X:%02X:%02X:%02X:%02X:%02X",
              mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    }

    /* ---- SPI link (esclave) ---- */
    ESP_ERROR_CHECK(spi_link_init(APP_SPI_FRAME_SIZE, PIN_MOSI, PIN_MISO,
                                  PIN_SCLK, PIN_CS));
    ULOGI("spi", "SPI link ready (frame=%u)", (unsigned)APP_SPI_FRAME_SIZE);

    /* ---- File de messages UDP ---- */
    QueueHandle_t q = xQueueCreate(APP_QUEUE_LEN, sizeof(gp_packet_t));
    configASSERT(q != NULL);
    ULOGI("queue", "Queue created (%u elts)", (unsigned)APP_QUEUE_LEN);

    /* ---- Serveur UDP ---- */
    udp_server_start(q); 
    ULOGI("udp", "UDP server up");

}
