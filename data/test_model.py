#!/usr/bin/env python3
import numpy as np
import tensorflow as tf
import os

# 🎯 parametres
SEQUENCE_LENGTH = 1500
N_FEATURES = 2
N_CLASSES = 3

# 📝 chemins des fichiers
DATA_DIR = os.path.dirname(os.path.abspath(__file__))
MODEL_PATH = os.path.join(DATA_DIR, 'model.tflite')

def load_test_data():
    """charge des donnees de test"""
    # 📊 generation de donnees aleatoires
    X_test = np.random.randn(1, SEQUENCE_LENGTH, N_FEATURES)
    return X_test

def test_model():
    """teste le modele TFLite"""
    print("🔍 test du modele TFLite...")
    
    # 📝 chargement du modele
    if not os.path.exists(MODEL_PATH):
        print(f"❌ modele non trouve: {MODEL_PATH}")
        return
    
    print(f"📦 taille du modele: {os.path.getsize(MODEL_PATH)} octets")
    
    # 🧠 initialisation interpreteur
    interpreter = tf.lite.Interpreter(model_path=MODEL_PATH)
    interpreter.allocate_tensors()
    
    # 📊 details du modele
    input_details = interpreter.get_input_details()
    output_details = interpreter.get_output_details()
    
    print("\n📊 details du modele:")
    print(f"entree: {input_details[0]['shape']}")
    print(f"sortie: {output_details[0]['shape']}")
    
    # 📝 preparation des donnees de test
    X_test = load_test_data()
    
    # 🎯 inference
    interpreter.set_tensor(input_details[0]['index'], X_test.astype(np.float32))
    interpreter.invoke()
    
    # 📊 resultats
    output_data = interpreter.get_tensor(output_details[0]['index'])
    probabilities = output_data[0]
    
    print("\n📊 resultats de l'inference:")
    for i, prob in enumerate(probabilities):
        print(f"classe {i}: {prob:.4f}")
    
    print("\n✅ test termine!")

if __name__ == "__main__":
    test_model() 