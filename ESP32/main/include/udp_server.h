/**
 * @file    udp_server.h
 * @brief   Serveur UDP pour la réception de paquets gamepad et dispatch vers une queue.
 *
 * @project Projet immersif – ESP32
 * @author  Hrithik SHEIKH
 * @date    2025-09-15
 *
 * @details
 *   - udp_server_start(q): démarre les tâches UDP et pont UDP→SPI, en publiant
 *     chaque paquet décodé dans la queue fournie (élément = sizeof(gp_packet_t)).
 *   - udp_server_get_queue(): accès en lecture au handle interne (facultatif).
 *   - udp_buffer_count(): nombre d’éléments en attente dans la queue.
 *   - udp_drop_count(): nombre de paquets dropés (stat).
 */

#pragma once
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include <stdint.h>  

/**
 * @brief Démarre le serveur UDP et le pont UDP→SPI.
 * @param rx_queue Queue de destination (élément = sizeof(gp_packet_t)).
 * @return ESP_OK si OK, sinon un code d’erreur.
 */
esp_err_t udp_server_start(QueueHandle_t rx_queue);

/**
 * @brief  Retourne la queue interne utilisée par le serveur UDP.
 * @return Handle de queue ou NULL si non initialisée.
 */
QueueHandle_t udp_server_get_queue(void);

/**
 * @brief  Nombre de paquets en attente dans la queue.
 * @return Compteur d’éléments en file.
 */
UBaseType_t udp_buffer_count(void);

/**
 * @brief  Nombre cumulé de paquets dropés (stat).
 * @return Compteur depuis le démarrage.
 */
uint32_t udp_drop_count(void);
