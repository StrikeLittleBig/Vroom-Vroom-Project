#include "user_uart.h"
#include "sdkconfig.h"
#include "esp_log.h"

/* Fallback si l’option Kconfig n’existe pas encore */
#ifndef CONFIG_USER_UART_BAUD
#define CONFIG_USER_UART_BAUD (115200)
#endif

#if CONFIG_USER_UART_ENABLE

#include "driver/uart.h"
#include "esp_vfs_dev.h"

static const char *TAG = "uart";

esp_err_t user_uart_init(void)
{
    const int uart_num = UART_NUM_0;

    /* Installer le driver UART0 (RX buffer 1 Ko suffit pour console) */
    esp_err_t err = uart_driver_install(uart_num, 1024, 0, 0, NULL, 0);
    if (err != ESP_OK) return err;

    const uart_config_t cfg = {
        .baud_rate  = CONFIG_USER_UART_BAUD,
        .data_bits  = UART_DATA_8_BITS,
        .parity     = UART_PARITY_DISABLE,
        .stop_bits  = UART_STOP_BITS_1,
        .flow_ctrl  = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB
    };
    ESP_ERROR_CHECK(uart_param_config(uart_num, &cfg));

    /* Conserver les broches par défaut du bootloader (NO_CHANGE) */
    ESP_ERROR_CHECK(uart_set_pin(uart_num,
                                 UART_PIN_NO_CHANGE, /* TX */
                                 UART_PIN_NO_CHANGE, /* RX */
                                 UART_PIN_NO_CHANGE, /* RTS */
                                 UART_PIN_NO_CHANGE  /* CTS */));

    /* API dispo en IDF 5.5.1 (dépréciée mais ok) */
    esp_vfs_dev_uart_use_driver(uart_num);
    esp_vfs_dev_uart_set_rx_line_endings(ESP_LINE_ENDINGS_CR);
    esp_vfs_dev_uart_set_tx_line_endings(ESP_LINE_ENDINGS_CRLF);

    ESP_LOGI(TAG, "Console UART0 prête @ %d bauds", CONFIG_USER_UART_BAUD);
    return ESP_OK;
}

#else  /* !CONFIG_USER_UART_ENABLE */

esp_err_t user_uart_init(void)
{
    return ESP_OK; /* UART console désactivée */
}

#endif /* CONFIG_USER_UART_ENABLE */
