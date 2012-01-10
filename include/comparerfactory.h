#pragma once
#include "comparers/regexp.h"
#include <boost/regex.hpp>

class ComparerFactory
{
    private:
	boost::regex re_;
    public:
	ComparerFactory()
	    : re_("^(.*)://(.*)$", boost::regex::perl)
	{}
	
	Comparer* Create(const std::string& str)
	{
	    boost::match_results<std::string::const_iterator> what;
	    
	    if (!boost::regex_match(str, what, re_) || what.size() < 3) {
		return NULL;
	    }
	    
	    if (!what[1].matched && !what[2].matched) {
		return NULL;
	    }
	    
	    std::string comparer(what[1].first, what[1].second);
	    if (comparer == "regexp") {
		return new Regexp(std::string(what[2].first, what[2].second));
	    }
	    return NULL;
	}
};
