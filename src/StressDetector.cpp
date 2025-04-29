#include "StressDetector.h"
#include <Arduino.h>
#include <stdarg.h>
#include <esp_heap_caps.h>

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
    Serial.println("\n🔍 debut initialisation StressDetector");
    
    // 📊 affichage memoire disponible
    Serial.print("📊 memoire PSRAM disponible: ");
    Serial.print(ESP.getFreePsram());
    Serial.println(" bytes");
    
    // 🎯 allocation dans PSRAM
    Serial.println("\n📊 allocation des buffers...");
    
    // Allouer d'abord les buffers circulaires (plus petits)
    irBuffer = new (std::nothrow) CircularBuffer<float, SEQUENCE_LENGTH>();
    if (!irBuffer) {
        Serial.println("❌ erreur allocation irBuffer");
        return;
    }
    Serial.println("✅ irBuffer alloue");
    
    redBuffer = new (std::nothrow) CircularBuffer<float, SEQUENCE_LENGTH>();
    if (!redBuffer) {
        Serial.println("❌ erreur allocation redBuffer");
        return;
    }
    Serial.println("✅ redBuffer alloue");
    
    // Allouer ensuite tensor_arena avec MALLOC_CAP_8BIT | MALLOC_CAP_SPIRAM
    Serial.println("\n📊 allocation tensor_arena...");
    Serial.print("📊 taille requise: ");
    Serial.print(TENSOR_ARENA_SIZE);
    Serial.println(" bytes");
    
    // Utiliser les bons flags de capacité pour PSRAM
    tensor_arena = (uint8_t*)heap_caps_malloc(TENSOR_ARENA_SIZE, MALLOC_CAP_8BIT | MALLOC_CAP_SPIRAM);
    if (!tensor_arena) {
        Serial.println("❌ erreur allocation tensor_arena");
        return;
    }
    Serial.println("✅ tensor_arena alloue");
    
    // Vérifier l'état de la mémoire après allocations
    Serial.print("📊 memoire PSRAM restante: ");
    Serial.print(ESP.getFreePsram());
    Serial.println(" bytes");
    
    Serial.println("\n✅ initialisation StressDetector terminee");
    clearBuffers();
}

StressDetector::~StressDetector() {
    Serial.println("\n🔍 destruction StressDetector");
    
    if (interpreter) {
        Serial.println("📊 liberation interpreter");
        delete interpreter;
    }
    if (tensor_arena) {
        Serial.println("📊 liberation tensor_arena");
        heap_caps_free(tensor_arena);
    }
    if (irBuffer) {
        Serial.println("📊 liberation irBuffer");
        delete irBuffer;
    }
    if (redBuffer) {
        Serial.println("📊 liberation redBuffer");
        delete redBuffer;
    }
    
    Serial.println("✅ destruction terminee");
}

bool StressDetector::begin() {
    Serial.println("\n🔍 debut initialisation tflite");
    
    // ✅ verification allocations
    if (!tensor_arena || !irBuffer || !redBuffer) {
        Serial.println("❌ erreur allocation memoire");
        Serial.print("📊 tensor_arena: ");
        Serial.println(tensor_arena ? "OK" : "NULL");
        Serial.print("📊 irBuffer: ");
        Serial.println(irBuffer ? "OK" : "NULL");
        Serial.print("📊 redBuffer: ");
        Serial.println(redBuffer ? "OK" : "NULL");
        return false;
    }
    
    // 🧠 initialisation tflite avec le modele integre
    Serial.println("\n📊 initialisation tflite...");
    model = tflite::GetModel(model_data);
    if (model->version() != TFLITE_SCHEMA_VERSION) {
        Serial.println("❌ version modele incompatible");
        return false;
    }
    
    static tflite::AllOpsResolver resolver;
    static MicroErrorReporter error_reporter;
    
    Serial.println("📊 creation interpreter...");
    interpreter = new tflite::MicroInterpreter(
        model, resolver, tensor_arena, TENSOR_ARENA_SIZE, &error_reporter
    );
    
    Serial.println("📊 allocation tenseurs...");
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
    
    Serial.println("✅ initialisation tflite terminee");
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