#include "spi_link.h"
#include "driver/spi_master.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include <string.h>

#ifndef ULOGI
  #define ULOGI  ESP_LOGI
  #define ULOGW  ESP_LOGW
  #define ULOGE  ESP_LOGE
#endif

static const char *TAG = "spi_link_m";

static const spi_host_device_t s_host = SPI2_HOST; // ESP32-C3 = SPI2
static spi_device_handle_t s_dev = NULL;

static size_t   s_frame_sz = 0;
static uint8_t *s_txbuf    = NULL;

esp_err_t spi_link_init(size_t frame_size,
                        int mosi, int miso, int sclk, int cs)
{
    s_frame_sz = frame_size;

    spi_bus_config_t buscfg = {
        .mosi_io_num = mosi,
        .miso_io_num = miso,
        .sclk_io_num = sclk,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = (int)frame_size
    };
    ESP_ERROR_CHECK(spi_bus_initialize(s_host, &buscfg, SPI_DMA_CH_AUTO));

    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 1 * 1000 * 1000, // 1 MHz pour commencer
        .mode = 0,                         // CPOL=0, CPHA=0
        .spics_io_num = cs,
        .queue_size = 2,
        .flags = SPI_DEVICE_HALFDUPLEX,
        .command_bits = 0,
        .address_bits = 0,
        .dummy_bits = 0,
    };
    ESP_ERROR_CHECK(spi_bus_add_device(s_host, &devcfg, &s_dev));

    s_txbuf = (uint8_t*)heap_caps_malloc(frame_size, MALLOC_CAP_DMA);
    if (!s_txbuf) return ESP_ERR_NO_MEM;
    memset(s_txbuf, 0, frame_size);

    ULOGI(TAG, "SPI MASTER ready (frame=%u, MOSI=%d MISO=%d SCLK=%d CS=%d)",
          (unsigned)frame_size, mosi, miso, sclk, cs);
    return ESP_OK;
}

esp_err_t spi_link_send(const void *data, size_t len, TickType_t timeout)
{
    (void)timeout; // non utilisé en maître
    if (!s_dev || !s_txbuf || s_frame_sz == 0) return ESP_ERR_INVALID_STATE;

    size_t n = (len > s_frame_sz) ? s_frame_sz : len;
    memcpy(s_txbuf, data, n);
    if (n < s_frame_sz) memset(s_txbuf + n, 0, s_frame_sz - n);

    spi_transaction_t t = {
        .length   = s_frame_sz * 8, // bits
        .tx_buffer = s_txbuf,
        .rx_buffer = NULL
    };
    esp_err_t r = spi_device_transmit(s_dev, &t);
    if (r != ESP_OK) ULOGW(TAG, "transmit err=%d", r);
    return r;
}

void spi_link_poll(void) { /* no-op en maître */ }
bool spi_link_busy(void) { return false; }
