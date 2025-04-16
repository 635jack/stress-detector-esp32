#include <Arduino.h>
#include <Wire.h>
#include "MAX30105.h"
#include "heartRate.h"
#include "HRVCalculator.h"

// Définition des broches I2C
#define I2C_SDA 21
#define I2C_SCL 22

// Instance du capteur MAX30102
MAX30105 particleSensor;

// Instance du calculateur HRV
HRVCalculator hrvCalculator;

// Variables pour le calcul du BPM et HRV
const int BUFFER_SIZE = 100;
uint32_t irBuffer[BUFFER_SIZE];
uint32_t redBuffer[BUFFER_SIZE];
int32_t bufferLength = BUFFER_SIZE;
int32_t spo2;
int8_t validSPO2;
int32_t heartRate;
int8_t validHeartRate;

// Variables pour le suivi du temps
unsigned long lastBeatTime = 0;
unsigned long currentTime;

void setup() {
    Serial.begin(115200);
    Serial.println("Initialisation du capteur MAX30102...");

    // Initialisation du bus I2C
    Wire.begin(I2C_SDA, I2C_SCL);

    // Initialisation du capteur
    if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) {
        Serial.println("Erreur: Capteur MAX30102 non détecté!");
        while (1);
    }

    // Configuration du capteur
    particleSensor.setup();
    particleSensor.setPulseAmplitudeRed(0x0A);
    particleSensor.setPulseAmplitudeIR(0x0A);

    Serial.println("Capteur MAX30102 initialisé avec succès!");
}

void loop() {
    // Lecture des données du capteur
    for (byte i = 0; i < bufferLength; i++) {
        while (particleSensor.available() == false)
            particleSensor.check();

        redBuffer[i] = particleSensor.getRed();
        irBuffer[i] = particleSensor.getIR();
        particleSensor.nextSample();
    }

    // Calcul du BPM et SpO2
    maxim_heart_rate_and_oxygen_saturation(irBuffer, bufferLength, redBuffer, &spo2, &validSPO2, &heartRate, &validHeartRate);

    // Calcul de la HRV
    currentTime = millis();
    if (validHeartRate && heartRate > 0) {
        unsigned long rrInterval = 60000 / heartRate; // Conversion BPM en intervalle RR (ms)
        hrvCalculator.addRRInterval(rrInterval);
        float hrv = hrvCalculator.calculateHRV();
        
        // Affichage des résultats
        Serial.print("BPM: ");
        Serial.print(heartRate);
        Serial.print(" | HRV (RMSSD): ");
        Serial.println(hrv);
    }

    if (validSPO2) {
        Serial.print("SpO2: ");
        Serial.println(spo2);
    }

    delay(1000);
}