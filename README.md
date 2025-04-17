# Détecteur de Stress ESP32

Ce projet utilise un ESP32 avec un capteur MAX30102 pour détecter le stress. Le capteur mesure la fréquence cardiaque et la variabilité de la fréquence cardiaque (HRV) qui sont des indicateurs importants du niveau de stress.

## Approches du Projet

### 1. Approche Simple (Actuelle)
- Utilisation directe des données brutes du capteur MAX30102
- Mesure des signaux RED et IR
- Affichage des valeurs sur le port série
- Configuration I2C optimisée pour le capteur

### 2. Approche Avancée (À venir)
- Calcul de la fréquence cardiaque
- Analyse de la variabilité de la fréquence cardiaque (HRV)
- Détection des pics R pour une analyse plus précise
- Filtrage des signaux pour réduire le bruit
- Algorithmes de détection de stress basés sur la HRV

## Configuration Matérielle

- ESP32 Feather
- Capteur MAX30102
- Connexions :
  - VCC -> 3.3V
  - GND -> GND
  - SDA -> GPIO 23
  - SCL -> GPIO 22

## Dépendances

- SparkFun MAX3010x Pulse and Proximity Sensor Library
- Wire (pour la communication I2C)

## Installation

1. Clonez ce dépôt
2. Ouvrez le projet dans PlatformIO
3. Téléversez le code sur votre ESP32
4. Ouvrez le moniteur série à 115200 bauds

## Utilisation

1. Connectez le capteur MAX30102 à votre doigt
2. Observez les valeurs sur le moniteur série
3. Les valeurs RED et IR seront affichées en temps réel

## Prochaines Étapes

- Implémentation de l'algorithme de détection de pics R
- Calcul de la fréquence cardiaque
- Analyse de la HRV
- Interface utilisateur pour afficher le niveau de stress 