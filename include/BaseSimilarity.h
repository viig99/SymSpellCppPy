//
// Created by vigi99 on 25/09/20.
//

#pragma once

#include "iostream"

class BaseSimilarity {
public:
    virtual double Similarity(std::string string1, std::string string2) = 0;

    virtual double Similarity(std::string string1, std::string string2, double minSimilarity) = 0;
};