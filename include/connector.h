#pragma once

/*extern "C" {
#include <dirent.h>
}*/

#include "object.h"
#include "fdmanager.h"
#include "json.h"

class Connector
    : public Object
{
    private:
	std::string name_;
	const Real& real_;
	FdManager& fd_manager_;
    protected:
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
	
	virtual  ~Connector() {}
	
	const std::string& Name() const
	{
	    return name_;
	}
	
	int Open(const std::string& path, int flags)
	{
	    int fd  = fd_manager_.Get(this);
	    int ret = Open(fd, path, flags);
	    if (ret < 0) {
		fd_manager_.Release(fd, this);
	    }
	    return ret;
	}
	int Close(int fd) 
	{
	    int r = CloseFd(fd);
	    if (r >= 0) {
		fd_manager_.Release(fd, this);
	    }
	    return r;
	}
	
	virtual int Write(int fd, const void* data, int size) = 0;
	virtual int Read(int fd, void* data, int size) = 0;
	virtual int Unlink(const std::string& path) = 0;
	virtual int Rename(const std::string& name, const std::string& path)  { return -1; };
	virtual int MkDir(const std::string& dir, int flags)  { return -1; };
	virtual int RmDir(const std::string& str) { return -1; };

	virtual int ReadDir(int fd, std::string& filename) { return -1; }
	DIR* OpenDir(const std::string& name)
	{
	    DIR* fd = (DIR*)fd_manager_.GetDir(this);
	    if (!OpenDirFd(fd, name)) {
		fd_manager_.ReleaseDir(fd, this);
		return NULL;
	    }
	    return fd;
	}
	
	int CloseDir(DIR* fd)
	{
	    int r = CloseDirFd(fd);
	    if (r >= 0) {
		fd_manager_.ReleaseDir(fd, this);
	    }
	    return r;
	}
    protected:
	virtual int Open(int fd, const std::string& path, int flags) = 0;
	virtual int CloseFd(int fd) = 0;
	
	virtual bool OpenDirFd(DIR* fd, const std::string& path)  { return false; }
	virtual int CloseDirFd(DIR* fd)  { return -1; }
	    
	
};
class ConnectorFactory
    : public Object
{
    public:
	virtual Connector* Create(const std::string& name, const JsonNode& node,  const Real& real, FdManager& fd_manager) = 0;
};


#include "connectors/dummy.h"
#include "connectors/memory.h"
