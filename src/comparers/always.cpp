#include <boost/regex.hpp>
#include "comparers/always.h"

namespace FWL {
    Always::Always(const std::string& pattern)
    {}

    bool Always::operator()(const std::string& target)
    {
        return true;
    }

    Comparer *AlwaysFactory::Create(const std::string& name)
    {
        return new Always(name);
    }
}
