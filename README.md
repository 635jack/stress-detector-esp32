# 🤖 Détecteur de Stress ESP32 - Version Base

## 📝 Description
Ce projet utilise un ESP32 avec un capteur MAX30102 pour détecter le stress en temps réel. Le capteur mesure les signaux IR et RED qui sont utilisés pour l'analyse du stress via un réseau de neurones.

## 🎯 Fonctionnalités
- Mesure des signaux IR et RED en temps réel (100Hz)
- Détection de la présence du doigt
- Analyse du stress avec classification en 3 états :
  - Repos
  - Stress
  - Exercice
- Interface utilisateur avec affichage de progression
- Utilisation de la PSRAM pour le stockage des données
- Stockage persistant avec SPIFFS

## 🛠️ Matériel requis
- ESP32 avec PSRAM
- Capteur MAX30102
- Câbles de connexion

## 📦 Structure du projet
- `src/main.cpp` : Programme principal
- `include/StressDetector.h` : Définition du détecteur de stress
- `src/StressDetector.cpp` : Implémentation du détecteur

## 🔧 Configuration du capteur
```cpp
particleSensor.setup();
particleSensor.setPulseAmplitudeRed(0x2F);
particleSensor.setPulseAmplitudeGreen(0);
particleSensor.setPulseAmplitudeIR(0x2F);
particleSensor.setPulseWidth(411);
particleSensor.setSampleRate(100);
particleSensor.setFIFOAverage(16);
```

## 📊 Fonctionnalités
- Détection automatique du doigt
- Échantillonnage à 100Hz
- Affichage de la progression en temps réel
- Classification avec probabilités pour chaque état
- Gestion des erreurs et mode dégradé

## 🔄 Workflow de développement
1. Commencer par cette branche `main` pour la version de base
2. Tester la lecture du capteur et la détection du doigt
3. Choisir une approche d'apprentissage automatique :
   - `offline-mlp` pour un modèle pré-entraîné
   - `online-hebb` pour l'apprentissage en ligne

## 📝 Notes
- Le capteur MAX30102 doit être correctement connecté aux broches I2C
- La PSRAM est requise pour le fonctionnement
- Le système utilise SPIFFS pour le stockage persistant
- Seuil de détection du doigt : 10000 