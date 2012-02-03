#include <boost/detail/atomic_count.hpp>
extern "C" {
#include <sys/types.h>
#include <sys/sysctl.h>
}
#include "fdmanager.h"

namespace FWL {
    int getBeginFd()
    {
#ifdef __linux__
        int name[] = { CTL_FS, FS_MAXFILE };
#else
        int name[] = { CTL_KERN, KERN_MAXFILES };
#endif
        int    data = 0;
        size_t len  = sizeof(data);

        ::sysctl(name, 2, &data, &len, NULL, 0);
        return data;
    }

    FdManager::FdManager()
        : begin_(getBeginFd())
        , fd_(begin_)
    {}
    
    int FdManager::Begin() const
    {
	return begin_;
    }

    Connector *FdManager::GetConnector(int fd)
    {
        if (fd < begin_) {
            return NULL;
        }
        Connectors::const_iterator it = connectors_.find(fd);
        if (it != connectors_.end()) {
            return it->second;
        }
        return NULL;
    }

    int FdManager::Get(Connector *connector)
    {
        int fd = ++fd_;

        connectors_.insert(std::make_pair(fd, connector));
        return fd;
    }

    bool FdManager::Release(int fd, Connector *connector)
    {
        Connectors::const_iterator it = connectors_.find(fd);

        if (it != connectors_.end()) {
            if (it->second == connector) {
                connectors_.erase(it);
                return true;
            }
        }
        return false;
    }
}
