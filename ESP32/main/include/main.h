/**
 * @file    main.h
 * @brief   Point d’entrée de l’application ESP32.
 *          Initialise le logging, NVS, LED de statut, SoftAP Wi-Fi, lien SPI
 *          et serveur UDP (réception vers file de messages).
 *
 * @project Projet immersif – ESP32
 * @author  Hrithik SHEIKH
 * @date    2025-09-15
 *
 * @details Fonctions:
 *   - app_main(): séquence d’initialisation et démarrage des services.
 *
 * @copyright
 *   Ce fichier est fourni dans le cadre du projet pédagogique. Licence interne
 *   ou MIT selon dépôt (à préciser).
 */
#ifndef APP_MAIN_H_
#define APP_MAIN_H_

#include <stdint.h>

/* ========================== Configuration locale ========================== */
/* NB: idéalement à déplacer en Kconfig plus tard.                           */
#define APP_LED_COUNT        8
#define APP_QUEUE_LEN        128
#define APP_SPI_FRAME_SIZE   64

/* Brochage (connecteur carte) */
#define PIN_MOSI  2   /* J3-4  */
#define PIN_MISO  1   /* J3-3  */
#define PIN_SCLK  0   /* J3-2  */
#define PIN_CS    5   /* J1-7  */
#define PIN_HS    3   /* J3-5  */


/* ============================= API du module ============================== */
/**
 * @brief Entrée principale FreeRTOS.
 * @note  Appelée automatiquement par l’IDF après boot.
 */
void app_main(void);

#endif /* APP_MAIN_H_ */
