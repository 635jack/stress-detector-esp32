#include "HRVCalculator.h"

HRVCalculator::HRVCalculator() {
    rrIntervals.reserve(MAX_INTERVALS);
}

void HRVCalculator::addRRInterval(uint32_t rrInterval) {
    if (rrIntervals.size() >= MAX_INTERVALS) {
        rrIntervals.erase(rrIntervals.begin());
    }
    rrIntervals.push_back(rrInterval);
}

float HRVCalculator::calculateHRV() {
    if (rrIntervals.size() < 2) {
        return 0.0f;
    }
    return calculateRMSSD();
}

void HRVCalculator::reset() {
    rrIntervals.clear();
}

float HRVCalculator::calculateRMSSD() {
    float sumSquaredDiffs = 0.0f;
    int count = 0;
    
    for (size_t i = 1; i < rrIntervals.size(); i++) {
        float diff = rrIntervals[i] - rrIntervals[i-1];
        sumSquaredDiffs += diff * diff;
        count++;
    }
    
    if (count == 0) {
        return 0.0f;
    }
    
    return sqrt(sumSquaredDiffs / count);
} 