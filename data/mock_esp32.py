import numpy as np
import tensorflow as tf
import time
import os
import threading
import queue
from concurrent.futures import ThreadPoolExecutor

# 🎯 chemins des fichiers
DATA_DIR = "/Users/kojack/Documents/PlatformIO/Projects/stress-detector-esp32/data"
MODEL_PATH = os.path.join(DATA_DIR, "model/model.tflite")

class ESP32Memory:
    """Simule la memoire de l'ESP32"""
    RAM_SIZE = 520 * 1024  # 520KB RAM
    PSRAM_SIZE = 4 * 1024 * 1024  # 4MB PSRAM
    
    def __init__(self):
        self.ram_used = 0
        self.psram_used = 0
        
    def malloc(self, size, use_psram=False):
        """simule l'allocation memoire"""
        if use_psram:
            if self.psram_used + size > self.PSRAM_SIZE:
                raise MemoryError("❌ PSRAM pleine")
            self.psram_used += size
            return np.zeros(size, dtype=np.uint8)
        else:
            if self.ram_used + size > self.RAM_SIZE:
                raise MemoryError("❌ RAM pleine")
            self.ram_used += size
            return np.zeros(size, dtype=np.uint8)
    
    def free(self, size, use_psram=False):
        """simule la liberation memoire"""
        if use_psram:
            self.psram_used -= size
        else:
            self.ram_used -= size

class ESP32Core:
    """Simule un coeur de l'ESP32"""
    CLOCK_SPEED = 240_000_000  # 240MHz
    
    def __init__(self, core_id):
        self.core_id = core_id
        self.current_task = None
        self.busy = False
    
    def execute(self, task, *args):
        """simule l'execution d'une tache"""
        self.busy = True
        self.current_task = task.__name__
        
        # simulation du temps de calcul en fonction de la complexite
        if "normalize" in task.__name__:
            # operations flottantes plus lentes
            time.sleep(0.001)
        elif "predict" in task.__name__:
            # inference plus lente
            time.sleep(0.005)
        
        result = task(*args)
        self.busy = False
        self.current_task = None
        return result

class MockESP32:
    def __init__(self):
        print("🔄 initialisation du mock ESP32...")
        
        # 🧠 simulation hardware
        self.memory = ESP32Memory()
        self.core0 = ESP32Core(0)  # coeur applicatif
        self.core1 = ESP32Core(1)  # coeur temps reel
        
        # 📊 queues pour la communication inter-coeurs
        self.sample_queue = queue.Queue(maxsize=100)
        self.result_queue = queue.Queue()
        
        # 🧠 chargement du modele en PSRAM
        model_size = os.path.getsize(MODEL_PATH)
        self.model_buffer = self.memory.malloc(model_size, use_psram=True)
        print(f"📊 modele charge en PSRAM: {model_size/1024:.1f}KB")
        
        # 🧠 initialisation tflite
        self.interpreter = tf.lite.Interpreter(model_path=MODEL_PATH)
        self.interpreter.allocate_tensors()
        
        # 📊 recuperation des details
        self.input_details = self.interpreter.get_input_details()
        self.output_details = self.interpreter.get_output_details()
        
        # 📈 buffers circulaires en RAM
        self.ir_buffer = []
        self.red_buffer = []
        self.sample_count = 0
        
        # 📊 normalisation
        self.ir_mean = 0
        self.ir_std = 1
        self.red_mean = 0
        self.red_std = 1
        
        # 🔄 demarrage des threads
        self.running = True
        self.acquisition_thread = threading.Thread(target=self._acquisition_task)
        self.inference_thread = threading.Thread(target=self._inference_task)
        self.acquisition_thread.start()
        self.inference_thread.start()
        
        print("✅ mock ESP32 initialise")
    
    def _acquisition_task(self):
        """tache d'acquisition sur core1"""
        last_time = time.time()
        while self.running:
            current_time = time.time()
            if current_time - last_time >= 0.01:  # 100Hz
                if not self.sample_queue.full():
                    ir, red = self._read_sensor()
                    self.sample_queue.put((ir, red))
                last_time = current_time
            else:
                time.sleep(0.001)  # petit delai pour ne pas surcharger le CPU
    
    def _inference_task(self):
        """tache d'inference sur core0"""
        while self.running:
            try:
                ir, red = self.sample_queue.get(timeout=0.1)
                self.core0.execute(self.add_sample, ir, red)
                
                if self.is_buffer_full():
                    probabilities = self.core0.execute(self.predict)
                    if probabilities is not None:
                        self.result_queue.put(probabilities)
            except queue.Empty:
                continue
    
    def _read_sensor(self):
        """simule la lecture du MAX30102"""
        ir = np.random.normal(1000, 100)
        red = np.random.normal(800, 80)
        return np.clip(ir, 0, 2000), np.clip(red, 0, 1600)
    
    def add_sample(self, ir, red):
        """ajoute un echantillon aux buffers"""
        self.ir_buffer.append(float(ir))
        self.red_buffer.append(float(red))
        self.sample_count += 1
        
        if len(self.ir_buffer) > 1500:
            self.ir_buffer.pop(0)
            self.red_buffer.pop(0)
    
    def is_buffer_full(self):
        """verifie si le buffer est plein"""
        return self.sample_count >= 1500
    
    def normalize_buffers(self):
        """normalise les donnees (operation couteuse)"""
        # simulation des calculs flottants plus lents sur ESP32
        time.sleep(0.001)
        
        self.ir_mean = float(np.mean(self.ir_buffer))
        self.red_mean = float(np.mean(self.red_buffer))
        
        self.ir_std = float(np.std(self.ir_buffer))
        self.red_std = float(np.std(self.red_buffer))
        
        if self.ir_std < 1e-6: self.ir_std = 1
        if self.red_std < 1e-6: self.red_std = 1
    
    def predict(self):
        """fait une prediction"""
        if not self.is_buffer_full():
            return None
        
        # 📈 normalisation (sur core0)
        self.core0.execute(self.normalize_buffers)
        
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
    
    def stop(self):
        """arrete les threads"""
        self.running = False
        self.acquisition_thread.join()
        self.inference_thread.join()

def main():
    # 🎯 creation du mock ESP32
    esp32 = MockESP32()
    
    try:
        # 📈 simulation pendant 30 secondes
        print("\n📈 simulation en cours...")
        start_time = time.time()
        predictions = []
        
        while time.time() - start_time < 30:
            if not esp32.result_queue.empty():
                probabilities = esp32.result_queue.get()
                predictions.append(probabilities)
                print(f"\n📊 prediction {len(predictions)}:")
                print(f"classe 0: {probabilities[0]:.2%}")
                print(f"classe 1: {probabilities[1]:.2%}")
                print(f"classe 2: {probabilities[2]:.2%}")
                print(f"classe predite: {np.argmax(probabilities)}")
            time.sleep(0.1)
        
        # 📊 statistiques
        print("\n📊 statistiques:")
        print(f"nombre de predictions: {len(predictions)}")
        print(f"frequence moyenne: {len(predictions)/30:.1f} Hz")
        
    finally:
        esp32.stop()

if __name__ == "__main__":
    main() 