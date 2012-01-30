#pragma once
extern "C" {
#include <sys/types.h> 
#include <regex.h>
}
#include "../comparer.h"

class Regexp
    : public Comparer
{
    private:
        regex_t re_;
    public:
        Regexp(const std::string& pattern)
        {
    	    ::rexcomp(&re_, parrent.c_str(), REG_NOSUB);
        }

        bool operator()(const std::string& target)
        {
            return ::regexec(&re_, target_.c_str(), 0, NULL, 0);
        }
        
        ~Regexp()
        {
    	    ::regfree(&re_);
        }
};


class RegexpFactory
    : public ComparerFactory
{
    Comparer *Create(const std::string& name)
    {
        return new Regexp(name);
    }
};
