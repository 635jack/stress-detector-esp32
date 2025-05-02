import numpy as np
import tensorflow as tf
from tensorflow.keras.models import Sequential
from tensorflow.keras.layers import Dense, Dropout, BatchNormalization
from sklearn.model_selection import train_test_split
import matplotlib.pyplot as plt
import os

# 🎯 parametres
SEQUENCE_LENGTH = 1500
N_FEATURES = 2
N_CLASSES = 3
BATCH_SIZE = 32
EPOCHS = 50

# 📝 chemins des fichiers
DATA_DIR = "/Users/kojack/Documents/PlatformIO/Projects/stress-detector-esp32/data"

def load_data():
    """charge les donnees preprocessees"""
    X = np.load(os.path.join(DATA_DIR, 'X_processed.npy'))
    y = np.load(os.path.join(DATA_DIR, 'y_processed.npy'))
    
    # normalisation des donnees 🔄
    mean = np.mean(X, axis=(0, 1))
    std = np.std(X, axis=(0, 1))
    X = (X - mean) / (std + 1e-8)
    
    # aplatir les donnees pour le MLP
    X = X.reshape(X.shape[0], -1)
    
    return X, y

def create_model():
    """cree le modele MLP simple"""
    model = Sequential([
        # couche d'entree 📥 avec forme fixe
        Dense(64, activation='relu', input_shape=(SEQUENCE_LENGTH * N_FEATURES,)),
        BatchNormalization(),
        Dropout(0.2),
        
        # couche cachee 🎯
        Dense(32, activation='relu'),
        BatchNormalization(),
        Dropout(0.2),
        
        # couche de sortie 📊
        Dense(N_CLASSES, activation='softmax')
    ])
    
    # compilation avec optimisations
    optimizer = tf.keras.optimizers.Adam(learning_rate=0.001)
    model.compile(optimizer=optimizer,
                 loss='sparse_categorical_crossentropy',
                 metrics=['accuracy'])
    
    return model

def plot_training_history(history):
    """visualise l'historique d'entrainement"""
    plt.figure(figsize=(12, 4))
    
    # 📊 accuracy
    plt.subplot(1, 2, 1)
    plt.plot(history.history['accuracy'], label='train')
    plt.plot(history.history['val_accuracy'], label='validation')
    plt.title('accuracy')
    plt.legend()
    
    # 📊 loss
    plt.subplot(1, 2, 2)
    plt.plot(history.history['loss'], label='train')
    plt.plot(history.history['val_loss'], label='validation')
    plt.title('loss')
    plt.legend()
    
    plt.tight_layout()
    plt.savefig(os.path.join(DATA_DIR, 'training_history.png'))
    plt.close()

def main():
    # 📝 chargement des donnees
    print("🔄 chargement des donnees...")
    X, y = load_data()
    
    # 🎯 split train/validation
    X_train, X_val, y_train, y_val = train_test_split(X, y, test_size=0.2, random_state=42)
    
    print(f"📊 dimensions des donnees:")
    print(f"X_train: {X_train.shape}")
    print(f"X_val: {X_val.shape}")
    
    # 🧠 creation et entrainement du modele
    print("🎯 creation du modele...")
    model = create_model()
    model.summary()
    
    print("⏳ entrainement du modele...")
    history = model.fit(
        X_train, y_train,
        batch_size=BATCH_SIZE,
        epochs=EPOCHS,
        validation_data=(X_val, y_val),
        verbose=1
    )
    
    # 📊 evaluation du modele
    print("📊 evaluation du modele...")
    test_loss, test_acc = model.evaluate(X_val, y_val, verbose=0)
    print(f"accuracy: {test_acc:.4f}")
    
    # 📈 visualisation de l'entrainement
    plot_training_history(history)
    
    # 💾 sauvegarde du modele
    print("💾 sauvegarde du modele...")
    model.save(os.path.join(DATA_DIR, 'stress_detector_model.h5'))
    
    # 📱 conversion pour TFLite
    print("📱 conversion pour TFLite...")
    
    # Utiliser l'ancien convertisseur au lieu du MLIR
    converter = tf.lite.TFLiteConverter.from_keras_model(model)
    
    # Configuration pour ESP32
    converter.target_spec.supported_ops = [
        tf.lite.OpsSet.TFLITE_BUILTINS
    ]
    
    # Forcer la version 3 du schéma
    converter.target_spec.schema_version = 3
    
    # Optimisations pour ESP32
    converter.optimizations = [tf.lite.Optimize.DEFAULT]
    converter.target_spec.supported_types = [tf.float32]
    converter.inference_input_type = tf.float32
    converter.inference_output_type = tf.float32
    
    # Désactiver le MLIR
    converter.experimental_new_converter = False
    
    # Forcer la forme d'entrée fixe
    converter.target_spec.supported_ops = [
        tf.lite.OpsSet.TFLITE_BUILTINS
    ]
    
    # Conversion du modèle
    tflite_model = converter.convert()
    
    # 💾 sauvegarde du modele TFLite
    model_dir = os.path.join(DATA_DIR, 'model')
    os.makedirs(model_dir, exist_ok=True)
    
    tflite_path = os.path.join(model_dir, 'model.tflite')
    with open(tflite_path, 'wb') as f:
        f.write(tflite_model)
    
    # Vérification du modèle créé
    print(f"✅ Modèle TFLite sauvegardé: {tflite_path}")
    print(f"✅ Taille du modèle: {os.path.getsize(tflite_path)} octets")
    
    # Utiliser un interpréteur pour vérifier la version du schéma
    interpreter = tf.lite.Interpreter(model_path=tflite_path)
    interpreter.allocate_tensors()
    
    print("✅ terminé!")

if __name__ == "__main__":
    main()