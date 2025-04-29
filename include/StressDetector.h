#pragma once

#include <Arduino.h>
#include <TensorFlowLite_ESP32.h>
#include "tensorflow/lite/micro/all_ops_resolver.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/schema/schema_generated.h"
#include <CircularBuffer.h>
#include <stdint.h>
#include "model_data.h"  // 🎯 modele integre

// 🎯 parametres du modele
#define SEQUENCE_LENGTH 1500
#define N_FEATURES 2
#define TENSOR_ARENA_SIZE (200 * 1024)

class StressDetector {
public:
    StressDetector();
    ~StressDetector();
    
    // 🔄 initialisation
    bool begin();
    
    // 📊 ajout de donnees
    void addSample(uint32_t ir, uint32_t red);
    
    // 🎯 prediction
    bool predict(float* probabilities);
    
    // 📝 getters
    bool isBufferFull() const { return sampleCount >= SEQUENCE_LENGTH; }
    int getSampleCount() const { return sampleCount; }
    
private:
    // 🧠 modele tflite
    const tflite::Model* model;
    tflite::MicroInterpreter* interpreter;
    TfLiteTensor* input_tensor;
    TfLiteTensor* output_tensor;
    
    // 🎯 buffer pour l'inference (dans PSRAM)
    uint8_t* tensor_arena;
    
    // 📊 buffers pour les donnees (dans PSRAM)
    CircularBuffer<float, SEQUENCE_LENGTH>* irBuffer;
    CircularBuffer<float, SEQUENCE_LENGTH>* redBuffer;
    int sampleCount;
    
    // 📈 normalisation
    float ir_mean, ir_std;
    float red_mean, red_std;
    
    // 🔧 fonctions utilitaires
    void normalizeBuffers();
    void clearBuffers();
}; 