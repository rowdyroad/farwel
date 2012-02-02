#pragma once

extern "C" {
#include <dirent.h>
}

#include <vector>
#include <boost/optional.hpp>
#include "log.h"
#include "object.h"
#include "fdmanager.h"
#include "json.h"
#include "directory.h"
namespace FWL {
    class Connector
        : public Object
    {
        private:
            std::string name_;
            FdManager&  fd_manager_;
            typedef boost::unordered_map<int, std::string>   Keys;
            typedef boost::unordered_map<int, Directory>     Directories;
            Keys            keys_;
            Directories     dirs_;
            std::string     empty_key_;
            const JsonNode& config_;
            LogIntr         log_;
        protected:

            const std::string& GetKey(int fd) const;
            const JsonNode& Config() const { return config_; }
            Log& Logger() { return *log_; }

        public:
            Connector(const std::string& name, const JsonNode& config, FdManager& fd_manager, LogIntr log);
            const std::string& Name() const { return name_; }
            int Open(const std::string& path, int flags);
            int Close(int fd);
            virtual int Write(int fd, const void *data, size_t size) = 0;
            virtual int Read(int fd, void *data, size_t size)        = 0;
            virtual int Unlink(const std::string& path) = 0;
            virtual int Rename(const std::string& name, const std::string& path)  { return -1;  }
            virtual int MkDir(const std::string& dir, int flags)  { return -1;  }
            virtual int RmDir(const std::string& str) { return -1;  }
            virtual struct dirent *ReadDir(DIR *dd);
            void *OpenDir(const std::string& name);
            int CloseDir(DIR *dd);
            virtual bool GetFileSize(const std::string& name, size_t& size) { return false; }

            bool GetFileSize(int fd, size_t& size);

            virtual blksize_t GetBlockSize() const { return 0xFFFF; }
        protected:
            virtual int Open(int fd, const std::string& path, int flags) = 0;
            virtual int CloseFd(int fd) = 0;
            virtual bool OpenDir(Directory& dir)  { return false;  }
            virtual bool CloseDir(Directory& dir)  { return -1;  }
    };

    typedef boost::intrusive_ptr<Connector>   ConnectorIntr;

    class ConnectorFactory
        : public Object
    {
        public:
            virtual Connector *Create(const std::string& name, const JsonNode& node, FdManager& fd_manager, LogIntr log) = 0;
    };

    typedef boost::intrusive_ptr<ConnectorFactory>   ConnectorFactoryIntr;
}

#include "connectors/dummy.h"
#include "connectors/memory.h"
#include "connectors/db.h"
