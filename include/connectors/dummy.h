#pragma once

#include <vector>
#include <libjson/libjson.h>
#include "../connector.h"

class Dummy
    : public Connector
{
    public:
	Dummy(const std::string& name, const Real& real, FdManager& fd_manager)
	    : Connector(name, real, fd_manager)
	{}
	
	int Open(const std::string& path, int flags)
	{
	    return GetNextFd();
	}
	
	size_t Write(int fd, const void* data, size_t size)
	{
	    return size;
	}
	
	size_t Read(int fd, void* data, size_t size)
	{ 
	    ::memset(data, 0, size);
	    return size;
	}
	
	size_t Close(int fd)
	{ 
	    return 0;
	}
	
	size_t Unlink(const std::string& path)
	{
	    return 0;
	}
};
