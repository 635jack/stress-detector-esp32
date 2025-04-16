#ifndef HRV_CALCULATOR_H
#define HRV_CALCULATOR_H

#include <Arduino.h>
#include <vector>

class HRVCalculator {
public:
    HRVCalculator();
    
    // Ajoute un intervalle RR (en ms) à l'historique
    void addRRInterval(uint32_t rrInterval);
    
    // Calcule la HRV (RMSSD) sur les derniers intervalles
    float calculateHRV();
    
    // Réinitialise l'historique
    void reset();
    
private:
    static const int MAX_INTERVALS = 100;
    std::vector<uint32_t> rrIntervals;
    
    // Calcule la différence quadratique moyenne des intervalles RR successifs
    float calculateRMSSD();
};

#endif // HRV_CALCULATOR_H 