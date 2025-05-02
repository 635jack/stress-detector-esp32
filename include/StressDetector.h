#pragma once
#include <Arduino.h>
#include <TensorFlowLite_ESP32.h>
#include "tensorflow/lite/micro/all_ops_resolver.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/schema/schema_generated.h"
#include <CircularBuffer.h>
#include <stdint.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

// 🎯 parametres du modele
#define SEQUENCE_LENGTH 1500
#define N_FEATURES 2
#define TENSOR_ARENA_SIZE (48 * 1024) // augmente a 48KB pour etre sur

// 🔍 debug flags
#define DEBUG_TENSOR_ALLOCATION 1
#define DEBUG_MODEL_VERSION 1
#define DEBUG_POINTERS 1

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
    bool isInitialized() const { return initialized; }
    
    // 📈 normalisation
    void normalizeBuffers();
    void clearBuffers();
    
private:
    // 🧠 modele tflite
    const tflite::Model* model;
    tflite::MicroInterpreter* interpreter;
    TfLiteTensor* input_tensor;
    TfLiteTensor* output_tensor;
    
    // 🎯 buffer pour l'inference
    #ifdef BOARD_HAS_PSRAM
    uint8_t* tensor_arena = nullptr;
    #else
    uint8_t tensor_arena[TENSOR_ARENA_SIZE];
    #endif
    
    // 📊 buffers pour les donnees
    CircularBuffer<float, SEQUENCE_LENGTH> irBuffer;
    CircularBuffer<float, SEQUENCE_LENGTH> redBuffer;
    int sampleCount;
    
    // 📈 normalisation
    float ir_mean, ir_std;
    float red_mean, red_std;
    
    // 🔒 protection de la memoire
    SemaphoreHandle_t memoryMutex;
    
    // ✅ flag d'initialisation
    bool initialized;
};