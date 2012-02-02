#pragma once
#include <boost/regex.hpp>
#include "comparer.h"

namespace FWL {
    class Regexp
        : public Comparer
    {
        private:
            boost::regex re_;
        public:
            Regexp(const std::string& pattern);
            bool operator()(const std::string& target);
    };


    class RegexpFactory
        : public ComparerFactory
    {
        Comparer *Create(const std::string& name);
    };
}
