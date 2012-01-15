#pragma once
#include <boost/regex.hpp>
#include "../comparer.h"

class Regexp
    : public Comparer
{
    private:
        boost::regex re_;
    public:
        Regexp(const std::string& pattern)
            : re_(pattern, boost::regex_constants::perl)
        {}

        bool operator()(const std::string& target)
        {
            return(boost::regex_search(target, re_));
        }
};


class RegexpFactory
    : public ComparerFactory
{
    Comparer *Create(const std::string& name)
    {
        return(new Regexp(name));
    }
};
