# 🤖 Scripts d'analyse des données de stress

Ce dossier contient les scripts Python pour le prétraitement et l'entraînement du modèle de détection de stress.

## 📋 Prérequis

```bash
pip install -r requirements.txt
```

## 🔄 Pipeline de traitement

1. **Prétraitement des données** (`preprocess.py`)
   - Charge les données brutes du capteur MAX30102
   - Nettoie et normalise les signaux IR et RED
   - Crée des séquences avec augmentation de données
   - Génère des visualisations pour l'analyse

2. **Entraînement du modèle** (`train_model.py`)
   - Utilise une architecture CNN-LSTM
   - Entraîne sur les séquences augmentées
   - Sauvegarde le modèle en format H5 et TFLite
   - Génère des graphiques de performance

## 🚀 Utilisation

1. Prétraitement :
```bash
python preprocess.py
```
- Entrée : `stress_data_*.csv`
- Sortie : 
  - `X_processed.npy`, `y_processed.npy`
  - Visualisations : `raw_data_analysis.png`, `features_analysis.png`, `augmented_sequences.png`

2. Entraînement :
```bash
python train_model.py
```
- Entrée : `X_processed.npy`, `y_processed.npy`
- Sortie :
  - `stress_detector_model.h5`
  - `stress_detector_model.tflite`
  - `training_history.png`

## 📊 Structure des données

- **Données brutes** : CSV avec colonnes [ir, red, state]
- **États** : 
  - 0 : repos
  - 1 : stress
  - 2 : exercice
- **Séquences** : 1500 échantillons (15 secondes à 100Hz)

## 🔧 Configuration

Les paramètres principaux peuvent être modifiés dans les scripts :
- `WINDOW_SIZE` : taille des séquences
- `SAMPLING_RATE` : fréquence d'échantillonnage
- `AUGMENTATION_FACTOR` : nombre de versions augmentées 