# VroomVroomProject

Projet académique réalisé à l’ENSICAEN.
Dépôt publié à des fins de présentation de compétences techniques.

Projet embarqué pour piloter une voiture téléguidée via une architecture mixte **STM32 + ESP32** sous **FreeRTOS**.

Chaîne de commande complète :  
**Manette → Application PC → Wi-Fi (UDP) → ESP32 → SPI → STM32 → PWM (moteurs/servos)**

- L’ESP32 gère la connectivité réseau (réception UDP) et relaie les ordres en SPI vers le STM32.  
- Le STM32 exécute les consignes temps réel (PWM, sécurité, logique moteur).  
- L’ensemble tourne sur **FreeRTOS** côté ESP32 et STM32.  

---

##  Sommaire
1. Démarrage rapide
2. Lancement de l’application PC
3. Flash de l’ESP32
4. Flash de la STM32
5. Précautions

---

##  Démarrage rapide

Cloner le dépôt :
```bash
git clone https://gitlab.ecole.ensicaen.fr/rozoy/vroomvroomproject.git
cd vroomvroomproject
```

Le projet est divisé en 3 parties :  
- **APPLI** : Application PC (Linux/Windows/Mac) pour l’envoi des données manette Xbox.  
- **ESP32** : Code à flasher sur l’ESP32.  
- **STM32** : Code à flasher sur le STM32.  

---

##  Préparer l’application PC

1. Aller dans le dossier :
   ```bash
   cd APPLI
   ```

2. Installer Python, `venv` et `pip` :
   ```bash
   sudo apt install -y python3 python3-venv python3-pip
   ```

3. Créer un environnement virtuel :
   ```bash
   python3 -m venv .venv
   ```

4. Activer l’environnement virtuel :
   ```bash
   source .venv/bin/activate
   ```

5. *(Optionnel)* Mettre à jour `pip` :
   ```bash
   python -m pip install --upgrade pip
   ```

6. Installer les dépendances :
   ```bash
   pip install -r requirements.txt
   ```

7. Lancer l’application :
   ```bash
   python main.py
   ```

Vous pouvez maintenant brancher votre manette et appuyer sur **StartApp**.  
Une connexion doit alors être établie entre la manette et l’application.

---

## 🔌 Flasher l’ESP32

#### 1. Prérequis & Installation (Windows)

1. Installer [ESP-IDF Tools](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/windows-setup.html) pour IDF 5.5.1 (inclut Python, Ninja, toolchains).
2. Ouvrir le terminal ESP-IDF PowerShell (ou “ESP-IDF CMD”).
3. Cloner le repo:

   ```sh
   git clone https://gitlab.ecole.ensicaen.fr/rozoy/vroomvroomproject.git
   cd VroomVroomProject/ESP32/Project
   ```
4. Choisir la cible:

   ```sh
   idf.py set-target esp32c3
   ```


#### 2. Configuration du projet

Configurer via `idf.py menuconfig` dans un terminal ESP-IDF:

- **Réseau (SoftAP)**
  - `ESP_WIFI_SSID` : SSID du point d’accès
  - `ESP_WIFI_PASSWORD` : mot de passe WPA2/WPA3
  - `ESP_WIFI_CHANNEL` : canal (1–13)
  - `ESP_MAX_STA_CONN` : nb max stations
  - `ESP_GTK_REKEYING_ENABLE` et `ESP_GTK_REKEY_INTERVAL`
- **Console UART**
  - `USER_UART_ENABLE` : activer/désactiver
  - `USER_UART_BAUD` : baud rate (ex. 625000)
- **LED**
  - `USER_LED_GPIO` : GPIO pour WS2812 (par défaut 8)

#### 3. Compilation, flash & monitor

```sh
idf.py build
idf.py -p <PORT> flash
idf.py -p <PORT> monitor
# Quitter le monitor : Ctrl+]
```

Le monitoring est optionnel mais permet d'afficher des logs détaillés.

---

## 🔧 Flasher la STM32

1. Ouvrir **STM32CubeIDE**.  
2. Importer le projet **ReceiveFinal** présent dans le dossier `STM32/`.  
3. Compiler et téléverser dans la carte en cliquant sur le bouton **Run** (flèche verte).  

---

##  Précautions

- Bien suivre la fiche technique fournie : **Precaution_a_prendre.pdf**.  
- Toujours connecter les 2 pins avec un cavalier (**NSS/CS → PB10**).  
- Pour la sécurité du Chipset de votre ordinateur, toujours utiliser un Hub USB externe (armoire de droite en salle A203). Pourquoi : 

Sur les STM32 Nucleo, par défaut, vous verrez qu'un jumper sur le connecteur JP5 est connecté en mode U5V (alimentation par USB). Aucun soucis tant que l'on ne test pas sur le véhicule
Sur le véhicule, le jumper sur le connecteur JP5 est connecté en mode E5V (alimentation Externe). Dans notre cas, alimentation externe en 6V par le module de puissance 1060

En résumé, NE PAS ESSAYER de mettre le jumper sur U5V tout en alimentant le véhicule par le module de puissance 1060, sans quoi cela peut être destructif pour le chipset de votre PC (conflit d'alimentation) ! Toujours utiliser un Hub en sécurité 

En revanche, vous pouvez laisser le jumper sur E5V, alimenter avec le module 1060 en 6V et programmer la carte STM32 depuis votre PC !

En résumé :

Test sur véhicule, être certain d'être en E5V
Test sur votre PC seulement (sans véhicule), être certain d'être en U5V

---
