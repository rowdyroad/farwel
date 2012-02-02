#include "connectors/memory.h"

namespace FWL {
    Memory::Memory(const std::string& name, const JsonNode& config, FdManager& fd_manager, LogIntr log)
        : Connector(name, config, fd_manager, log)
    {}

    int Memory::Open(int fd, const std::string& path, int flags)
    {
        return 0;
    }

    int Memory::Write(int fd, const void *data, size_t size)
    {
        return size;
    }

    int Memory::Read(int fd, void *data, size_t size)
    {
        return size;
    }

    int Memory::CloseFd(int fd)
    {
        return 0;
    }

    int Memory::Unlink(const std::string& path)
    {
        return 0;
    }

    Connector *MemoryFactory::Create(const std::string& name, const JsonNode& config, FdManager& fd_manager, LogIntr log)
    {
        return new Memory(name, config, fd_manager, log);
    }
}
