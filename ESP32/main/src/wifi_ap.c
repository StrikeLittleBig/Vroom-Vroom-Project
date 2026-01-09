#include "wifi_ap.h"

#include <string.h>
#include "esp_event.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_netif.h"

#include "led_rgb.h"

/* Fallback si tu utilises tes macros ULOG* ailleurs */
#ifndef ULOGI
  #define ULOGI  ESP_LOGI
  #define ULOGW  ESP_LOGW
  #define ULOGE  ESP_LOGE
#endif

/* =========================== Kconfig bindings ============================ */
#define EXAMPLE_ESP_WIFI_SSID       CONFIG_ESP_WIFI_SSID
#define EXAMPLE_ESP_WIFI_PASS       CONFIG_ESP_WIFI_PASSWORD
#define EXAMPLE_ESP_WIFI_CHANNEL    CONFIG_ESP_WIFI_CHANNEL
#define EXAMPLE_MAX_STA_CONN        CONFIG_ESP_MAX_STA_CONN
#if CONFIG_ESP_GTK_REKEYING_ENABLE
  #define EXAMPLE_GTK_REKEY_INTERVAL  CONFIG_ESP_GTK_REKEY_INTERVAL
#else
  #define EXAMPLE_GTK_REKEY_INTERVAL  0
#endif

/* ============================ état du module ============================= */
static const char *TAG = "wifi_ap";
static volatile int s_sta_count = 0;

/* ============================ event handler ============================== */
static void wifi_event_handler(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    if (base != WIFI_EVENT) return;

    switch (id) {
    case WIFI_EVENT_AP_START:
        led_set_mode(LED_AP_UP);
        ULOGI(TAG, "AP started (SSID=%s)", EXAMPLE_ESP_WIFI_SSID);
        break;

    case WIFI_EVENT_AP_STOP:
        led_set_mode(LED_WIFI_DOWN);
        ULOGW(TAG, "AP stopped");
        break;

    case WIFI_EVENT_AP_STACONNECTED: {
        s_sta_count++;
        led_set_mode(LED_STA_CONNECTED);
        const wifi_event_ap_staconnected_t *e = (const wifi_event_ap_staconnected_t *)data;
        ULOGI(TAG, "Station " MACSTR " join, AID=%d", MAC2STR(e->mac), e->aid);
        break;
    }

    case WIFI_EVENT_AP_STADISCONNECTED: {
        if (s_sta_count > 0) s_sta_count--;
        if (s_sta_count == 0) led_set_mode(LED_AP_UP);
        const wifi_event_ap_stadisconnected_t *e = (const wifi_event_ap_stadisconnected_t *)data;
        ULOGW(TAG, "Station " MACSTR " leave, AID=%d, reason=%d", MAC2STR(e->mac), e->aid, e->reason);
        break;
    }

    default:
        break;
    }
}

/* ================================ API ==================================== */
void wifi_ap_start(void)
{
    /* Réseau de base */
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    (void)esp_netif_create_default_wifi_ap(); /* handle ignoré volontairement */

    /* Wi-Fi driver */
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    /* Événements */
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL));

    /* Paramètres SoftAP depuis sdkconfig */
    wifi_config_t wifi_config = {
        .ap = {
            .ssid           = EXAMPLE_ESP_WIFI_SSID,
            .ssid_len       = strlen(EXAMPLE_ESP_WIFI_SSID),
            .channel        = EXAMPLE_ESP_WIFI_CHANNEL,
            .password       = EXAMPLE_ESP_WIFI_PASS,
            .max_connection = EXAMPLE_MAX_STA_CONN,
#ifdef CONFIG_ESP_WIFI_SOFTAP_SAE_SUPPORT
            .authmode       = WIFI_AUTH_WPA3_PSK,
            .sae_pwe_h2e    = WPA3_SAE_PWE_BOTH,
#else
            .authmode       = WIFI_AUTH_WPA2_PSK,
#endif
            .pmf_cfg        = { .required = true },
            .gtk_rekey_interval = EXAMPLE_GTK_REKEY_INTERVAL,
        },
    };
    if (strlen(EXAMPLE_ESP_WIFI_PASS) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    /* Démarrage SoftAP */
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ULOGI(TAG, "SoftAP up. SSID:%s PASS:%s CH:%d",
          EXAMPLE_ESP_WIFI_SSID,
          (strlen(EXAMPLE_ESP_WIFI_PASS) ? EXAMPLE_ESP_WIFI_PASS : "<open>"),
          EXAMPLE_ESP_WIFI_CHANNEL);
}

int wifi_ap_get_sta_count(void)
{
    return s_sta_count;
}
