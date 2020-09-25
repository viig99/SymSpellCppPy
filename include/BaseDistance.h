//
// Created by vigi99 on 25/09/20.
//

#pragma once

#include "iostream"


class BaseDistance {
public:
    virtual double Distance(std::string string1, std::string string2) = 0;

    virtual double Distance(std::string string1, std::string string2, double maxDistance) = 0;
};