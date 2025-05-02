# 🤖 Détecteur de Stress ESP32

## 📝 Description
Ce projet utilise un ESP32 avec un capteur MAX30102 pour détecter le stress. Le capteur mesure les signaux IR et RED qui sont utilisés pour l'analyse du stress.

## 🎯 Fonctionnalités
- Mesure des signaux IR et RED en temps réel
- Analyse du stress avec TensorFlow Lite
- Interface utilisateur avec LED d'état
- Mode dégradé en cas d'erreur

## 🛠️ Configuration Matérielle
- ESP32 Feather
- Capteur MAX30102
- Connexions :
  - VCC -> 3.3V
  - GND -> GND
  - SDA -> GPIO 23
  - SCL -> GPIO 22

## 📦 Dépendances
```ini
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino
monitor_speed = 115200
lib_deps =
    sparkfun/SparkFun MAX3010x Pulse and Proximity Sensor Library
    rlogiacco/CircularBuffer
    tensorflow/tensorflow-lite-esp32
build_flags =
    -DBOARD_HAS_PSRAM
    -mfix-esp32-psram-cache-issue
```

## 📁 Structure du Projet
```
stress-detector-esp32/
├── data/                  # Données et scripts Python
│   ├── model/            # Modèle TensorFlow Lite
│   ├── preprocess.py     # Prétraitement des données
│   └── train_model.py    # Entraînement du modèle
├── src/                  # Code source Arduino
│   ├── main.cpp         # Programme principal
│   └── StressDetector.cpp # Classe de détection
├── include/             # Headers
│   └── StressDetector.h # Définition de la classe
└── platformio.ini       # Configuration PlatformIO
```

## 🧠 Code Source Principal

### main.cpp
```cpp
#include <Arduino.h>
#include <Wire.h>
#include "MAX30105.h"
#include <CircularBuffer.h>
#include <SPIFFS.h>
#include "StressDetector.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>

// Définition de la LED pour indiquer l'état du système
#define LED_PIN 2 // LED intégrée de l'ESP32

MAX30105 particleSensor;
StressDetector stressDetector;
SemaphoreHandle_t initDoneSemaphore;

// 🎯 parametres pour l'analyse
#define SAMPLING_RATE 100 // hz
#define FINGER_PRESENT_THRESHOLD 10000 // seuil pour detecter le doigt

// 📊 variables d'etat
bool isRecording = false;
unsigned long lastSampleTime = 0;
bool detectorInitialized = false;

// 📝 etats de stress
const char* STATES[] = {"repos", "stress", "exercice"};

void setup() {
    Serial.begin(115200);
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);
    
    // 🔌 config i2c
    Wire.begin();
    Wire.setClock(400000);
    
    // 🔍 init capteur
    if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) {
        Serial.println("❌ max30102 non trouve");
        while (1);
    }
    
    // ⚙️ config optimisee
    particleSensor.setup();
    particleSensor.setPulseAmplitudeRed(0x2F);
    particleSensor.setPulseAmplitudeGreen(0);
    particleSensor.setPulseAmplitudeIR(0x2F);
    particleSensor.setPulseWidth(0x9F);
    particleSensor.setSampleRate(SAMPLING_RATE);
    particleSensor.setFIFOAverage(16);
    
    // 🧠 init detecteur
    if (!stressDetector.begin()) {
        Serial.println("⚠️ Mode de fonctionnement dégradé");
        detectorInitialized = false;
    } else {
        Serial.println("✅ detecteur initialise");
        detectorInitialized = true;
    }
}

void loop() {
    uint32_t irValue = particleSensor.getIR();
    uint32_t redValue = particleSensor.getRed();
    
    if (irValue > FINGER_PRESENT_THRESHOLD) {
        if (!isRecording) {
            Serial.println("👆 doigt detecte");
            isRecording = true;
            lastSampleTime = millis();
        }
        
        if (millis() - lastSampleTime >= (1000 / SAMPLING_RATE)) {
            lastSampleTime = millis();
            
            if (!detectorInitialized) {
                Serial.printf("📊 IR: %u, Red: %u\n", irValue, redValue);
                return;
            }
            
            stressDetector.addSample(irValue, redValue);
            
            if (stressDetector.isBufferFull()) {
                float probabilities[3] = {0};
                
                if (stressDetector.predict(probabilities)) {
                    Serial.println("\n📊 resultats:");
                    for (int i = 0; i < 3; i++) {
                        Serial.printf("%s: %.1f%%\n", STATES[i], probabilities[i] * 100);
                    }
                    
                    int maxIndex = 0;
                    for (int i = 1; i < 3; i++) {
                        if (probabilities[i] > probabilities[maxIndex]) {
                            maxIndex = i;
                        }
                    }
                    
                    Serial.printf("\n🎯 etat: %s\n", STATES[maxIndex]);
                    isRecording = false;
                    stressDetector.clearBuffers();
                }
            }
        }
    }
    
    delay(10);
}
```

