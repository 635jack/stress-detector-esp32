import pandas as pd
import numpy as np
from scipy import signal
import matplotlib.pyplot as plt
import seaborn as sns
import os
from io import StringIO

# 🎯 parametres
WINDOW_SIZE = 100  # 1 seconde a 100Hz
OVERLAP = 0.5      # 50% de chevauchement
AUGMENTATION_FACTOR = 7  # nombre de versions augmentees par sequence

# 📝 chemins des fichiers
DATA_FILE = "/Users/kojack/Documents/PlatformIO/Projects/stress-detector-esp32/data/stress_data_20250425_113807.csv"
OUTPUT_DIR = "/Users/kojack/Documents/PlatformIO/Projects/stress-detector-esp32/data"

def load_data(file_path):
    """charge et nettoie les donnees brutes"""
    # 📝 lecture du fichier
    with open(file_path, 'r') as f:
        lines = f.readlines()
    
    # 🧹 nettoyage des donnees
    cleaned_lines = []
    for line in lines:
        # on garde uniquement les lignes qui contiennent exactement 4 nombres separes par des virgules
        parts = line.strip().split(',')
        if len(parts) == 4 and all(part.strip().replace('.', '').isdigit() for part in parts):
            cleaned_lines.append(line)
    
    # 📝 creation du dataframe
    df = pd.read_csv(StringIO(''.join(cleaned_lines)), 
                     names=['ir', 'red', 'state', 'xiaomi_score'])
    
    # conversion en numerique
    df['ir'] = pd.to_numeric(df['ir'])
    df['red'] = pd.to_numeric(df['red'])
    
    # on ne garde que les colonnes utiles
    df = df[['ir', 'red', 'state']]
    
    return df

def augment_sequence(sequence):
    """augmente une sequence de donnees"""
    augmented_sequences = []
    
    # 🎯 bruit gaussien (sigma = 5% de l'amplitude)
    noise = np.random.normal(0, 0.05 * np.std(sequence), sequence.shape)
    augmented_sequences.append(sequence + noise)
    
    # 🎯 variations d'amplitude (±10%)
    amplitude_factor = np.random.uniform(0.9, 1.1)
    augmented_sequences.append(sequence * amplitude_factor)
    
    # 🎯 rotation des signaux (±5 degres)
    angle = np.random.uniform(-5, 5)
    rotation_matrix = np.array([[np.cos(np.radians(angle)), -np.sin(np.radians(angle))],
                              [np.sin(np.radians(angle)), np.cos(np.radians(angle))]])
    rotated = np.dot(sequence, rotation_matrix)
    augmented_sequences.append(rotated)
    
    # 🎯 combinaison de transformations
    for _ in range(AUGMENTATION_FACTOR - 3):
        # melange aleatoire des transformations
        if np.random.random() < 0.5:
            noise = np.random.normal(0, 0.05 * np.std(sequence), sequence.shape)
            augmented = sequence + noise
        else:
            amplitude_factor = np.random.uniform(0.9, 1.1)
            augmented = sequence * amplitude_factor
        
        angle = np.random.uniform(-5, 5)
        rotation_matrix = np.array([[np.cos(np.radians(angle)), -np.sin(np.radians(angle))],
                                  [np.sin(np.radians(angle)), np.cos(np.radians(angle))]])
        augmented = np.dot(augmented, rotation_matrix)
        
        augmented_sequences.append(augmented)
    
    return np.array(augmented_sequences)

def normalize_features(df):
    """normalise les features"""
    # 📈 normalisation
    for feature in ['ir', 'red']:
        mean = df[feature].mean()
        std = df[feature].std()
        df[f'{feature}_norm'] = (df[feature] - mean) / std
    
    return df

