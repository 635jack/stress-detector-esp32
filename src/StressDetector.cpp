#include "StressDetector.h"
#include <Arduino.h>
#include <stdarg.h>
#include <SPIFFS.h>

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
    sampleCount(0),
    ir_mean(0), ir_std(1),
    red_mean(0), red_std(1),
    initialized(false),
    memoryMutex(xSemaphoreCreateMutex())  // initialisation directe
{
    // verification que le mutex est bien cree
    if (memoryMutex == nullptr) {
        Serial.println("❌ erreur creation mutex");
        return;
    }
    clearBuffers();
}

StressDetector::~StressDetector() {
    if (interpreter) {
        delete interpreter;
    }
    if (memoryMutex) {
        vSemaphoreDelete(memoryMutex);
    }
    if (tensor_arena) {
        free(tensor_arena);
    }
}

bool StressDetector::begin() {
    // 🔒 verification du mutex
    if (memoryMutex == nullptr) {
        Serial.println("❌ mutex non initialise");
        return false;
    }

    // 🧠 verification PSRAM
    #ifdef BOARD_HAS_PSRAM
    if (!psramFound()) {
        Serial.println("❌ PSRAM non detectee");
        return false;
    }
    
    // 📊 affichage info memoire
    Serial.printf("📊 PSRAM totale: %d octets\n", ESP.getFreePsram());
    Serial.printf("📊 Heap totale: %d octets\n", ESP.getFreeHeap());
    
    // 🧠 allocation du buffer des tenseurs sur le core 0
    if (xPortGetCoreID() != 0) {
        Serial.println("❌ l'allocation doit etre faite sur le core 0");
        return false;
    }
    
    tensor_arena = (uint8_t*)ps_malloc(TENSOR_ARENA_SIZE);
    if (!tensor_arena) {
        Serial.printf("❌ Erreur allocation PSRAM pour tensor_arena (%d octets)\n", TENSOR_ARENA_SIZE);
        return false;
    }
    Serial.printf("✅ tensor_arena alloue en PSRAM: %d octets\n", TENSOR_ARENA_SIZE);
    #endif

    // 📝 chargement du modele depuis SPIFFS
    if (!SPIFFS.begin(true)) {
        Serial.println("❌ erreur SPIFFS");
        return false;
    }
    
    File modelFile = SPIFFS.open("/model.tflite", "r");
    if (!modelFile) {
        Serial.println("❌ erreur ouverture modele");
        return false;
    }
    
    size_t modelSize = modelFile.size();
    Serial.printf("📦 taille du modele: %d octets\n", modelSize);
    
    // 🧠 allocation du modele en PSRAM
    uint8_t* modelBuffer = nullptr;
    #ifdef BOARD_HAS_PSRAM
        modelBuffer = (uint8_t*)ps_malloc(modelSize);
        if (!modelBuffer) {
            Serial.printf("❌ Erreur allocation PSRAM pour modele (%d octets)\n", modelSize);
            return false;
        }
        Serial.printf("✅ modele alloue en PSRAM: %d octets\n", modelSize);
    #else
        modelBuffer = (uint8_t*)malloc(modelSize);
        if (!modelBuffer) {
            Serial.println("❌ erreur allocation memoire modele");
            return false;
        }
    #endif
    
    modelFile.read(modelBuffer, modelSize);
    modelFile.close();
    
    // 🧠 initialisation tflite
    model = tflite::GetModel(modelBuffer);
    if (model == nullptr) {
        Serial.println("❌ erreur chargement modele");
        return false;
    }
    
    #if DEBUG_POINTERS
    if (!model) {
        Serial.println("❌ ERREUR : modele NULL");
        return false;
    }
    #endif
    
    #if DEBUG_MODEL_VERSION
    Serial.printf("📊 version du modele: %d\n", model->version());
    Serial.printf("📊 version attendue: %d\n", TFLITE_SCHEMA_VERSION);
    #endif
    
    // On ignore la vérification de version et on continue
    Serial.println("⚠️ version du modele differente, on continue quand meme...");
    
    static tflite::AllOpsResolver resolver;
    static MicroErrorReporter error_reporter;
    
    // Configuration plus simple de l'interpréteur
    Serial.println("🔧 Creation de l'interpreteur TFLite...");
    
    // Vérification de la mémoire disponible
    #ifdef BOARD_HAS_PSRAM
    size_t free_psram = ESP.getFreePsram();
    Serial.printf("📊 PSRAM disponible: %d octets\n", free_psram);
    if (free_psram < TENSOR_ARENA_SIZE) {
        Serial.println("❌ PSRAM insuffisante");
        return false;
    }
    #endif
    
    interpreter = new tflite::MicroInterpreter(
        model, resolver, tensor_arena, TENSOR_ARENA_SIZE, &error_reporter
    );
    
    #if DEBUG_POINTERS
    if (!interpreter) {
        Serial.println("❌ ERREUR : interpreteur NULL");
        return false;
    }
    #endif
    
    Serial.println("🔧 Allocation des tenseurs...");
    
    #if DEBUG_TENSOR_ALLOCATION
    Serial.println("📝 Avant AllocateTensors()");
    #endif
    
    TfLiteStatus alloc_status = interpreter->AllocateTensors();
    
    #if DEBUG_TENSOR_ALLOCATION
    Serial.println("📝 Apres AllocateTensors()");
    #endif
    
    if (alloc_status != kTfLiteOk) {
        error_reporter.Report("%s", "❌ erreur: impossible d'allouer les tenseurs 😢");
        return false;
    }
    
    input_tensor = interpreter->input(0);
    output_tensor = interpreter->output(0);
    
    #if DEBUG_POINTERS
    if (!input_tensor || !output_tensor) {
        Serial.println("❌ ERREUR : tenseurs NULL");
        return false;
    }
    #endif
    
    // Vérification de la forme d'entrée
    if (input_tensor->dims->size != 2 || 
        input_tensor->dims->data[0] != 1 || 
        input_tensor->dims->data[1] != SEQUENCE_LENGTH * N_FEATURES) {
        Serial.printf("❌ forme d'entree invalide. Attendu: [1, %d], Obtenu: [%d, %d]\n", 
                     SEQUENCE_LENGTH * N_FEATURES,
                     input_tensor->dims->data[0],
                     input_tensor->dims->data[1]);
        return false;
    }
    
    Serial.println("✅ modele charge avec succes");
    initialized = true;
    return true;
}

