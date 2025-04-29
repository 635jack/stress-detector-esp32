#!/usr/bin/env python3
import os
import subprocess
import shutil

# 🎯 chemins
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
PROJECT_DIR = os.path.dirname(SCRIPT_DIR)
MODEL_FILE = "stress_detector_model.tflite"
SPIFFS_DIR = os.path.join(PROJECT_DIR, "data", "model")

def main():
    # 📝 creation du dossier SPIFFS
    os.makedirs(SPIFFS_DIR, exist_ok=True)
    
    # 📝 copie du modele
    src_path = os.path.join(SCRIPT_DIR, MODEL_FILE)
    dst_path = os.path.join(SPIFFS_DIR, MODEL_FILE)
    print(f"📦 copie du modele vers {dst_path}")
    shutil.copy2(src_path, dst_path)
    
    # 📤 upload vers l'ESP32
    print("📤 upload vers SPIFFS...")
    result = subprocess.run(
        ["pio", "run", "--target", "uploadfs"],
        cwd=PROJECT_DIR,
        capture_output=True,
        text=True
    )
    
    if result.returncode == 0:
        print("✅ upload termine avec succes!")
    else:
        print("❌ erreur lors de l'upload:")
        print(result.stderr)
        exit(1)

if __name__ == "__main__":
    main() 