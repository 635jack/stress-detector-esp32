#include <Arduino.h>
#include <Wire.h>
#include "MAX30105.h"
#include <CircularBuffer.h>
#include <SPIFFS.h>

MAX30105 particleSensor;

// 🎯 parametres pour l'analyse
#define BUFFER_SIZE 1500 // buffer pour 15 secondes a 100Hz
#define SAMPLING_RATE 100 // hz
#define RECORDING_TIME 15 // secondes d'enregistrement
#define FINGER_PRESENT_THRESHOLD 10000 // seuil pour detecter le doigt
#define SAMPLES_PER_STATE 15 // nombre de sessions par etat

// 📊 structures de donnees
CircularBuffer<uint32_t, BUFFER_SIZE> irBuffer;
CircularBuffer<uint32_t, BUFFER_SIZE> redBuffer;
bool isRecording = false;
unsigned long recordingStartTime = 0;
int sampleCount = 0;

// 🏷️ variables pour les etiquettes
int currentState = 0; // 0: repos, 1: stress, 2: exercice
bool waitingForState = true;
int sessionCount = 0;
int totalSamples = 0;

// 📝 variables pour la sauvegarde
String currentFileName = "";
File dataFile;

void setup() {
  Serial.begin(115200);
  delay(2000);
  
  Serial.println("🚀 initialisation...");
  
  // 🔌 config i2c
  Wire.begin();
  Wire.setClock(400000);
  
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
  particleSensor.setSampleRate(SAMPLING_RATE);
  particleSensor.setFIFOAverage(16);
  
  // 📝 initialisation SPIFFS
  if(!SPIFFS.begin(true)) {
    Serial.println("❌ erreur SPIFFS");
    while(1);
  }
  
  // 📝 creation du fichier
  currentFileName = "/data_" + String(millis()) + ".csv";
  dataFile = SPIFFS.open(currentFileName, FILE_WRITE);
  if(!dataFile) {
    Serial.println("❌ erreur creation fichier");
    while(1);
  }
  
  // 📝 en-tete CSV
  dataFile.println("ir,red,state");
  
  // 📝 menu de selection
  Serial.println("\n📊 menu de configuration:");
  Serial.println("1. repos");
  Serial.println("2. stress");
  Serial.println("3. exercice");
  Serial.println("entrez le numero (1-3):");
}

void loop() {
  // 📝 attente de la saisie de l'etat
  if (waitingForState) {
    if (Serial.available() > 0) {
      char input = Serial.read();
      if (input >= '1' && input <= '3') {
        currentState = input - '1';
        Serial.print("etat selectionne: ");
        switch(currentState) {
          case 0: Serial.println("repos"); break;
          case 1: Serial.println("stress"); break;
          case 2: Serial.println("exercice"); break;
        }
        Serial.print("session: ");
        Serial.print(sessionCount + 1);
        Serial.print("/");
        Serial.println(SAMPLES_PER_STATE);
        Serial.println("⏳ placez votre doigt sur le capteur...");
        waitingForState = false;
      }
    }
    return;
  }
  
  uint32_t irValue = particleSensor.getIR();
  uint32_t redValue = particleSensor.getRed();
  
  // 🔍 verification presence du doigt
  if (irValue > FINGER_PRESENT_THRESHOLD) {
    if (!isRecording) {
      Serial.println("👆 doigt detecte - debut de l'enregistrement");
      isRecording = true;
      recordingStartTime = millis();
      sampleCount = 0;
      irBuffer.clear();
      redBuffer.clear();
    }
    
    // 📝 enregistrement des donnees
    if (isRecording) {
      irBuffer.push(irValue);
      redBuffer.push(redValue);
      sampleCount++;
      
      // 📊 affichage progression
      if (sampleCount % 100 == 0) {
        float progress = (millis() - recordingStartTime) / (RECORDING_TIME * 1000.0) * 100;
        Serial.print("📈 progression: ");
        Serial.print(progress);
        Serial.println("%");
      }
      
      // ✅ fin de l'enregistrement
      if ((millis() - recordingStartTime) >= (RECORDING_TIME * 1000)) {
        Serial.println("✅ enregistrement termine");
        Serial.println("📊 donnees enregistrees:");
        Serial.println("ir,red,state");
        
        // 📤 envoi et sauvegarde des donnees
        for (int i = 0; i < sampleCount; i++) {
          String dataLine = String(irBuffer[i]) + "," + 
                          String(redBuffer[i]) + "," + 
                          String(currentState);
          Serial.println(dataLine);
          dataFile.println(dataLine);
        }
        dataFile.flush();
        
        isRecording = false;
        sessionCount++;
        totalSamples += sampleCount;
        
        // 📊 affichage statistiques
        Serial.print("\n📊 statistiques:\n");
        Serial.print("sessions completees: ");
        Serial.print(sessionCount);
        Serial.print("/");
        Serial.println(SAMPLES_PER_STATE);
        Serial.print("total echantillons: ");
        Serial.println(totalSamples);
        
        if (sessionCount >= SAMPLES_PER_STATE) {
          Serial.println("\n🎉 collecte terminee pour cet etat!");
          Serial.println("\n📊 menu de configuration:");
          Serial.println("1. repos");
          Serial.println("2. stress");
          Serial.println("3. exercice");
          Serial.println("entrez le numero (1-3):");
          sessionCount = 0;
          totalSamples = 0;
          waitingForState = true;
        } else {
          Serial.println("\n📊 menu de configuration:");
          Serial.println("1. repos");
          Serial.println("2. stress");
          Serial.println("3. exercice");
          Serial.println("entrez le numero (1-3):");
          waitingForState = true;
        }
      }
    }
  } else if (isRecording) {
    Serial.println("❌ doigt retire - enregistrement interrompu");
    isRecording = false;
    Serial.println("\n📊 menu de configuration:");
    Serial.println("1. repos");
    Serial.println("2. stress");
    Serial.println("3. exercice");
    Serial.println("entrez le numero (1-3):");
    waitingForState = true;
  }
  
  delay(10);
}