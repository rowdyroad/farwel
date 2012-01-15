#pragma once
#include <boost/unordered_map.hpp>
#include <boost/detail/atomic_count.hpp>
extern "C" {
#include <sys/types.h>
#include <sys/sysctl.h>
}

class Connector;

class FdManager
{
    private:
        size_t begin_;
        boost::detail::atomic_count fd_;
        typedef boost::unordered_map<int, Connector *>   Connectors;
        Connectors connectors_;

        Connector *GetConnector(int fd)
        {
            if (fd < begin_) {
                return(NULL);
            }
            Connectors::const_iterator it = connectors_.find(fd);
            if (it != connectors_.end()) {
                return(it->second);
            }
            return(NULL);
        }

        int getBeginFd()
        {
            int    name[] = { CTL_KERN, KERN_MAXFILES };
            int    data   = 0;
            size_t len    = sizeof(data);

            ::sysctl(name, 2, &data, &len, NULL, 0);
            return(data);
        }

        friend class Main;
    public:

        FdManager()
            : begin_(getBeginFd())
            , fd_(begin_)
        {}

        int Get(Connector *connector)
        {
            int fd = ++fd_;

            connectors_.insert(std::make_pair(fd, connector));
            return(fd);
        }

        bool Release(int fd, Connector *connector)
        {
            Connectors::const_iterator it = connectors_.find(fd);

            if (it != connectors_.end()) {
                if (it->second == connector) {
                    connectors_.erase(it);
                    return(true);
                }
            }
            return(false);
        }
};
