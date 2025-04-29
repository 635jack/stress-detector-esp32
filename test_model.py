import numpy as np
import tensorflow as tf
import os

# 🎯 chemins des fichiers
DATA_DIR = "/Users/kojack/Documents/PlatformIO/Projects/stress-detector-esp32/data"
MODEL_PATH = os.path.join(DATA_DIR, "model/model.tflite")

def load_test_data():
    """charge les donnees de test 📝"""
    X = np.load(os.path.join(DATA_DIR, 'X_processed.npy'))
    y = np.load(os.path.join(DATA_DIR, 'y_processed.npy'))
    
    # normalisation des donnees 🔄
    mean = np.mean(X, axis=(0, 1))
    std = np.std(X, axis=(0, 1))
    X = (X - mean) / (std + 1e-8)
    
    # aplatir les donnees pour le MLP
    X = X.reshape(X.shape[0], -1)
    
    # on prend quelques echantillons pour le test
    test_indices = np.random.choice(len(X), size=5, replace=False)
    return X[test_indices], y[test_indices]

def main():
    print("🔄 chargement du modele tflite...")
    interpreter = tf.lite.Interpreter(model_path=MODEL_PATH)
    interpreter.allocate_tensors()
    
    # recuperation des details du modele 📊
    input_details = interpreter.get_input_details()
    output_details = interpreter.get_output_details()
    
    print("\n📝 details du modele:")
    print(f"entree: {input_details[0]['shape']}")
    print(f"sortie: {output_details[0]['shape']}")
    
    # chargement des donnees de test
    print("\n🔄 chargement des donnees de test...")
    X_test, y_test = load_test_data()
    
    print("\n🎯 predictions:")
    for i in range(len(X_test)):
        # preparation de l'entree
        input_data = X_test[i:i+1].astype(np.float32)
        interpreter.set_tensor(input_details[0]['index'], input_data)
        
        # inference
        interpreter.invoke()
        
        # recuperation de la prediction
        prediction = interpreter.get_tensor(output_details[0]['index'])
        predicted_class = np.argmax(prediction[0])
        true_class = y_test[i]
        
        # affichage des resultats
        print(f"\n🔍 echantillon {i+1}:")
        print(f"classe reelle: {true_class}")
        print(f"prediction: {predicted_class}")
        print(f"confiance: {prediction[0][predicted_class]:.2%}")
        print("✅" if predicted_class == true_class else "❌")

if __name__ == "__main__":
    main() 