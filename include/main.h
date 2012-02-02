#pragma once
#define BOOST_FILESYSTEM_VERSION    3
#include <list>
#include <string>
#include <boost/unordered_map.hpp>
#include "comparer.h"
#include "connector.h"
#include "fdmanager.h"
#include "log.h"

namespace FWL {
    class Main
    {
        class Location
        {
            private:
                ComparerIntr comparer_;
                Connector    *connector_;
            public:
                Location(ComparerIntr comparer, Connector *connector);
                Connector *operator()(const std::string& str) const;
        };

        typedef boost::unordered_map<std::string, ConnectorIntr>          Connectors;
        typedef boost::unordered_map<std::string, ConnectorFactoryIntr>   ConnectorFactories;
        typedef boost::unordered_map<std::string, ComparerFactoryIntr>    ComparerFactories;
        typedef std::list<Location>                                       Locations;

        ConnectorFactories connector_factories_;
        ComparerFactories  comparer_factories_;

        std::string config_file_;
        Connectors  connectors_;
        FdManager   fd_manager_;
        Locations   locations_;
        LogIntr     log_;
        public:
            Log& Logger() const;
            Main(const std::string& config_file);
            Connector *GetConnector(int fd);
            Connector *GetDirConnector(void *dd);
            Connector *GetConnector(const std::string& path);
            bool LoadConfig();
    };
}
