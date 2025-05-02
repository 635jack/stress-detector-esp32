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

void initDetectorTask(void* parameter) {
  Serial.println("🧠 initialisation du detecteur sur le core 0...");
  
  if (!stressDetector.begin()) {
    Serial.println("❌ erreur initialisation detecteur");
    detectorInitialized = false;
  } else {
    Serial.println("✅ detecteur initialise !");
    detectorInitialized = true;
  }
  
  xSemaphoreGive(initDoneSemaphore);
  vTaskDelete(NULL);
}

void setup() {
  Serial.begin(115200);
  
  // Configuration de la LED d'état
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW); // LED éteinte pendant l'initialisation
  
  delay(5000); // ⏳ delai plus long pour la stabilisation
  
  Serial.println("🚀 initialisation...");
  
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
  // Correction de la valeur de la largeur d'impulsion qui cause un débordement
  particleSensor.setPulseWidth(0x9F); // Au lieu de 411, utiliser la valeur hexadécimale correcte
  particleSensor.setSampleRate(SAMPLING_RATE);
  particleSensor.setFIFOAverage(16);
  particleSensor.enableDIETEMPRDY();
  
  // 🧠 init detecteur de stress sur le core 0
  initDoneSemaphore = xSemaphoreCreateBinary();
  if (initDoneSemaphore == NULL) {
    Serial.println("❌ erreur creation semaphore");
    while (1);
  }
  
  if (xPortGetCoreID() != 0) {
    Serial.println("📌 creation de la tache d'initialisation sur le core 0...");
    xTaskCreatePinnedToCore(
      initDetectorTask,
      "init_detector",
      8192,
      NULL,
      1,
      NULL,
      0  // core 0
    );
    
    // ⏳ attente de la fin de l'initialisation
    if (xSemaphoreTake(initDoneSemaphore, pdMS_TO_TICKS(10000)) != pdTRUE) {
      Serial.println("❌ timeout initialisation detecteur");
      detectorInitialized = false;
    }
  } else {
    // on est deja sur le core 0
    initDetectorTask(NULL);
  }
  
  vSemaphoreDelete(initDoneSemaphore);
  
  if (!detectorInitialized) {
    Serial.println("⚠️ Mode de fonctionnement dégradé - uniquement données brutes");
    // LED clignote rapidement pour indiquer une erreur
    for(int i=0; i<10; i++) {
      digitalWrite(LED_PIN, HIGH);
      delay(100);
      digitalWrite(LED_PIN, LOW);
      delay(100);
    }
  } else {
    Serial.println("⏳ placez votre doigt sur le capteur...");
    digitalWrite(LED_PIN, HIGH); // LED fixe pour indiquer système prêt
  }
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
      
      // Mode dégradé - uniquement affichage des valeurs brutes
      if (!detectorInitialized) {
        if (lastSampleTime % 500 == 0) {
          Serial.printf("📊 IR: %u, Red: %u\n", irValue, redValue);
        }
        return;
      }
      
      // 📊 ajout des donnees
      stressDetector.addSample(irValue, redValue);
      
      // 🎯 prediction si buffer plein
      if (stressDetector.isBufferFull()) {
        float probabilities[3] = {0};
        
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
          stressDetector.clearBuffers();
        } else {
          Serial.println("❌ erreur lors de la prediction");
          isRecording = false;
          stressDetector.clearBuffers();
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
    stressDetector.clearBuffers();
    Serial.println("\n⏳ placez votre doigt sur le capteur pour une nouvelle analyse...");
  }
  
  // Clignotement LED pour indiquer activité
  if (isRecording && millis() % 500 < 250) {
    digitalWrite(LED_PIN, HIGH);
  } else if (isRecording) {
    digitalWrite(LED_PIN, LOW);
  } else if (detectorInitialized) {
    digitalWrite(LED_PIN, HIGH);
  }
  
  delay(10);
}