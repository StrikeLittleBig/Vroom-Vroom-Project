/**
 * @file    user_uart.h
 * @brief   Initialisation de la console UART (stdout/stderr) pour logs I/O.
 *
 * @project Projet immersif – ESP32
 * @author  Hrithik SHEIKH
 * @date    2025-09-15
 *
 * @details
 *   - user_uart_init(): configure UART0 comme console VFS, avec le débit choisi
 *     (CONFIG_USER_UART_BAUD si défini, sinon 115200 bauds).
 *   - Ce module ne règle PAS les niveaux de log (voir user_log_setup).
 */

#pragma once
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief  Initialise la console UART (UART0) et redirige stdout/stderr dessus.
 * @return ESP_OK si OK, sinon code d’erreur.
 */
esp_err_t user_uart_init(void);

#ifdef __cplusplus
}
#endif
