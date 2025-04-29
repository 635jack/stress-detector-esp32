#include "StressDetector.h"
#include <Arduino.h>
#include <stdarg.h>

// 🔧 classe pour gérer les erreurs
class MicroErrorReporter : public tflite::ErrorReporter {
public:
    int Report(const char* format, va_list args) override {
        char buffer[256];
        vsnprintf(buffer, sizeof(buffer), format, args);
        Serial.println(buffer);
        return 0;
    }
};

StressDetector::StressDetector() : 
    model(nullptr),
    interpreter(nullptr),
    input_tensor(nullptr),
    output_tensor(nullptr),
    tensor_arena(nullptr),
    irBuffer(nullptr),
    redBuffer(nullptr),
    sampleCount(0),
    ir_mean(0), ir_std(1),
    red_mean(0), red_std(1)
{
    // 🎯 allocation dans PSRAM
    tensor_arena = (uint8_t*)ps_malloc(TENSOR_ARENA_SIZE);
    irBuffer = new CircularBuffer<float, SEQUENCE_LENGTH>();
    redBuffer = new CircularBuffer<float, SEQUENCE_LENGTH>();
    
    clearBuffers();
}

StressDetector::~StressDetector() {
    if (interpreter) {
        delete interpreter;
    }
    if (tensor_arena) {
        free(tensor_arena);
    }
    if (irBuffer) {
        delete irBuffer;
    }
    if (redBuffer) {
        delete redBuffer;
    }
}

bool StressDetector::begin() {
    // 🧠 initialisation tflite avec le modele integre
    model = tflite::GetModel(model_data);
    if (model->version() != TFLITE_SCHEMA_VERSION) {
        Serial.println("❌ version modele incompatible");
        return false;
    }
    
    static tflite::AllOpsResolver resolver;
    static MicroErrorReporter error_reporter;
    
    interpreter = new tflite::MicroInterpreter(
        model, resolver, tensor_arena, TENSOR_ARENA_SIZE, &error_reporter
    );
    
    if (interpreter->AllocateTensors() != kTfLiteOk) {
        Serial.println("❌ erreur allocation tenseurs");
        return false;
    }
    
    input_tensor = interpreter->input(0);
    output_tensor = interpreter->output(0);
    
    // ✅ verification dimensions
    if (input_tensor->dims->size != 3 || 
        input_tensor->dims->data[1] != SEQUENCE_LENGTH || 
        input_tensor->dims->data[2] != N_FEATURES) {
        Serial.println("❌ dimensions modele invalides");
        return false;
    }
    
    return true;
}

void StressDetector::addSample(uint32_t ir, uint32_t red) {
    // 📊 ajout aux buffers
    irBuffer->push(ir);
    redBuffer->push(red);
    
    if (sampleCount < SEQUENCE_LENGTH) {
        sampleCount++;
    }
}

bool StressDetector::predict(float* probabilities) {
    if (!isBufferFull()) {
        return false;
    }
    
    // 📈 normalisation
    normalizeBuffers();
    
    // 🔄 copie des donnees dans le tenseur d'entree
    float* input_data = input_tensor->data.f;
    for (int i = 0; i < SEQUENCE_LENGTH; i++) {
        input_data[i * N_FEATURES] = ((*irBuffer)[i] - ir_mean) / ir_std;
        input_data[i * N_FEATURES + 1] = ((*redBuffer)[i] - red_mean) / red_std;
    }
    
    // 🧠 inference
    if (interpreter->Invoke() != kTfLiteOk) {
        Serial.println("❌ erreur inference");
        return false;
    }
    
    // 📊 copie des probabilites
    float* output_data = output_tensor->data.f;
    for (int i = 0; i < 3; i++) {
        probabilities[i] = output_data[i];
    }
    
    return true;
}

void StressDetector::normalizeBuffers() {
    // 📊 calcul moyenne et ecart-type
    ir_mean = 0;
    red_mean = 0;
    
    for (int i = 0; i < SEQUENCE_LENGTH; i++) {
        ir_mean += (*irBuffer)[i];
        red_mean += (*redBuffer)[i];
    }
    
    ir_mean /= SEQUENCE_LENGTH;
    red_mean /= SEQUENCE_LENGTH;
    
    ir_std = 0;
    red_std = 0;
    
    for (int i = 0; i < SEQUENCE_LENGTH; i++) {
        float ir_diff = (*irBuffer)[i] - ir_mean;
        float red_diff = (*redBuffer)[i] - red_mean;
        ir_std += ir_diff * ir_diff;
        red_std += red_diff * red_diff;
    }
    
    ir_std = sqrt(ir_std / SEQUENCE_LENGTH);
    red_std = sqrt(red_std / SEQUENCE_LENGTH);
    
    // 🔧 eviter division par zero
    if (ir_std < 1e-6) ir_std = 1;
    if (red_std < 1e-6) red_std = 1;
}

void StressDetector::clearBuffers() {
    irBuffer->clear();
    redBuffer->clear();
    sampleCount = 0;
} 