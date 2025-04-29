import numpy as np
import tensorflow as tf
import time
import os

# 🎯 chemins des fichiers
DATA_DIR = "/Users/kojack/Documents/PlatformIO/Projects/stress-detector-esp32/data"
MODEL_PATH = os.path.join(DATA_DIR, "model/model.tflite")

class MockESP32:
    def __init__(self):
        print("🔄 initialisation du mock ESP32...")
        
        # 🧠 chargement du modele
        self.interpreter = tf.lite.Interpreter(model_path=MODEL_PATH)
        self.interpreter.allocate_tensors()
        
        # 📊 recuperation des details
        self.input_details = self.interpreter.get_input_details()
        self.output_details = self.interpreter.get_output_details()
        
        # 📈 buffers pour les donnees
        self.ir_buffer = []
        self.red_buffer = []
        self.sample_count = 0
        
        # 📊 normalisation
        self.ir_mean = 0
        self.ir_std = 1
        self.red_mean = 0
        self.red_std = 1
        
        print("✅ mock ESP32 initialise")
    
    def add_sample(self, ir, red):
        """ajoute un echantillon aux buffers"""
        self.ir_buffer.append(ir)
        self.red_buffer.append(red)
        self.sample_count += 1
        
        # 🔄 garde seulement les derniers 1500 echantillons
        if len(self.ir_buffer) > 1500:
            self.ir_buffer.pop(0)
            self.red_buffer.pop(0)
    
    def is_buffer_full(self):
        """verifie si le buffer est plein"""
        return self.sample_count >= 1500
    
    def normalize_buffers(self):
        """normalise les donnees"""
        # 📊 calcul moyenne
        self.ir_mean = np.mean(self.ir_buffer)
        self.red_mean = np.mean(self.red_buffer)
        
        # 📊 calcul ecart-type
        self.ir_std = np.std(self.ir_buffer)
        self.red_std = np.std(self.red_buffer)
        
        # 🔧 eviter division par zero
        if self.ir_std < 1e-6: self.ir_std = 1
        if self.red_std < 1e-6: self.red_std = 1
    
    def predict(self):
        """fait une prediction"""
        if not self.is_buffer_full():
            print("❌ buffer pas plein")
            return None
        
        # 📈 normalisation
        self.normalize_buffers()
        
        # 🔄 preparation des donnees
        input_data = np.zeros((1, 1500 * 2), dtype=np.float32)
        for i in range(1500):
            input_data[0, i * 2] = (self.ir_buffer[i] - self.ir_mean) / self.ir_std
            input_data[0, i * 2 + 1] = (self.red_buffer[i] - self.red_mean) / self.red_std
        
        # 🧠 inference
        self.interpreter.set_tensor(self.input_details[0]['index'], input_data)
        self.interpreter.invoke()
        
        # 📊 recuperation des probabilites
        probabilities = self.interpreter.get_tensor(self.output_details[0]['index'])
        return probabilities[0]

def generate_mock_data(n_samples=1500):
    """genere des donnees de test"""
    # 📊 generation de donnees IR
    ir_data = np.random.normal(1000, 100, n_samples)
    ir_data = np.clip(ir_data, 0, 2000)
    
    # 📊 generation de donnees RED
    red_data = np.random.normal(800, 80, n_samples)
    red_data = np.clip(red_data, 0, 1600)
    
    return ir_data, red_data

def main():
    # 🎯 creation du mock ESP32
    esp32 = MockESP32()
    
    # 📊 generation de donnees de test
    print("\n🔄 generation des donnees de test...")
    ir_data, red_data = generate_mock_data()
    
    # 📈 simulation de l'acquisition
    print("\n📈 simulation de l'acquisition...")
    for i in range(len(ir_data)):
        esp32.add_sample(ir_data[i], red_data[i])
        if (i + 1) % 100 == 0:
            print(f"📊 echantillons acquis: {i + 1}/1500")
    
    # 🎯 prediction
    print("\n🎯 prediction...")
    start_time = time.time()
    probabilities = esp32.predict()
    end_time = time.time()
    
    # 📊 affichage des resultats
    print("\n📊 resultats:")
    print(f"temps d'inference: {(end_time - start_time) * 1000:.2f} ms")
    print(f"classe 0: {probabilities[0]:.2%}")
    print(f"classe 1: {probabilities[1]:.2%}")
    print(f"classe 2: {probabilities[2]:.2%}")
    print(f"classe predite: {np.argmax(probabilities)}")

if __name__ == "__main__":
    main() 