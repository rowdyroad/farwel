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
#include "filesystem.h"
namespace FWL {
    class Connector
        : public Object
    {
        private:
            std::string name_;
            FdManager&  fd_manager_;
            typedef boost::unordered_map<int, FileIntr>   Files;
            typedef boost::unordered_map<int, DirectoryIntr>     Directories;
            typedef boost::unordered_map<std::string, NodeIntr> Nodes;

            Files           files_;
            Directories     dirs_;
            Nodes 	    nodes_;
            std::string     empty_key_;
            const JsonNode& config_;
            LogIntr         log_;
            int openFile(FileIntr& file);

        protected:
            const JsonNode& Config() const { return config_; }
            Log& Logger() { return *log_; }
            void remove(FileIntr& fd)
            {
        	files_.erase(fd->Fd());
        	nodes_.erase(fd->Name());
            }
            
            void remove(DirectoryIntr& fd)
            {
        	dirs_.erase(fd->Fd());
        	nodes_.erase(fd->Name());
            }
            
            void insert(DirectoryIntr& fd)
            {
        	dirs_.insert(std::make_pair(fd->Fd(), fd));
        	nodes_.insert(std::make_pair(fd->Name(), fd));
            }
            
            void insert(FileIntr& fd)
            {
        	files_.insert(std::make_pair(fd->Fd(), fd));
        	nodes_.insert(std::make_pair(fd->Name(), fd));
            }

        public:
            const std::string& Name() const { return name_; }
            Connector(const std::string& name, const JsonNode& config, FdManager& fd_manager, LogIntr log);
            //! -- file
            int Open(const std::string& path, int flags);
            int Write(int fd, const void *data, size_t size);
            int Read(int fd, void *data, size_t size);
            int Close(int fd);
            
            bool GetFileSize(const std::string& name, size_t& size);
            bool GetFileSize(int fd, size_t& size);
            int CloseDir(DIR *d);
            void *OpenDir(const std::string& name);
    
	    //! -- noded
	    virtual int Unlink(const std::string& path) = 0;
            virtual int Rename(const std::string& name, const std::string& path) = 0;
            virtual int MkDir(const std::string& dir, mode_t mode) = 0;
            virtual int RmDir(const std::string& str) = 0;
            virtual blksize_t GetBlockSize() const { return 0xFFFF; }
            virtual struct dirent *ReadDir(DIR *d);

        protected:

            virtual bool Open(DirectoryIntr& dir)  = 0;
            virtual bool Close(DirectoryIntr& dir) = 0;

            virtual int Write(FileIntr& file, const void* data, size_t size) = 0;
            virtual int Read(FileIntr& file, void* data, size_t size) = 0;
            virtual bool Exists(FileIntr& file)   = 0;
            virtual bool Create(FileIntr& file)   = 0;
            virtual bool Truncate(FileIntr& file) = 0;
            virtual bool Close(FileIntr& file) = 0;
            
            virtual bool GetFileSize(FileIntr& file, size_t& size) = 0;
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
