//
// Created by vigi99 on 25/09/20.
//

#pragma once

#include "iostream"
#include "Defines.h"


class BaseDistance {
public:
    virtual double Distance(xstring string1, xstring string2) = 0;

    virtual double Distance(xstring string1, xstring string2, double maxDistance) = 0;
};