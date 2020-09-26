//
// Created by vigi99 on 25/09/20.
//

#pragma once

#include <utility>

#include "DamerauOSA.h"
#include "Levenshtein.h"
#include "memory"

enum DistanceAlgorithm {
    LevenshteinDistance,
    DamerauOSADistance
};

class EditDistance {
private:
    DistanceAlgorithm mAlgorithm;
    BaseDistance* distanceComparer;

public:
    explicit EditDistance(DistanceAlgorithm algorithm): mAlgorithm(algorithm) {
        switch (algorithm) {
            case DistanceAlgorithm::DamerauOSADistance:
                distanceComparer = dynamic_cast<BaseDistance*>(std::make_unique<DamerauOSA>().get());
                break;
            case DistanceAlgorithm::LevenshteinDistance:
                distanceComparer = dynamic_cast<BaseDistance*>(std::make_unique<Levenshtein>().get());
                break;
            default:
                throw std::invalid_argument("Unknown distance algorithm.");
        }
    }

    int Compare(std::string string1, std::string string2, int maxDistance) {
        return (int) distanceComparer->Distance(std::move(string1), std::move(string2), maxDistance);
    }
};