### StressDetector.h
```cpp
#pragma once
#include <Arduino.h>
#include <TensorFlowLite_ESP32.h>
#include "tensorflow/lite/micro/all_ops_resolver.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/schema/schema_generated.h"
#include <CircularBuffer.h>
#include <stdint.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

// 🎯 parametres du modele
#define SEQUENCE_LENGTH 1500
#define N_FEATURES 2
#define TENSOR_ARENA_SIZE (100 * 1024) // 100KB

class StressDetector {
public:
    StressDetector();
    ~StressDetector();
    
    bool begin();
    void addSample(uint32_t ir, uint32_t red);
    bool predict(float* probabilities);
    bool isBufferFull() const { return sampleCount >= SEQUENCE_LENGTH; }
    int getSampleCount() const { return sampleCount; }
    void normalizeBuffers();
    void clearBuffers();
    
private:
    const tflite::Model* model;
    tflite::MicroInterpreter* interpreter;
    TfLiteTensor* input_tensor;
    TfLiteTensor* output_tensor;
    
    #ifdef BOARD_HAS_PSRAM
    uint8_t* tensor_arena = nullptr;
    #else
    uint8_t tensor_arena[TENSOR_ARENA_SIZE];
    #endif
    
    CircularBuffer<float, SEQUENCE_LENGTH> irBuffer;
    CircularBuffer<float, SEQUENCE_LENGTH> redBuffer;
    int sampleCount;
    
    float ir_mean, ir_std;
    float red_mean, red_std;
    
    SemaphoreHandle_t memoryMutex;
    bool initialized;
};
```

### train_model.py
```python
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
        # couche d'entree 📥
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
    
    # compilation
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
    converter = tf.lite.TFLiteConverter.from_keras_model(model)
    
    # Configuration pour ESP32
    converter.target_spec.supported_ops = [
        tf.lite.OpsSet.TFLITE_BUILTINS
    ]
    
    # Forcer la version 3 du schéma
    converter.target_spec.schema_version = 3
    
    # Utiliser DEFAULT plutôt que des optimisations plus avancées
    converter.optimizations = [tf.lite.Optimize.DEFAULT]
    
    # S'assurer d'utiliser float32 (crucial pour ESP32)
    converter.target_spec.supported_types = [tf.float32]
    converter.inference_input_type = tf.float32
    converter.inference_output_type = tf.float32
    
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

## 🚀 Installation
1. Clonez ce dépôt
2. Ouvrez le projet dans PlatformIO
3. Installez les dépendances
4. Téléversez le code sur votre ESP32

## 📊 Utilisation
1. Connectez le capteur MAX30102 à votre doigt
2. Observez les valeurs sur le moniteur série
3. Les résultats d'analyse s'afficheront automatiquement

## 🔧 Dépannage
- Si la LED clignote rapidement : erreur d'initialisation
- Si les valeurs sont à 0 : vérifiez les connexions
- Si le modèle ne charge pas : vérifiez le fichier SPIFFS

## 📈 Prochaines Étapes
- Amélioration de la précision du modèle
- Interface utilisateur graphique
- Stockage des données
- Analyse de la variabilité cardiaque 