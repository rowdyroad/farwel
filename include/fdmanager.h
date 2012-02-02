#pragma once
#include <boost/unordered_map.hpp>
#include <boost/detail/atomic_count.hpp>
extern "C" {
#include <sys/types.h>
#include <sys/sysctl.h>
}

namespace FWL {
    class Connector;

    class FdManager
    {
        private:
            int begin_;
            boost::detail::atomic_count fd_;
            typedef boost::unordered_map<int, Connector *>   Connectors;
            Connectors connectors_;

            Connector *GetConnector(int fd);
            Connector *GetDirConnector(int *d) { return GetConnector(*d); }
            friend class Main;
        public:
            FdManager();
            int Get(Connector *connector);
            bool Release(int fd, Connector *connector);
    };
}
