#pragma once
#define NDEBUG
#include <libjson/libjson.h>
#include <string>
#include <set>
#include <deque>
#include <boost/unordered_map.hpp>
#include <boost/unordered_set.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/intrusive_ptr.hpp>
#include <boost/detail/atomic_count.hpp>
#include "connectorfactory.h"
#include "comparerfactory.h"
#include "fdmanager.h"
#include "real.h"


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
		int sort_;
		boost::intrusive_ptr<Comparer> comparer_;
		Connector* connector_;
	    public:
		template<typename T>
		struct Comp
		{
		    bool operator()(const T& a, const T& b) 
		    {
			return a.sort_ > b.sort_;
		    }
		};
		
		Location(int sort, const boost::intrusive_ptr<Comparer>& comparer, Connector* connector)
		    : sort_(sort)
		    , comparer_(comparer)
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
	
	;

	typedef boost::unordered_map<std::string, boost::intrusive_ptr<Connector> > Connectors;
	typedef boost::unordered_map<size_t, Fd> Fds;
	typedef std::set<Location, Location::Comp<Location> > Locations;
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
	    int fd = real_.open(config_file_.c_str(), O_RDONLY);
	    if (fd == -1) {
		fprintf(stderr,"Couldn't open config file: %s %s(%d)\n",config_file_.c_str(), strerror(errno), errno);
		exit(-1);
	    }	

	    std::string config;
	    char buf[1024];
	    do  {
		size_t len = real_.read(fd, &buf[0], sizeof(buf));
		if (len == 0) break;
		config.append(&buf[0], len);
	    } while (1);

	    real_.close(fd);

	    try {
    		JSONNode cfg = libjson::parse(config);
    		JSONNode& connectors = cfg["connectors"];
    		for (JSONNode::const_iterator it = connectors.begin(); it != connectors.end(); ++it) {
    		    Connector* cntr = connector_factory_.Create((*it), real_, fd_manager_);
    		    printf("Connector %s - %p:\n", (*it).name().c_str(), cntr);
    		    if (cntr) {
    			connectors_.insert(std::make_pair((*it).name(), boost::intrusive_ptr<Connector>(cntr, true)));
    		    }
    		}
    		
    		JSONNode& locations = cfg["locations"];
    		for (JSONNode::const_iterator it = locations.begin(); it != locations.end(); ++it) {
    		    printf("Location:\nconnector:%s\n",(*it)["connector"].as_string().c_str());
    		    Connectors::iterator cit = connectors_.find((*it)["connector"].as_string());
    		    if (cit != connectors_.end()) {
    			printf("Connector found: rule:%s\n",(*it)["rule"].as_string().c_str());
    			Comparer* cmpr = comparer_factory_.Create((*it)["rule"].as_string());
    			if (cmpr) {
    			    printf("Comparer:%p\n", cmpr);
    			    
    			    int sort = 0;
    			    try {
    				sort = (*it).at("sort").as_int();
    			    } catch (const std::out_of_range& e) {};
    			
    			    locations_.insert(Location(sort, boost::intrusive_ptr<Comparer>(cmpr, true), cit->second.get()));
    			}
    		    }
    		}
    		//for (JSON
	    } catch(const std::invalid_argument& e) {
		fprintf(stderr, "error: %s\n", e.what());
		return false;
	    }
	    
	    return true;
	}
};
