#pragma once

#include <vector>
#include "connector.h"
#include "log.h"

namespace FWL {
    class Memory
        : public Connector
    {
        public:
            Memory(const std::string& name, const JsonNode& config, FdManager& fd_manager, LogIntr log);
            int Open(int fd, const std::string& path, int flags);
            int Write(int fd, const void *data, size_t size);
            int Read(int fd, void *data, size_t size);
            int CloseFd(int fd);
            int Unlink(const std::string& path);
    };

    class MemoryFactory
        : public ConnectorFactory
    {
        public:
            Connector *Create(const std::string& name, const JsonNode& config, FdManager& fd_manager, LogIntr log);
    };
}
