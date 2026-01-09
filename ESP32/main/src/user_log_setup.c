#include "user_log_setup.h"

#include <stddef.h>
#include "esp_log.h"

/* Fallback si tu utilises des macros ULOG* ailleurs */
#ifndef ULOGI
  #define ULOGI  ESP_LOGI
  #define ULOGW  ESP_LOGW
  #define ULOGE  ESP_LOGE
#endif

/* ============================ état du module ============================= */
static const char *TAG = "log";

/* Petit utilitaire pour lister proprement tag + niveau */
typedef struct {
    const char *tag;
    esp_log_level_t level;
} tag_level_t;

/* ================================ API ==================================== */
void user_log_setup(void)
{
    /* Par défaut : on ne montre que les erreurs de tous les modules */
    esp_log_level_set("*", ESP_LOG_ERROR);

    /* Couper totalement le bruit de certains composants très bavards */
    const tag_level_t mute_list[] = {
        { "wifi",            ESP_LOG_NONE },
        { "esp_netif_lwip",  ESP_LOG_NONE },
        { "spi_flash",       ESP_LOG_NONE },
        { "app_init",        ESP_LOG_NONE },
        { "main_task",       ESP_LOG_NONE },
        /* { "phy_init",     ESP_LOG_NONE }, */
        /* { "heap_init",    ESP_LOG_NONE }, */
        /* { "sleep_gpio",   ESP_LOG_NONE }, */
    };
    for (size_t i = 0; i < sizeof(mute_list)/sizeof(mute_list[0]); ++i) {
        esp_log_level_set(mute_list[i].tag, mute_list[i].level);
    }

    /* Réactiver SEULEMENT nos tags en INFO */
    const tag_level_t info_list[] = {
        { "boot",        ESP_LOG_INFO },
        { "led",         ESP_LOG_INFO },
        { "wifi_ap",     ESP_LOG_INFO },
        { "udp",         ESP_LOG_INFO },
        { "queue",       ESP_LOG_INFO },
        { "spi_link",    ESP_LOG_INFO },
        { "spi_bridge",  ESP_LOG_INFO },
        { "pad",         ESP_LOG_INFO },  /* garde si ce module existe chez toi */
    };
    for (size_t i = 0; i < sizeof(info_list)/sizeof(info_list[0]); ++i) {
        esp_log_level_set(info_list[i].tag, info_list[i].level);
    }

    ULOGI(TAG, "Log setup OK (default=ERROR, %d tags INFO, %d tags muets).",
          (int)(sizeof(info_list)/sizeof(info_list[0])),
          (int)(sizeof(mute_list)/sizeof(mute_list[0])));
}
