#pragma once
#include <boost/regex.hpp>
#include "comparer.h"

namespace FWL {
    class Always
        : public Comparer
    {
        public:
            Always(const std::string& pattern);
            bool operator()(const std::string& target);
    };


    class AlwaysFactory
        : public ComparerFactory
    {
        Comparer *Create(const std::string& name);
    };
}
