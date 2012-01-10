#pragma once


extern "C" {
#include <dirent.h>
}
#include <vector>
#include "../connector.h"


class RealWrapper
    : public Connector
{
    private:
	typedef boost::unordered_map<int, int> FdMap;
	FdMap fd_map_;
	typedef boost::unordered_map<void*, void*> DirFdMap;
	DirFdMap dir_fd_map_;
    public:
	RealWrapper(const std::string& name, const Real& real, FdManager& fd_manager)
	    : Connector(name, real, fd_manager)
	{}
	
	int Open(int fd, const std::string& path, int flags)
	{
	    int real_fd = GetReal().open(path.c_str(), flags);
	    if (real_fd < 0) {
		return real_fd;
	    }
	    fd_map_.insert(std::make_pair(fd, real_fd));
	    return fd;
	}
	
	int Write(int fd, const void* data, int size)
	{
	    FdMap::const_iterator it = fd_map_.find(fd);
	    if (it == fd_map_.end()) {
		return -1;
	    }
	    return GetReal().write(it->second, data, size);
	}
	
	int Read(int fd, void* data, int size)
	{ 
	    FdMap::const_iterator it = fd_map_.find(fd);
	    if (it == fd_map_.end()) {
		return -1;
	    }
	    return GetReal().read(it->second, data, size);
	}
	
	int CloseFd(int fd)
	{ 
	    FdMap::const_iterator it = fd_map_.find(fd);
	    if (it == fd_map_.end()) {
		return -1;
	    }
	    int r = GetReal().close(it->second);
	    if (r >=0) {
		fd_map_.erase(it);
	    }
	    
	    return r;
	}
	
	bool OpenDirFd(DIR* dir, const std::string& path)
	{
	    printf("OpenDirFd:%s\n", path.c_str());
	    DIR * rdir = GetReal().opendir(path.c_str());
	    if (rdir) {
	    	struct _dirdesc* ddir = reinterpret_cast<struct _dirdesc*>(dir);
	    	struct _dirdesc* drdir = reinterpret_cast<struct _dirdesc*>(rdir);
	    	int fd = dir->dd_fd;
	    	::memcpy(dir, drdir, sizeof(struct _dirdesc));
	    	dir->dd_fd = fd;
		dir_fd_map_.insert(std::make_pair(ddir, drdir));
		return true;
	    }
	    return false;
	}
	
	int CloseDirFd(DIR* dir)
	{
	    DirFdMap::const_iterator it = dir_fd_map_.find(dir);
	    if (it == dir_fd_map_.end()) {
		return -1;
	    }
	    int r =  GetReal().closedir((DIR*)it->second);
	    if (r >=0) {
		dir_fd_map_.erase(it);
	    }
	    return r;
	}
	
	int Unlink(const std::string& path)
	{
	    return GetReal().unlink(path.c_str());
	}
};

