#include <boost/regex.hpp>
#include "comparers/regexp.h"

namespace FWL {
    Regexp::Regexp(const std::string& pattern)
        : re_(pattern, boost::regex_constants::perl)
    {}

    bool Regexp::operator()(const std::string& target)
    {
        return boost::regex_search(target, re_);
    }

    Comparer *RegexpFactory::Create(const std::string& name)
    {
        return new Regexp(name);
    }
}
