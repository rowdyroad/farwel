#pragma once

#include <vector>
#include "../connector.h"


class Dummy
    : public Connector
{
    public:
	Dummy(const std::string& name, const Real& real, FdManager& fd_manager)
	    : Connector(name, real, fd_manager)
	{}
	
	int Open(int fd, const std::string& path, int flags)
	{
	    return fd;
	}
	
	int Write(int fd, const void* data, int size)
	{
	    return size;
	}
	
	int Read(int fd, void* data, int size)
	{ 
	    return size;
	}
	
	int CloseFd(int fd)
	{ 
	    return 0;
	}
	
	int Unlink(const std::string& path)
	{
	    return 0;
	}
};

class DummyFactory
    : public ConnectorFactory
{
    public:
	Connector* Create(const std::string& name, const JsonNode& node, const Real& real, FdManager& fd_manager)
	{
	    return new Dummy(name, real, fd_manager);
	}
};


