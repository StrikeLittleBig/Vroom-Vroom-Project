/**
 * @file    wifi_ap.h
 * @brief   Démarrage d’un point d’accès (SoftAP) et gestion des événements.
 *
 * @project Projet immersif – ESP32
 * @author  Hrithik SHEIKH
 * @date    2025-09-15
 *
 * @details
 *   - wifi_ap_start(): initialise la pile réseau, le Wi-Fi et lance le SoftAP.
 *   - wifi_ap_get_sta_count(): retourne le nombre de stations associées.
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/** Lance la configuration et le démarrage du SoftAP (utilise sdkconfig). */
void wifi_ap_start(void);

/** Nombre courant de stations associées (best-effort). */
int  wifi_ap_get_sta_count(void);

#ifdef __cplusplus
}
#endif
