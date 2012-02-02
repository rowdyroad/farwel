#include <boost/regex.h>
#include "comparer.h"
namespace FWL {
    bool Comparer::Parse(const std::string& str, std::pair<std::string, std::string>& ret)
    {
        static boost::regex re_("^(.*)://(.*)$", boost::regex::perl);
        boost::match_results<std::string::const_iterator> what;

        if (!boost::regex_match(str, what, re_) || (what.size() < 3)) {
            return false;
        }

        if (!what[1].matched && !what[2].matched) {
            return false;
        }
        ret.first  = std::string(what[1].first, what[1].second);
        ret.second = std::string(what[2].first, what[2].second);
        return true;
    }
};
