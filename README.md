# 🤖 Détecteur de Stress ESP32 - Version Base

## 📝 Description
Ce projet utilise un ESP32 avec un capteur MAX30102 pour détecter le stress en temps réel en analysant la variabilité de la fréquence cardiaque (HRV). Cette version de base se concentre sur la lecture des données du capteur et le calcul de la HRV.

## 🎯 Fonctionnalités
- Mesure des signaux IR et RED en temps réel
- Calcul du BPM (Battements Par Minute)
- Calcul de la SpO2 (Saturation en oxygène)
- Calcul de la HRV (RMSSD)
- Interface utilisateur avec LED d'état
- Affichage des données sur le port série

## 🛠️ Matériel requis
- ESP32 (testé avec Feather ESP32)
- Capteur MAX30102
- Câbles de connexion

## 📦 Structure du projet
- `src/main.cpp` : Programme principal
- `include/HRVCalculator.h` : Calcul de la HRV
- `src/HRVCalculator.cpp` : Implémentation du calculateur HRV

## 🔧 Installation
1. Cloner le dépôt
2. Installer les dépendances PlatformIO
3. Compiler et téléverser sur l'ESP32

## 📊 Fonctionnalités
- Lecture en temps réel du BPM et SpO2
- Calcul de la HRV (RMSSD)
- Affichage des données sur le port série

## 🔄 Workflow de développement
1. Commencer par cette branche `main` pour la version de base
2. Tester la lecture du capteur et le calcul HRV
3. Choisir une approche d'apprentissage automatique :
   - `offline-mlp` pour un modèle pré-entraîné
   - `online-hebb` pour l'apprentissage en ligne

## 📝 Notes
- Le capteur MAX30102 doit être correctement connecté aux broches I2C (SDA: 21, SCL: 22)
- La fréquence d'échantillonnage est de 100Hz
- Le calcul de la HRV utilise une fenêtre glissante de 100 échantillons 