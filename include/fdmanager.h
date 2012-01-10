#pragma once

#include <boost/detail/atomic_count.hpp>

class FdManager
{
    private:
	std::auto_ptr<boost::detail::atomic_count> fd_;
    public:
    
	FdManager(int begin = 4)
	    : fd_(new boost::detail::atomic_count(begin))
	{}
	
	int GetFd()
	{
	    return ++(*fd_);
	}
};


