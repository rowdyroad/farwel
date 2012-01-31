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

        const std::string& GetKey(int fd) const
        {
            Keys::const_iterator it = keys_.find(fd);

            if (it == keys_.end()) {
                return empty_key_;
            }
            return it->second;
        }

        const JsonNode& Config() const
        {
            return config_;
        }

        Log& Logger()
        {
            return *log_;
        }

    public:
        Connector(const std::string& name, const JsonNode& config, FdManager& fd_manager, LogIntr log)
            : name_(name)
            , fd_manager_(fd_manager)
            , config_(config)
            , log_(log)
        {
            JsonNodeConstOp log_node = config.get_child_optional("log");

            if (log_node) {
                std::auto_ptr<Log> nl(new Log(*log_));
                JsonNodeConstOp    level = log_node->get_child_optional("level");
                if (level) {
                    nl->SetLogLevel(level->get_value<std::string>());
                }

                JsonNodeConstOp sinks = log_node->get_child_optional("sinks");
                if (sinks && !sinks->empty()) {
                    nl->ClearSinks();
                    BOOST_FOREACH(const JsonNode::value_type & it, *sinks)
                    {
                        nl->UseSink(it.second.get_value<std::string>());
                    }
                }
                log_.reset(nl.release());
            }
        }

        virtual ~Connector() {}

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
            keys_.insert(std::make_pair(fd, path));
            return ret;
        }

        int Close(int fd)
        {
            int r = CloseFd(fd);

            if (r < 0) {
                return -1;
            }
            fd_manager_.Release(fd, this);
            keys_.erase(fd);
            return r;
        }

        virtual int Write(int fd, const void *data, size_t size) = 0;
        virtual int Read(int fd, void *data, size_t size)        = 0;
        virtual int Unlink(const std::string& path) = 0;
        virtual int Rename(const std::string& name, const std::string& path)  { return -1;  }
        virtual int MkDir(const std::string& dir, int flags)  { return -1;  }
        virtual int RmDir(const std::string& str) { return -1;  }

        virtual struct dirent *ReadDir(DIR *dd)
        {
            Directories::iterator it = dirs_.find(*(int *)dd);

            if (it == dirs_.end()) {
                return NULL;
            }
            return it->second.Read();
        }

        void *OpenDir(const std::string& name)
        {
            Directory dir(fd_manager_.Get(this), name);

            if (!OpenDir(dir)) {
                fd_manager_.Release(dir.Fd(), this);
                return NULL;
            }
            return (void *)&(dirs_.insert(std::make_pair(dir.Fd(), dir)).first->first);
        }

        int CloseDir(DIR *dd)
        {
            Directories::iterator it = dirs_.find(*(int *)dd);

            if (it == dirs_.end()) {
                return -1;
            }

            if (CloseDir(it->second)) {
                fd_manager_.Release(it->first, this);
                dirs_.erase(it);
                return 0;
            }
            return -1;
        }

        virtual bool GetFileSize(const std::string& name, size_t& size) { return false; }

        bool GetFileSize(int fd, size_t& size)
        {
            Keys::const_iterator it = keys_.find(fd);

            if (it == keys_.end()) {
                return false;
            }
            return GetFileSize(it->second, size);
        }

        virtual blksize_t GetBlockSize() const
        {
            return 0xFFFF;
        }

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


#include "connectors/dummy.h"
#include "connectors/memory.h"
#include "connectors/db.h"
