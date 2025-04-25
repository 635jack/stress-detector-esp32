import numpy as np
import tensorflow as tf
from tensorflow.keras.models import Sequential
from tensorflow.keras.layers import Conv1D, LSTM, Dense, Dropout, BatchNormalization, MaxPooling1D, Flatten
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
    return X, y

def create_model():
    """cree le modele CNN-LSTM"""
    model = Sequential([
        # 🎯 couches convolutives pour extraire les features
        Conv1D(filters=64, kernel_size=5, activation='relu', input_shape=(SEQUENCE_LENGTH, N_FEATURES)),
        BatchNormalization(),
        MaxPooling1D(pool_size=2),
        
        Conv1D(filters=128, kernel_size=3, activation='relu'),
        BatchNormalization(),
        MaxPooling1D(pool_size=2),
        
        # 🎯 LSTM pour capturer les dependances temporelles
        LSTM(128, return_sequences=True),
        Dropout(0.3),
        
        LSTM(64),
        Dropout(0.3),
        
        # 🎯 couches denses pour la classification
        Dense(64, activation='relu'),
        BatchNormalization(),
        Dropout(0.3),
        
        Dense(N_CLASSES, activation='softmax')
    ])
    
    model.compile(
        optimizer='adam',
        loss='sparse_categorical_crossentropy',
        metrics=['accuracy']
    )
    
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
    converter = tf.lite.TFLiteConverter.from_keras_model(model)
    tflite_model = converter.convert()
    
    # 💾 sauvegarde du modele TFLite
    with open(os.path.join(DATA_DIR, 'stress_detector_model.tflite'), 'wb') as f:
        f.write(tflite_model)
    
    print("✅ terminé!")

if __name__ == "__main__":
    main() 