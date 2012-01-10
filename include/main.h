#pragma once
#define NDEBUG
#include <sstream>
#include <string>
#include <set>
#include <deque>
#include <boost/foreach.hpp>
#include <boost/unordered_map.hpp>
#include <boost/unordered_set.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/intrusive_ptr.hpp>
#include <boost/detail/atomic_count.hpp>
#include "connectorfactory.h"
#include "comparerfactory.h"
#include "fdmanager.h"
#include "real.h"
#include "json.h"

#include "debug.h"
class Main
{	
    public:
	class Fd
	{
	    private:
        	Connector* connector_;	
        	int fd_;
    	    public:
    		Connector* GetConnector() const
    		{
    		    return connector_;
    		}
    		int GetRealFd() const
    		{
    		    return fd_;
    		    
    		}
    		Fd(Connector* connector)
    		    : connector_(connector)
    		    , fd_(-1)
    		{}
    		
    		Fd(int real_fd)
    		    : connector_(NULL)
    		    , fd_(real_fd)
    		{}
	};
	private:
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
	typedef boost::unordered_map<size_t, Fd> Fds;
	typedef std::list<Location> Locations;
	typedef boost::unordered_set<size_t> IgnoringFds;
	
	const Real& real_;
        std::string config_file_;
	Connectors connectors_;
	
	Fds fds_;
	IgnoringFds ignoring_fds_;
	FdManager fd_manager_;
	Locations locations_;
	ComparerFactory comparer_factory_;
	ConnectorFactory connector_factory_;
    public:
    
	Main(const Real& real, const std::string& config_file)
	    : real_(real)
	    , config_file_(config_file)
	{
	    LoadConfig();
	}
	
	Fd* GetFd(size_t fd)
	{
	    Fds::iterator it = fds_.find(fd);
	    if  (it != fds_.end()) {
		return &it->second;
	    }
	    return NULL;
	}
	
	bool AddConnector(size_t fd, Connector* cntr)
	{
	    return fds_.insert(std::make_pair(fd, Fd(cntr))).second;
	}
	
	size_t AddRealFd(int fd)
	{
	    size_t ffd = fd_manager_.GetFd();
	    fds_.insert(std::make_pair(ffd, Fd(fd)));
	    return ffd;
	}
	
	Connector* GetPathConnector(const std::string& str) const
	{
	    for (Locations::const_iterator it = locations_.begin(); it != locations_.end(); ++it) {
		Connector* cntr = (*it)(str);
		if (cntr) {
		    return cntr;
		}
	    }
	    return NULL;
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
	    
		printf("Loading configuration..");
    		BOOST_FOREACH(const JsonNode::value_type &it, root.get_child("connectors")) {
    		    Connector* cntr = connector_factory_.Create(it.first, it.second, real_, fd_manager_);
    		    printf("Connector %s - %p:\n", it.first.c_str(), cntr);
    		    if (cntr) {
    			connectors_.insert(std::make_pair(it.first, boost::intrusive_ptr<Connector>(cntr, true)));
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
    			Comparer* cmpr = comparer_factory_.Create(it.first);
    			if (cmpr) {
    			    printf("Comparer:%p\n", cmpr);
    			    locations_.push_back(Location(boost::intrusive_ptr<Comparer>(cmpr, true), cit->second.get()));
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
