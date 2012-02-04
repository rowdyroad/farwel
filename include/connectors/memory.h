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
    int Rename(const std::string& name, const std::string& newname);
    
    int MkDir(const std::string& name, mode_t mode);
    int RmDir(const std::string& name);
    bool GetFileSize(const std::string& name, size_t& size);
    bool OpenDir(Directory& dir);
    bool CloseDir(Directory& dir);
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
