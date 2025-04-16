# Détecteur de Stress ESP32

Ce projet utilise un ESP32 et un capteur MAX30102 pour détecter le stress en temps réel en analysant la variabilité de la fréquence cardiaque (HRV).

## 🛠️ Matériel requis
- ESP32 (testé avec Feather ESP32)
- Capteur MAX30102
- Câbles de connexion

## 📦 Structure du projet
- `src/main.cpp` : Programme principal
- `include/HRVCalculator.h` : Calcul de la HRV
- `src/HRVCalculator.cpp` : Implémentation du calculateur HRV

## 🌳 Branches Git
- `main` : Version de base avec lecture du capteur et calcul HRV
- `offline-mlp` : Version avec MLP pré-entraîné
- `online-hebb` : Version avec apprentissage Hebbien en ligne

## 🔧 Installation
1. Cloner le dépôt
2. Installer les dépendances PlatformIO
3. Compiler et téléverser sur l'ESP32

## 📊 Fonctionnalités
- Lecture en temps réel du BPM et SpO2
- Calcul de la HRV (RMSSD)
- Affichage des données sur le port série

## 🔄 Workflow de développement
1. Commencer par la branche `main` pour la version de base
2. Tester la lecture du capteur et le calcul HRV
3. Choisir une approche d'apprentissage automatique :
   - `offline-mlp` pour un modèle pré-entraîné
   - `online-hebb` pour l'apprentissage en ligne

## 📝 Notes
- Le capteur MAX30102 doit être correctement connecté aux broches I2C (SDA: 21, SCL: 22)
- La fréquence d'échantillonnage est de 100Hz
- Le calcul de la HRV utilise une fenêtre glissante de 100 échantillons 