#include <sstream>
#include <string>
#include <set>
#include <deque>
#include <boost/optional.hpp>
#include <boost/foreach.hpp>
#include <boost/unordered_map.hpp>
#include <boost/unordered_set.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/intrusive_ptr.hpp>
#include <boost/detail/atomic_count.hpp>
#include "comparer.h"
#include "connector.h"
#include "fdmanager.h"
#include "real.h"
#include "json.h"
#include "log.h"
#include "main.h"
namespace FWL {
    Main::Location::Location(ComparerIntr comparer, Connector *connector)
        : comparer_(comparer)
        , connector_(connector)
    {}

    Connector *Main::Location::operator()(const std::string& str) const
    {
        if ((*comparer_)(str)) {
            return connector_;
        }
        return NULL;
    }

    Log& Main::Logger() const
    {
        return *log_;
    }

    Main::Main(const std::string& config_file)
        : config_file_(config_file)
        , log_(new Log(Log::Info))
    {
        connector_factories_.insert(std::make_pair("dummy", ConnectorFactoryIntr(new DummyFactory, false)));
        connector_factories_.insert(std::make_pair("memory", ConnectorFactoryIntr(new MemoryFactory, false)));
        connector_factories_.insert(std::make_pair("sqldb", ConnectorFactoryIntr(new DbFactory, false)));
        connector_factories_.insert(std::make_pair("memcache", ConnectorFactoryIntr(new MemcacheFactory, false)));
        comparer_factories_.insert(std::make_pair("always", ComparerFactoryIntr(new AlwaysFactory, false)));
        comparer_factories_.insert(std::make_pair("regexp", ComparerFactoryIntr(new RegexpFactory, false)));
        Logger().RegisterSink("stderr", stderr);
        Logger().RegisterSink("stdout", stdout);
        LoadConfig();
    }

    Connector *Main::GetConnector(int fd)
    {
        return fd_manager_.GetConnector(fd);
    }

    Connector *Main::GetDirConnector(void *dd)
    {
        return fd_manager_.GetDirConnector((int *)dd);
    }

    Connector *Main::GetConnector(const std::string& path)
    {
        for (Locations::iterator it = locations_.begin(); it != locations_.end(); ++it) {
            Connector *cntr = (*it)(path);
            if (cntr) {
                return cntr;
            }
        }
        return NULL;
    }

    bool Main::LoadConfig()
    {
        JsonNode root;
        boost::property_tree::read_json(config_file_, root);

        try {
            Logger().Inf("Loading configuration...");
            JsonNodeOp log = root.get_child_optional("log");
            if (!log) {
                Logger().UseSink("stdout");
            } else {
                BOOST_FOREACH(const JsonNode::value_type & it, log->get_child("registered_sinks"))
                {
                    Logger().RegisterSink(it.first, it.second.get_value<std::string>());
                };
                JsonNodeOp sinks = log->get_child_optional("sinks");
                if (!sinks || sinks->empty()) {
                    Logger().UseSink("stdout");
                } else {
                    BOOST_FOREACH(const JsonNode::value_type & it, *sinks)
                    {
                        Logger().UseSink(it.second.get_value<std::string>());
                    }
                }
            }

            BOOST_FOREACH(const JsonNode::value_type & it, root.get_child("connectors"))
            {
                ConnectorFactories::iterator cntrit = connector_factories_.find(it.second.get<std::string>("type"));

                if (cntrit != connector_factories_.end()) {
                    Connector *cntr = cntrit->second->Create(it.first, it.second, fd_manager_, log_);
                    Logger().Inf("Connector %s - %p:\n", it.first.c_str(), cntr);
                    if (cntr) {
                        connectors_.insert(std::make_pair(it.first, ConnectorIntr(cntr, false)));
                    }
                }
            }

            const JsonNode& locations = root.get_child("locations");
            BOOST_FOREACH(const JsonNode::value_type & it, locations)
            {
                std::string cntr_name = it.second.get<std::string>("connector");

                Logger().Inf("Location:\nconnector:%s - %s\n", it.first.c_str(), cntr_name.c_str());
                Connectors::iterator cit = connectors_.find(cntr_name);
                if (cit != connectors_.end()) {
                    Logger().Inf("Connector found: rule:%s\n", it.first.c_str());
                    std::pair<std::string, std::string> ret;
                    if (Comparer::Parse(it.first, ret)) {
                        ComparerFactories::iterator cmpit = comparer_factories_.find(ret.first);
                        if (cmpit != comparer_factories_.end()) {
                            Comparer *cmpr = cmpit->second->Create(ret.second);
                            if (cmpr) {
                                Logger().Inf("Comparer:%p\n", cmpr);
                                locations_.push_back(Location(ComparerIntr(cmpr, false), cit->second.get()));
                            }
                        }
                    }
                }
            }

            if (log) {
                JsonNodeOp level = log->get_child_optional("level");
                if (level) {
                    Logger().SetLogLevel(level->get_value<std::string>());
                }
            }
        } catch (const std::exception& e) {
            Logger().Err(e.what());
            return false;
        }

        return true;
    }
}
