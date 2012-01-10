#pragma once

#include "object.h"
class Comparer
    : public Object
{
    public:
        virtual bool operator()(const std::string& target) = 0;
        virtual ~Comparer() {};
};
