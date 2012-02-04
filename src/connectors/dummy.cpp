#include "connectors/dummy.h"

namespace FWL {
    Dummy::Dummy(const std::string& name, const JsonNode& config, FdManager& fd_manager, LogIntr log)
        : Connector(name, config, fd_manager, log)
    {}

    int Dummy::Open(int fd, const std::string& path, int flags)
    {
        return fd;
    }

    int Dummy::Write(int fd, const void *data, size_t size)
    {
        return size;
    }

    int Dummy::Read(int fd, void *data, size_t size)
    {
        return size;
    }
    
    int Dummy::Rename(const std::string& name, const std::string& newname)
    {
	return 0;
    }
    
    int Dummy::MkDir(const std::string& name, mode_t mode)
    {
	return 0;
    }
    
    int Dummy::RmDir(const std::string& name)
    {
	return 0;
    }
    
    bool Dummy::GetFileSize(const std::string& name, size_t& size)
    {
	size = 0;
	return true;
    }
    
    bool Dummy::OpenDir(Directory& dir)
    {
	return true;
    }
    
    bool Dummy::CloseDir(Directory& dir)
    {
	return true;
    }

    int Dummy::CloseFd(int fd)
    {
        return 0;
    }

    int Dummy::Unlink(const std::string& path)
    {
        return 0;
    }

    Connector *DummyFactory::Create(const std::string& name, const JsonNode& config, FdManager& fd_manager, LogIntr log)
    {
        return new Dummy(name, config, fd_manager, log);
    }
}
