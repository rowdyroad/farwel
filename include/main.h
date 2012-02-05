#pragma once
#define BOOST_FILESYSTEM_VERSION    3
#include <list>
#include <string>
#include <boost/unordered_map.hpp>
#include "comparer.h"
#include "connector.h"
#include "fdmanager.h"
#include "log.h"
#include "connectors.h"
extern "C" {
#include <ctype.h>
}
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
        
        std::string toLower(const std::string& src)
        {
    	    std::string str;
    	    str.resize(src.size());
    	    for (size_t i = 0;i < src.size(); ++i) {
    		str[i] = ::tolower(src[i]);
    	    }
    	    return str;
        }
        void addConnector(const std::string& name, ConnectorFactoryIntr factory)
        {
    	    connector_factories_.insert(std::make_pair(toLower(name),factory));
        }
        void addComparer(const std::string& name, ComparerFactoryIntr factory)
        {
    	    comparer_factories_.insert(std::make_pair(toLower(name),factory));
        }
        
        void createConnectors();
        void createComparers();
        
        public:
            Log& Logger() const;
            Main(const std::string& config_file);
            Connector *GetConnector(int fd);
            Connector *GetDirConnector(void *dd);
            Connector *GetConnector(const std::string& path);
            bool LoadConfig();
    };
}
