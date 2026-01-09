/**
 * @file    spi_link.h
 * @brief   Lien SPI (esclave) avec signal de handshake vers le maître.
 *
 * @project Projet immersif – ESP32
 * @author  Hrithik SHEIKH
 * @date    2025-09-15
 *
 * @details
 *   - spi_link_init(): configure le bus SPI en mode esclave + broche HS.
 *   - spi_link_send(): copie une trame en TX, arme une transaction et attend
 *                      (jusqu’au timeout) que le maître la « clock ».
 *   - spi_link_poll(): variante non bloquante pour finaliser si besoin.
 *   - spi_link_busy(): indique si une transaction est en attente (HS=1).
 */

#pragma once
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"
#include "freertos/FreeRTOS.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief  Initialise le lien SPI esclave.
 * @param  frame_size Taille d’une trame (octets).
 * @param  mosi/miso/sclk/cs Brochage SPI.
 * @param  hs        GPIO handshake (sortie, 0 au repos, 1 = data prête).
 * @return ESP_OK si OK, code d’erreur sinon.
 */
esp_err_t spi_link_init(size_t frame_size,
                        int mosi, int miso, int sclk, int cs);

/**
 * @brief  Envoie une trame (bloquant jusqu’à ce que le maître la lise).
 * @param  data Pointeur vers les octets à envoyer.
 * @param  len  Longueur utile (<= frame_size). Le reste est 0-pad si besoin.
 * @param  timeout Délai max d’attente (ticks) pour la complétion.
 * @return ESP_OK, ESP_ERR_INVALID_STATE (déjà en attente), ESP_ERR_TIMEOUT,
 *         ou autre code d’erreur SPI.
 */
esp_err_t spi_link_send(const void *data, size_t len, TickType_t timeout);

/** @brief À appeler régulièrement si on ne veut pas bloquer. */
void spi_link_poll(void);

/** @brief Vrai si une transaction est en attente (HS=1). */
bool spi_link_busy(void);

#ifdef __cplusplus
}
#endif
