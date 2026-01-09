Projet immersif – ESP32-C3 (SoftAP + UDP → SPI)

Ce firmware pour ESP32-C3 met en place :

un point d’accès Wi-Fi (SoftAP),
un serveur UDP recevant des trames gamepad (gp_packet_t, 28 octets),
un pont UDP → SPI (esclave) avec handshake GPIO vers un STM32 maître,
une LED WS2812 d’état,
une console UART0 (baud configurable),
une politique de logs propre (peu bavarde, tags utiles seulement).
Cible testée : ESP-IDF 5.5.1, ESP32-C3-DevKitC-02.


1) Arborescence (exemple)
ESP32/Project/
├─ main/
│  ├─ include/
│  │  ├─ main.h
│  │  ├─ gp_proto.h
│  │  ├─ led_rgb.h
│  │  ├─ udp_server.h
│  │  ├─ user_log_setup.h
│  │  ├─ user_uart.h
│  │  └─ spi_link.h
│  ├─ src/
│  │  ├─ main.c
│  │  ├─ wifi_ap.c
│  │  ├─ led_rgb.c
│  │  ├─ udp_server.c
│  │  ├─ user_log_setup.c
│  │  ├─ user_uart.c
│  │  └─ spi_link.c
│  ├─ CMakeLists.txt
│  └─ Kconfig.projbuild
├─ managed_components/  (led_strip, etc.)
├─ sdkconfig            (généré)
├─ sdkconfig.defaults   (tes valeurs par défaut projet)
└─ CMakeLists.txt

2) Prérequis / Installation ESP-IDF
Windows

Installer ESP-IDF Tools (Espressif) pour IDF 5.5.1 (inclut Python, Ninja, toolchains).
Ouvrir le ESP-IDF PowerShell (ou “ESP-IDF CMD”).
Cloner le dépôt puis définir la cible :

git clone <ton_repo>
cd <ton_repo>/ESP32/Project
idf.py set-target esp32c3


3) Configuration du projet (menuconfig)

Les options pertinents sont exposées dans main/Kconfig.projbuild.
Réseau (SoftAP)
ESP_WIFI_SSID (SSID du point d’accès)
ESP_WIFI_PASSWORD (mot de passe WPA2/WPA3)
ESP_WIFI_CHANNEL (1–13)
ESP_MAX_STA_CONN (nb max stations)
ESP_GTK_REKEYING_ENABLE et ESP_GTK_REKEY_INTERVAL
Console UART
USER_UART_ENABLE (activer/désactiver)
USER_UART_BAUD (baud rate, ex. 625000)
LED
USER_LED_GPIO (par défaut 8 pour WS2812 sur C3-DevKitC-02)
Astuce : place des valeurs par défaut de projet dans sdkconfig.defaults pour qu’elles persistent (CI, nouvelles machines).
Exemple de sdkconfig.defaults :

CONFIG_USER_UART_ENABLE=y
CONFIG_USER_UART_BAUD=115200
CONFIG_ESP_WIFI_SSID="D_RACE_AP"
CONFIG_ESP_WIFI_PASSWORD="secret42"
CONFIG_ESP_WIFI_CHANNEL=6


Lancer la configuration :

idf.py menuconfig

4) Construction, flash, monitor
idf.py build
idf.py -p <PORT> flash
idf.py -p <PORT> monitor
# quitter le monitor : Ctrl+]


Tu devrais voir des logs structurés (tags : boot, wifi_ap, udp, spi_link, led, etc.).

5) Brochage matériel (ESP32-C3-DevKitC-02)
 Broches à éviter (bus Flash QSPI)

Mapping recommandé (exemple sûr) pour SPI2 (esclave) + HS
#define PIN_MOSI  3    /* J3-5  */
#define PIN_MISO  1    /* J3-3  */
#define PIN_SCLK  19   /* J3-13 */
#define PIN_CS    18   /* J3-12 */
#define PIN_HS     9   /* J1-12 : GPIO libre pour handshake */


Conserve GPIO20/21 pour l’UART0 si tu veux garder la console (RX/TX).
Évite GPIO8 si tu utilises la LED WS2812 onboard.

Handshake (HS) : sortie ESP (0 = repos, 1 = trame prête). Le STM32 maître lit la ligne et ne “clocke” le SPI que quand HS=1.

6) Comportement au runtime

LED WS2812 (une LED) :
LED_WIFI_DOWN : jaune clignotant (AP down / init)
LED_AP_UP : vert clignotant (AP actif, pas de STA)
LED_STA_CONNECTED : bleu clignotant (au moins 1 STA)

Wi-Fi SoftAP :
SSID/PASS/CHAN via menuconfig
MAC SoftAP loggée au boot
Serveur UDP :
écoute sur GP_UDP_PORT (défaut 5555 ou CONFIG_GP_UDP_PORT si tu l’ajoutes)
reçoit soit des trames binaires 28 o. (gp_packet_t), soit un fallback ASCII de 32 bits 0/1 pour buttons.
les paquets sont poussés dans une queue FreeRTOS (taille configurable côté main.c).
Pont UDP → SPI :
chaque paquet gp_packet_t est préparé dans un frame de taille fixe (APP_SPI_FRAME_SIZE)
spi_link_send() aligne/zero-pad si nécessaire, lève HS = 1, queue la transaction, attend la clock du maître (timeout), baisse HS = 0.