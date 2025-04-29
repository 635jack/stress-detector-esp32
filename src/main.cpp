#include <Arduino.h>
#include <Wire.h>
#include "MAX30105.h"
#include <CircularBuffer.h>
#include <SPIFFS.h>
#include "StressDetector.h"
#include <esp_heap_caps.h>

MAX30105 particleSensor;
StressDetector stressDetector;

// 🎯 parametres pour l'analyse
#define SAMPLING_RATE 100 // hz
#define FINGER_PRESENT_THRESHOLD 10000 // seuil pour detecter le doigt

// 📊 variables d'etat
bool isRecording = false;
unsigned long lastSampleTime = 0;

// 📝 etats de stress
const char* STATES[] = {"repos", "stress", "exercice"};

// 🔍 fonction pour scanner le bus i2c
void scanI2C() {
  Serial.println("🔍 scan du bus i2c...");
  for (byte address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    byte error = Wire.endTransmission();
    
    if (error == 0) {
      Serial.print("✅ peripherique i2c trouve a l'adresse 0x");
      if (address < 16) {
        Serial.print("0");
      }
      Serial.println(address, HEX);
    }
  }
  Serial.println("🔍 scan termine");
}

void setup() {
  Serial.begin(115200);
  delay(5000); // ⏳ delai plus long pour la stabilisation
  
  Serial.println("🚀 initialisation...");
  
  // 🎯 init PSRAM
  if (!psramFound()) {
    Serial.println("❌ PSRAM non trouvee");
    while(1);
  }
  Serial.println("✅ PSRAM trouvee");
  
  // 🔌 config i2c
  Wire.begin();
  Wire.setClock(400000);
  delay(1000); // ⏳ delai pour la stabilisation i2c
  
  // 🔍 scan du bus i2c
  scanI2C();
  
  // 🔍 init capteur
  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) {
    Serial.println("❌ max30102 non trouve");
    while (1);
  }
  
  Serial.println("✅ max30102 trouve !");
  
  // ⚙️ config optimisee
  particleSensor.setup();
  particleSensor.setPulseAmplitudeRed(0x2F);
  particleSensor.setPulseAmplitudeGreen(0);
  particleSensor.setPulseAmplitudeIR(0x2F);
  particleSensor.setPulseWidth(411);
  particleSensor.setSampleRate(SAMPLING_RATE);
  particleSensor.setFIFOAverage(16);
  particleSensor.enableDIETEMPRDY();
  
  // 🧠 init detecteur de stress
  if (!stressDetector.begin()) {
    Serial.println("❌ erreur initialisation detecteur");
    while (1);
  }
  
  Serial.println("✅ detecteur initialise !");
  Serial.println("⏳ placez votre doigt sur le capteur...");
}

void loop() {
  uint32_t irValue = particleSensor.getIR();
  uint32_t redValue = particleSensor.getRed();
  
  // 🔍 verification presence du doigt
  if (irValue > FINGER_PRESENT_THRESHOLD) {
    if (!isRecording) {
      Serial.println("👆 doigt detecte - debut de l'analyse");
      isRecording = true;
      lastSampleTime = millis();
    }
    
    // ⏱️ echantillonnage a 100Hz
    unsigned long currentTime = millis();
    if (currentTime - lastSampleTime >= (1000 / SAMPLING_RATE)) {
      lastSampleTime = currentTime;
      
      // 📊 ajout des donnees
      stressDetector.addSample(irValue, redValue);
      
      // 🎯 prediction si buffer plein
      if (stressDetector.isBufferFull()) {
        float probabilities[3];
        if (stressDetector.predict(probabilities)) {
          // 📊 affichage resultats
          Serial.println("\n📊 resultats de l'analyse:");
          for (int i = 0; i < 3; i++) {
            Serial.print(STATES[i]);
            Serial.print(": ");
            Serial.print(probabilities[i] * 100);
            Serial.println("%");
          }
          
          // 🎯 etat le plus probable
          int maxIndex = 0;
          for (int i = 1; i < 3; i++) {
            if (probabilities[i] > probabilities[maxIndex]) {
              maxIndex = i;
            }
          }
          
          Serial.print("\n🎯 etat detecte: ");
          Serial.println(STATES[maxIndex]);
          Serial.println("\n⏳ placez votre doigt sur le capteur pour une nouvelle analyse...");
          
          isRecording = false;
        }
      } else {
        // 📈 affichage progression
        if (stressDetector.getSampleCount() % 100 == 0) {
          float progress = (float)stressDetector.getSampleCount() / SEQUENCE_LENGTH * 100;
          Serial.print("📈 progression: ");
          Serial.print(progress);
          Serial.println("%");
        }
      }
    }
  } else if (isRecording) {
    Serial.println("❌ doigt retire - analyse interrompue");
    isRecording = false;
    Serial.println("\n⏳ placez votre doigt sur le capteur pour une nouvelle analyse...");
  }
  
  delay(10);
}