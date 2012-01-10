#pragma once

#include "object.h"
#include "fdmanager.h"

class Connector
    : public Object
{
    private:
	std::string name_;
	const Real& real_;
	FdManager& fd_manager_;
    protected:
	int GetNextFd()
	{
	    return fd_manager_.GetFd();
	};
	
	const Real& GetReal() const
	{
	    return real_;
	}
    public:
	explicit Connector(const std::string& name, const Real& real, FdManager& fd_manager)
	    : name_(name)
	    , real_(real)
	    , fd_manager_(fd_manager)
	{};
	
	const std::string& Name() const
	{
	    return name_;
	}
	
	virtual int Open(const std::string& path, int flags) = 0;
	virtual size_t Write(int fd, const void* data, size_t size) = 0;
	virtual size_t Read(int fd, void* data, size_t size) = 0;
	virtual size_t Close(int fd) = 0;
	virtual size_t Unlink(const std::string& path) = 0;
};
