//
// Created by vigi99 on 25/09/20.
//

#pragma once

#include <utility>

#include "DamerauOSA.h"
#include "Levenshtein.h"
#include "Defines.h"
#include "memory"

enum DistanceAlgorithm {
    LevenshteinDistance,
    DamerauOSADistance
};

class EditDistance {
private:
    BaseDistance *distanceComparer;
    DamerauOSA damerauOSADistance;
    Levenshtein levenshteinDistance;

public:
    explicit EditDistance(DistanceAlgorithm algorithm) {
        switch (algorithm) {
            case DistanceAlgorithm::DamerauOSADistance:
                this->distanceComparer = &damerauOSADistance;
                break;
            case DistanceAlgorithm::LevenshteinDistance:
                this->distanceComparer = &levenshteinDistance;
                break;
            default:
                throw std::invalid_argument("Unknown distance algorithm.");
        }
    }

    int Compare(const xstring &string1, const xstring &string2, double maxDistance) {
        return (int) this->distanceComparer->Distance(string1, string2, maxDistance);
    }
};
