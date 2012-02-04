#include <libmemcached/memcached.hpp>
#include <boost/foreach.hpp>
#include "connectors/dummy.h"


namespace FWL {
    Memcache::Memcache(const std::string& name, const JsonNode& config, FdManager& fd_manager, LogIntr log)
        : Connector(name, config, fd_manager, log)
        , client_(new memcache::Memcache)
    {
        BOOST_FOREACH(const JsonNode::value_type & server, config.get_child("servers"))
        {
            client_->addServer(server.second.get<std::string>("host"), server.second.get<in_port_t>("port"));
        }
    }

    int Memcache::Write(int fd, const void *data, size_t size)
    {
    /*    MemCacheClient::MemRequest request;
        request.mKey = GetKey(fd);
        request.mData.WriteBytes(data, size);
        return client_->Append(request) && request.mResult == MCERR_OK;*/
        return 0;
    }

    int Memcache::Read(int fd, void *data, size_t size)
    {
        /*MemCacheClient::MemRequest request;
        request.mKey = GetKey(fd);
        if (client_->Get(request)) {
    	    request.mData.ReadBytes(data, size);
    	    return request.mData.GetReadSize();
        }
        return -1;*/
        
        return 0;
    }

    int Memcache::Rename(const std::string& name, const std::string& newname)
    {
        
        return 0;
    }

    int Memcache::MkDir(const std::string& name, mode_t mode)
    {
        return 0;
    }

    int Memcache::RmDir(const std::string& name)
    {
        return 0;
    }

    bool Memcache::GetFileSize(const std::string& name, size_t& size)
    {
        size = 0;
        return true;
    }

    bool Memcache::OpenDir(Directory& dir)
    {
        return true;
    }

    bool Memcache::CloseDir(Directory& dir)
    {
        return true;
    }

    int Memcache::CloseFd(int fd)
    {
        return 0;
    }

    int Memcache::Unlink(const std::string& path)
    {
        return 0;
    }

    Connector *MemcacheFactory::Create(const std::string& name, const JsonNode& config, FdManager& fd_manager, LogIntr log)
    {
        return new Memcache(name, config, fd_manager, log);
    }
}
