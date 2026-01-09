/**
 * @file    led_rgb.h
 * @brief   Pilotage d’un ruban/single LED RGB via le driver led_strip (RMT).
 *
 * @project Projet immersif – ESP32
 * @author  Hrithik SHEIKH
 * @date    2025-09-15
 *
 * @details
 *   - led_init(gpio): initialise le périphérique (1 LED adressable sur <gpio>)
 *                     et lance la tâche de clignotement de statut.
 *   - led_set_mode(m): change le mode (couleur) affiché de façon clignotante.
 */

#pragma once
#include <stdint.h>

/** Modes de statut (couleur + blink) */
typedef enum {
    LED_WIFI_DOWN = 0,   /**< Jaune clignotant */
    LED_AP_UP,           /**< Vert clignotant  */
    LED_STA_CONNECTED    /**< Bleu clignotant  */
} led_mode_t;

/**
 * @brief   Initialise la LED RGB (1 LED) sur le GPIO donné et démarre la tâche.
 * @param   gpio  Numéro de GPIO (ex: 8 sur ESP32-C3 DevKitC-02).
 */
void led_init(int gpio);

/**
 * @brief   Change le mode d’affichage (thread-safe).
 * @param   m  Nouveau mode.
 */
void led_set_mode(led_mode_t m);
