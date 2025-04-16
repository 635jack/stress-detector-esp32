#include <Arduino.h>
#include <Wire.h>
#include "MAX30105.h"

MAX30105 particleSensor;

void setup() {
  Serial.begin(115200);
  delay(2000); // Attendre que le port série soit prêt
  
  Serial.println("Initialisation...");
  
  // Configuration I2C pour MAX30102
  Wire.setClock(100000); // Réduire la vitesse I2C à 100kHz
  
  // Scanner les périphériques I2C
  Serial.println("Scan I2C...");
  byte error, address;
  int nDevices = 0;
  for(address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();
    if (error == 0) {
      Serial.print("Périphérique I2C trouvé à l'adresse 0x");
      if (address < 16) {
        Serial.print("0");
      }
      Serial.println(address, HEX);
      nDevices++;
    }
  }
  if (nDevices == 0) {
    Serial.println("Aucun périphérique I2C trouvé");
  }
  
  // Initialisation du MAX30102
  if (!particleSensor.begin(Wire, I2C_SPEED_STANDARD)) {
    Serial.println("MAX30102 non trouvé");
    while (1);
  }
  
  Serial.println("MAX30102 trouvé !");
  
  // Configuration du capteur
  particleSensor.setup();
  particleSensor.setPulseAmplitudeRed(0x0A);
  particleSensor.setPulseAmplitudeGreen(0);
}

void loop() {
  // Lecture des données brutes
  uint32_t red = particleSensor.getRed();
  uint32_t ir = particleSensor.getIR();
  
  // Affichage des données sur le port série
  Serial.print("R[");
  Serial.print(red);
  Serial.print("] IR[");
  Serial.print(ir);
  Serial.println("]");
  
  delay(100);
}