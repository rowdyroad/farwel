#pragma once

#include <vector>
#include <soci/soci.h>
#include "connector.h"
#include "path.h"
namespace FWL {
    class Db
        : public Connector
    {
        private:
            std::auto_ptr<soci::session> session_;
            std::string conn_str_;
            std::string table_name_;
            std::string key_column_;
            std::string value_column_;
            std::string parent_column_;
            soci::session& Session();
            bool exists(const std::string& key);
            bool create(const std::string& key);
            bool update(const std::string& key);
            bool update(const std::string& key, const std::string& value);
            bool append(const std::string& key, const std::string& value);
            bool remove(const std::string& key);
            bool read(const std::string& key, std::string& data);
            bool readdir(const std::string& key, std::vector<std::string>& files);
            bool length(const std::string& key, size_t& size);
        public:
            Db(const std::string& name, const JsonNode& config, FdManager& fd_manager, LogIntr log);
            int Open(int fd, const std::string& path, int flags);
            int MkDir(const std::string& path, mode_t mode);
            int Write(int fd, const void *data, size_t size);
            int Read(int fd, void *data, size_t size);
            bool OpenDir(Directory& dir);
            bool CloseDir(Directory& dir);
            int CloseFd(int fd);
            bool GetFileSize(const std::string& path, size_t& size);
            int Unlink(const std::string& path);
            int RmDir(const std::string& path);
            int Rename(const std::string& name, const std::string& newname);
    };

    class DbFactory
        : public ConnectorFactory
    {
        public:
            Connector *Create(const std::string& name, const JsonNode& config, FdManager& fd_manager, LogIntr log);
    };
}
