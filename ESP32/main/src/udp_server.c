#include "udp_server.h"

#include "gp_proto.h"
#include "user_log_setup.h"
#include "spi_link.h"
#include "esp_log.h" 

#include "lwip/sockets.h"
#include "lwip/inet.h"
#include "lwip/netdb.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <string.h>
#include <inttypes.h>
  
#ifndef ULOGI
  #define ULOGI  ESP_LOGI
  #define ULOGW  ESP_LOGW
  #define ULOGE  ESP_LOGE
#endif


/* ============================ état du module ============================= */
static const char *TAG = "udp";
static QueueHandle_t s_rx_q = NULL;       // queue interne (remplace l’ex- s_q)
static volatile uint32_t s_drop_cnt = 0;  // stats: paquets dropés

/* =========================== protos internes ============================= */
static void udp_task(void *arg);
static void bridge_udp_to_spi_task(void *arg);

static inline void bin32(uint32_t v, char out[33]) {
    for (int i = 31; i >= 0; --i) out[31 - i] = (v & (1u << i)) ? '1' : '0';
    out[32] = '\0';
}

static inline void log_buffer_bin(const char *tag, const uint8_t *buf, int len) {
    for (int i = 0; i < len; i += 8) {
        char line[8*(8+1)+1]; // 8 octets * (8 bits + espace) + '\0'
        int k = 0, n = (len - i > 8) ? 8 : (len - i);
        for (int j = 0; j < n; ++j) {
            uint8_t b = buf[i + j];
            for (int bit = 7; bit >= 0; --bit) line[k++] = (b & (1u << bit)) ? '1' : '0';
            line[k++] = ' ';
        }
        line[k ? k-1 : 0] = '\0';
        ULOGI(tag, "%u: %s", i, line);
    }
}

static void gp_log_packet(const gp_packet_t *p)
{
    char b_btn[33];
    bin32(p->buttons, b_btn);

    ULOGI("pad", "t=%" PRIu32 " btn=%s lx=%.3f ly=%.3f rx=%.3f ry=%.3f lt=%.3f rt=%.3f",
          (uint32_t)esp_log_timestamp(),
          b_btn,
          p->lx, p->ly,
          p->rx, p->ry,
          p->lt, p->rt);
}

/* ASCII "0101…" → gp_packet_t (boutons). Retourne false si non conforme. */
static bool parse_ascii_01_to_packet(const uint8_t *buf, int len, gp_packet_t *out) {
    int bits = 0; uint32_t acc = 0;
    for (int i = 0; i < len; ++i) {
        char c = (char)buf[i];
        if (c == '\r' || c == '\n' || c == ' ') continue;
        if (c != '0' && c != '1') return false;
        if (bits < 32) { acc = (acc << 1) | (uint32_t)(c == '1'); bits++; }
    }
    if (bits == 0) return false;
    if (bits < 32) acc <<= (32 - bits); // left-pad si moins de 32 bits
    memset(out, 0, sizeof(*out));
    out->buttons = acc;
    return true;
}

/* =============================== tâches ================================== */

static void udp_task(void *arg)
{  
    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (sock < 0) {
        ULOGE(TAG, "socket() failed");
        vTaskDelete(NULL);
        return;
    }

    int yes = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port   = htons(GP_UDP_PORT),
        .sin_addr.s_addr = htonl(INADDR_ANY)
    };

    if (bind(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        ULOGE(TAG, "bind(*:%d) failed", GP_UDP_PORT);
        close(sock);
        vTaskDelete(NULL);
        return;
    }

    ULOGI(TAG, "listening on *:%d", GP_UDP_PORT);

    int dump_left = 3;
    for (;;) {
        uint8_t buf[128];
        struct sockaddr_in src; socklen_t sl = sizeof(src);
        int len = recvfrom(sock, buf, sizeof(buf), 0, (struct sockaddr*)&src, &sl);
        if (len <= 0) continue;

        if (dump_left-- > 0) {
            ULOGI("udp_in", "pkt %dB from %s:%d", len, inet_ntoa(src.sin_addr), ntohs(src.sin_port));
            log_buffer_bin("udp_in", buf, len);
        }

        if (len == (int)sizeof(gp_packet_t)) {
            gp_packet_t p;

            ULOGI("udp_in", "Raw UDP buffer:");
            log_buffer_bin("udp_in", buf, len);

            memcpy(&p, buf, sizeof(p));
            if (xQueueSend(s_rx_q, &p, 0) != pdPASS) {
                gp_packet_t throwaway; (void)xQueueReceive(s_rx_q, &throwaway, 0);
                if (xQueueSend(s_rx_q, &p, 0) != pdPASS) s_drop_cnt++;
            }
            gp_log_packet(&p);
        } else {
            // Optionnel : tu peux supprimer le parse_ascii_01_to_packet si tu ne veux plus supporter ce format
            ULOGW(TAG, "Unexpected size %d (from %s:%d) - ignoring",
                  len, inet_ntoa(src.sin_addr), ntohs(src.sin_port));
        }
    }
}

static void bridge_udp_to_spi_task(void *arg)
{
    for (;;) {
        gp_packet_t p;
        if (xQueueReceive(s_rx_q, &p, portMAX_DELAY) != pdTRUE) continue;

        esp_err_t err = spi_link_send(&p, sizeof(p), pdMS_TO_TICKS(5));
        if (err == ESP_OK) continue;

        if (err == ESP_ERR_INVALID_STATE) {
            /* Bus occupé / maître absent → petit délai + poll */
            vTaskDelay(pdMS_TO_TICKS(5));
            spi_link_poll();
        } else {
            vTaskDelay(pdMS_TO_TICKS(5));
        }
    }
}

/* ============================== API publique ============================= */

esp_err_t udp_server_start(QueueHandle_t rx_queue)
{
    if (rx_queue == NULL) return ESP_ERR_INVALID_ARG;
    s_rx_q = rx_queue;

    BaseType_t ok1 = xTaskCreate(udp_task, "udp_server", 4096, NULL, 5, NULL);
    BaseType_t ok2 = xTaskCreate(bridge_udp_to_spi_task, "bridge_udp_spi", 4096, NULL, 6, NULL);

    if (ok1 != pdPASS || ok2 != pdPASS) {
        ULOGE(TAG, "task creation failed");
        return ESP_ERR_NO_MEM;
    }
    return ESP_OK;
}

QueueHandle_t udp_server_get_queue(void) { return s_rx_q; }

UBaseType_t udp_buffer_count(void)
{
    return (s_rx_q) ? uxQueueMessagesWaiting(s_rx_q) : 0;
}

uint32_t udp_drop_count(void) { return s_drop_cnt; }