void StressDetector::addSample(uint32_t ir, uint32_t red) {
    if (xSemaphoreTake(memoryMutex, portMAX_DELAY) == pdTRUE) {
        // 📊 ajout aux buffers
        irBuffer.push(ir);
        redBuffer.push(red);
        
        if (sampleCount < SEQUENCE_LENGTH) {
            sampleCount++;
        }
        xSemaphoreGive(memoryMutex);
    }
}

bool StressDetector::predict(float* probabilities) {
    if (!initialized) {
        Serial.println("❌ modele non initialise");
        return false;
    }
    
    if (!isBufferFull()) {
        Serial.println("❌ buffer incomplet");
        return false;
    }
    
    if (xSemaphoreTake(memoryMutex, portMAX_DELAY) != pdTRUE) {
        Serial.println("❌ erreur acquisition mutex");
        return false;
    }
    
    // 📈 normalisation
    normalizeBuffers();
    
    // 🔄 copie des donnees dans le tenseur d'entree (aplati)
    float* input_data = input_tensor->data.f;
    for (int i = 0; i < SEQUENCE_LENGTH; i++) {
        input_data[i * N_FEATURES] = (irBuffer[i] - ir_mean) / ir_std;
        input_data[i * N_FEATURES + 1] = (redBuffer[i] - red_mean) / red_std;
    }
    
    // 🧠 inference
    bool success = true;
    TfLiteStatus invoke_status = interpreter->Invoke();
    if (invoke_status != kTfLiteOk) {
        Serial.printf("❌ erreur inference: %d\n", invoke_status);
        success = false;
    } else {
        // 📊 copie des probabilites
        float* output_data = output_tensor->data.f;
        for (int i = 0; i < 3; i++) {
            probabilities[i] = output_data[i];
        }
        
        // Validation des résultats
        float sum = 0.0f;
        for (int i = 0; i < 3; i++) {
            if (isnan(probabilities[i]) || isinf(probabilities[i])) {
                Serial.printf("⚠️ Valeur incorrecte: probabilities[%d] = %f\n", i, probabilities[i]);
                probabilities[i] = 0.0f;
                success = false;
            }
            sum += probabilities[i];
        }
        
        // Normaliser si nécessaire
        if (success && (sum < 0.9f || sum > 1.1f)) {
            Serial.printf("⚠️ Somme des probabilités anormale: %f, normalisation\n", sum);
            for (int i = 0; i < 3; i++) {
                probabilities[i] /= sum;
            }
        }
    }
    
    xSemaphoreGive(memoryMutex);
    return success;
}

void StressDetector::normalizeBuffers() {
    // 📊 calcul moyenne et ecart-type
    ir_mean = 0;
    red_mean = 0;
    
    for (int i = 0; i < SEQUENCE_LENGTH; i++) {
        ir_mean += irBuffer[i];
        red_mean += redBuffer[i];
    }
    
    ir_mean /= SEQUENCE_LENGTH;
    red_mean /= SEQUENCE_LENGTH;
    
    ir_std = 0;
    red_std = 0;
    
    for (int i = 0; i < SEQUENCE_LENGTH; i++) {
        float ir_diff = irBuffer[i] - ir_mean;
        float red_diff = redBuffer[i] - red_mean;
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
    if (xSemaphoreTake(memoryMutex, portMAX_DELAY) == pdTRUE) {
        irBuffer.clear();
        redBuffer.clear();
        sampleCount = 0;
        xSemaphoreGive(memoryMutex);
    }
}