#pragma once
#include <boost/unordered_map.hpp>
#include <boost/detail/atomic_count.hpp>


class Connector;

class FdManager
{
    private:
	boost::detail::atomic_count fd_;
	typedef boost::unordered_map<int, Connector*> FileConnectors;
	typedef boost::unordered_map<void*, Connector*> DirConnectors;
	
	FileConnectors file_connectors_;
	DirConnectors dir_connectors_;
	
	Connector* GetConnector(void* dir)
	{
	    DirConnectors::const_iterator it = dir_connectors_.find(dir);
	    if (it != dir_connectors_.end()) {
		return it->second;
	    }
	    return NULL;
	}
	
	Connector* GetConnector(int fd)
	{
	    FileConnectors::const_iterator it = file_connectors_.find(fd);
	    if (it != file_connectors_.end()) {
		return it->second;
	    }
	    return NULL;
	}
	friend class Main;
    public:

	FdManager(int begin = 4)
	    : fd_(begin)
	{}
	
	int Get(Connector* connector)
	{
	    int fd = ++fd_;
	    file_connectors_.insert(std::make_pair(fd, connector));
	    return fd;
	}
	
	void* GetDir(Connector* connector)
	{
	    struct _dirdesc* dir = new struct _dirdesc;
	    dir->dd_fd = ++fd_;
	    dir_connectors_.insert(std::make_pair(dir, connector));
	    return dir;
	}

	bool ReleaseDir(void* dir, Connector* connector)
	{
	    DirConnectors::const_iterator it = dir_connectors_.find(dir);
	    if (it != dir_connectors_.end()) {
		if (it->second == connector) {
		    delete reinterpret_cast<struct _dirdesc*>(dir);
		    dir_connectors_.erase(it);
		    return true;
		}
	    }
	    return false;
	}
	
	bool Release(int fd, Connector* connector)
	{
	    FileConnectors::const_iterator it = file_connectors_.find(fd);
	    if (it != file_connectors_.end()) {
		if (it->second == connector) {
		    file_connectors_.erase(it);
		    return true;
		}
	    }
	    return false;
	}
};


