#pragma once
#define NDEBUG
#define BOOST_FILESYSTEM_VERSION 3
#include <sstream>
#include <string>
#include <set>
#include <deque>
#include <boost/filesystem.hpp>
#include <boost/foreach.hpp>
#include <boost/unordered_map.hpp>
#include <boost/unordered_set.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/intrusive_ptr.hpp>
#include <boost/detail/atomic_count.hpp>
#include "comparer.h"
#include "connector.h"
#include "connectors/realwrapper.h"
#include "fdmanager.h"
#include "real.h"
#include "json.h"

#include "debug.h"
class Main
{	
        class Location
        {
    	    private:
		boost::intrusive_ptr<Comparer> comparer_;
		Connector* connector_;
	    public:

		Location(const boost::intrusive_ptr<Comparer>& comparer, Connector* connector)
		    : comparer_(comparer)
		    , connector_(connector)
		{}
		
		Connector* operator()(const std::string& str) const
		{
		    if ((*comparer_)(str)) {
			return connector_;
		    }
		    return NULL;
		}
	};

	typedef boost::unordered_map<std::string, boost::intrusive_ptr<Connector> > Connectors;
	typedef boost::unordered_map<std::string, boost::intrusive_ptr<ConnectorFactory> > ConnectorFactories;
	typedef boost::unordered_map<std::string, boost::intrusive_ptr<ComparerFactory> > ComparerFactories;
	typedef std::list<Location> Locations;
	
	ConnectorFactories connector_factories_;
	ComparerFactories comparer_factories_;
	
	const Real& real_;
        std::string config_file_;
	Connectors connectors_;
	FdManager fd_manager_;
	Locations locations_;
	RealWrapper real_wrapper_;
    public:
    
	Main(const Real& real, const std::string& config_file)
	    : real_(real)
	    , config_file_(config_file)
	    , real_wrapper_("RealWrapper", real, fd_manager_)
	{
	    connector_factories_.insert(std::make_pair("dummy", boost::intrusive_ptr<ConnectorFactory>(new DummyFactory)));
	    connector_factories_.insert(std::make_pair("memory", boost::intrusive_ptr<ConnectorFactory>(new MemoryFactory)));

	    comparer_factories_.insert(std::make_pair("always", boost::intrusive_ptr<ComparerFactory>(new AlwaysFactory)));
	    comparer_factories_.insert(std::make_pair("regexp", boost::intrusive_ptr<ComparerFactory>(new RegexpFactory)));
	    
	    
	    LoadConfig();
	}
	
	Connector* GetConnector(size_t fd)
	{
	    return fd_manager_.GetConnector(fd);
	}
	
	Connector* GetConnector(void* dd)
	{
	    return fd_manager_.GetConnector(dd);
	}
	
	Connector* GetPathConnector(const std::string& str)
	{
	    std::string path = boost::filesystem::absolute(str).string();
	    for (Locations::iterator it = locations_.begin(); it != locations_.end(); ++it) {
		Connector* cntr = (*it)(path);
		if (cntr) {
		    return cntr;
		}
	    }
	    return &real_wrapper_;
	}
	
	bool LoadConfig()
	{
	    std::stringstream config;
	    
	    int fd = real_.open(config_file_.c_str(), O_RDONLY);
	    if (fd == -1) {
		fprintf(stderr,"Couldn't open config file: %s %s(%d)\n",config_file_.c_str(), strerror(errno), errno);
		exit(-1);
	    };
	    char buf[1024];
	    do  {
		size_t len = real_.read(fd, &buf[0], sizeof(buf));
		if (len == 0) break;
		config.write(&buf[0], len);
	    } while (1);
	    real_.close(fd);

	    JsonNode root;
	    boost::property_tree::read_json(config, root);
	    try {
	    
		printf("Loading configuration...\n");
    		BOOST_FOREACH(const JsonNode::value_type &it, root.get_child("connectors")) {
    		    ConnectorFactories::iterator cntrit = connector_factories_.find(it.second.get<std::string>("type"));
    		    if (cntrit != connector_factories_.end()) {
    			Connector* cntr = cntrit->second->Create(it.first, it.second, real_, fd_manager_);
			printf("Connector %s - %p:\n", it.first.c_str(), cntr);
    			if (cntr) {
    			    connectors_.insert(std::make_pair(it.first, boost::intrusive_ptr<Connector>(cntr, true)));
    		        }
    		    }
    		}
    		
    		JsonNode& locations = root.get_child("locations");
    		size_t sort = 0;
    		BOOST_FOREACH(const JsonNode::value_type &it, root.get_child("locations")) {
		    std::string cntr_name = it.second.get<std::string>("connector");
    		    printf("Location:\nconnector:%s - %s\n",it.first.c_str(), cntr_name.c_str());
    		    Connectors::iterator cit = connectors_.find(cntr_name);
    		    if (cit != connectors_.end()) {
    			printf("Connector found: rule:%s\n",it.first.c_str());
    			std::pair<std::string, std::string> ret;
    			if (Comparer::Parse(it.first, ret)) {
    			    ComparerFactories::iterator cmpit = comparer_factories_.find(ret.first);
    			    if (cmpit != comparer_factories_.end()) {
    				Comparer* cmpr = cmpit->second->Create(ret.second);
    				if (cmpr) {
	    			    printf("Comparer:%p\n", cmpr);
    				    locations_.push_back(Location(boost::intrusive_ptr<Comparer>(cmpr, true), cit->second.get()));
    			    	}
    			    }
    			    
    			}
    		    }
    		}
	    } catch(const std::exception& e) {
		fprintf(stderr, "error: %s\n", e.what());
		return false;
	    }
	    
	    return true;
	}
};
