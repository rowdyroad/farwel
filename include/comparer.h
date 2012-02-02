#pragma once
#include <boost/intrusive_ptr.hpp>
#include "object.h"
namespace FWL {
    class Comparer
        : public Object
    {
        public:
            static bool Parse(const std::string& str, std::pair<std::string, std::string>& ret);
            virtual bool operator()(const std::string& target) = 0;
    };

    typedef boost::intrusive_ptr<Comparer>   ComparerIntr;

    class ComparerFactory
        : public Object
    {
        public:
            virtual Comparer *Create(const std::string& str) = 0;
    };

    typedef boost::intrusive_ptr<ComparerFactory>   ComparerFactoryIntr;
};

#include "comparers/regexp.h"
#include "comparers/always.h"
