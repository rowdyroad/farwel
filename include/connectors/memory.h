#pragma once

#include <vector>
#include "../connector.h"
#include "../log.h"


class Memory
    : public Connector
{
    public:
        Memory(const std::string& name, const JsonNode& config, FdManager& fd_manager, LogIntr log)
            : Connector(name, config, fd_manager, log)
        {}

        int Open(int fd, const std::string& path, int flags)
        {
            return 0;
        }

        int Write(int fd, const void *data, size_t size)
        {
            return size;
        }

        int Read(int fd, void *data, size_t size)
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

class MemoryFactory
    : public ConnectorFactory
{
    public:
        Connector *Create(const std::string& name, const JsonNode& config, FdManager& fd_manager, LogIntr log)
        {
            return new Memory(name, config, fd_manager, log);
        }
};