def prepare_sequences(df, sequence_length=1500):
    """prepare les sequences pour l'entrainement"""
    # 📝 creation des sequences
    sequences = []
    labels = []
    
    for i in range(0, len(df) - sequence_length, int(sequence_length * 0.5)):
        sequence = df.iloc[i:i+sequence_length]
        if len(sequence) == sequence_length:
            # 🎯 features normalisees
            X = sequence[['ir_norm', 'red_norm']].values
            
            # 🏷️ label (0: repos, 2: exercice)
            y = sequence['state'].iloc[0]
            
            # 🎯 augmentation des donnees
            augmented_X = augment_sequence(X)
            
            sequences.extend(augmented_X)
            labels.extend([y] * AUGMENTATION_FACTOR)
    
    return np.array(sequences), np.array(labels)

def visualize_raw_data(df):
    """visualise les donnees brutes"""
    plt.figure(figsize=(15, 10))
    
    # 📊 signaux bruts
    plt.subplot(2, 1, 1)
    plt.plot(df['ir'].iloc[:1000], label='IR')
    plt.plot(df['red'].iloc[:1000], label='RED')
    plt.title('signaux bruts (premiers 1000 echantillons)')
    plt.legend()
    
    # 📊 distribution des etats
    plt.subplot(2, 1, 2)
    sns.countplot(x='state', data=df)
    plt.title('distribution des etats')
    
    plt.tight_layout()
    plt.savefig(os.path.join(OUTPUT_DIR, 'raw_data_analysis.png'))
    plt.close()

def visualize_features(df):
    """visualise les features extraites"""
    plt.figure(figsize=(15, 10))
    
    # 📊 features normalisees
    plt.subplot(2, 1, 1)
    plt.plot(df['ir_norm'].iloc[:1000], label='IR norm')
    plt.plot(df['red_norm'].iloc[:1000], label='RED norm')
    plt.title('signaux normalises (premiers 1000 echantillons)')
    plt.legend()
    
    # 📊 correlation des features
    plt.subplot(2, 1, 2)
    features = ['ir_norm', 'red_norm']
    sns.heatmap(df[features].corr(), annot=True, cmap='coolwarm')
    plt.title('matrice de correlation des features')
    
    plt.tight_layout()
    plt.savefig(os.path.join(OUTPUT_DIR, 'features_analysis.png'))
    plt.close()

def visualize_augmented_sequences(X, y):
    """visualise les sequences augmentees"""
    plt.figure(figsize=(15, 10))
    
    # 📊 exemple de sequence originale et augmentee
    plt.subplot(2, 1, 1)
    plt.plot(X[0, :, 0], label='IR original')
    plt.plot(X[1, :, 0], label='IR augmente')
    plt.title('exemple de sequence originale et augmentee')
    plt.legend()
    
    # 📊 distribution des labels apres augmentation
    plt.subplot(2, 1, 2)
    sns.countplot(x=y)
    plt.title('distribution des labels apres augmentation')
    
    plt.tight_layout()
    plt.savefig(os.path.join(OUTPUT_DIR, 'augmented_sequences.png'))
    plt.close()

def main():
    # 📝 chargement des donnees
    df = load_data(DATA_FILE)
    
    # 📊 visualisation des donnees brutes
    visualize_raw_data(df)
    
    # 📈 normalisation
    df = normalize_features(df)
    
    # 📊 visualisation des features
    visualize_features(df)
    
    # 🎯 preparation des sequences
    X, y = prepare_sequences(df)
    
    # 📊 visualisation des sequences augmentees
    visualize_augmented_sequences(X, y)
    
    # 📊 affichage des statistiques
    print(f"nombre de sequences: {len(X)}")
    print(f"dimensions des sequences: {X.shape}")
    print(f"distribution des labels: {np.bincount(y)}")
    
    # 📈 sauvegarde des donnees preprocessees
    np.save(os.path.join(OUTPUT_DIR, 'X_processed.npy'), X)
    np.save(os.path.join(OUTPUT_DIR, 'y_processed.npy'), y)

if __name__ == "__main__":
    main() 