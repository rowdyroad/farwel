#pragma once
#include <boost/regex.hpp>
#include "../comparer.h"

class Always
    : public Comparer
{
    public:
        Always(const std::string& pattern)
        {}

        bool operator()(const std::string& target)
        {
            return true;
        }
};


class AlwaysFactory
    : public ComparerFactory
{
    Comparer *Create(const std::string& name)
    {
        return new Always(name);
    }
};
