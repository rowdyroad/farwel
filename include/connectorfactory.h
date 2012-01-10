#pragma once
#include "connector.h"
#include "json.h"

#include "connectors/dummy.h"



class ConnectorFactory
{
    public:
        Connector* Create(const std::string& name, const JsonNode& config, const Real& real, FdManager& manager)
	{
	    if (config.get("type","") == "dummy") {
		return new Dummy(name, real, manager);
	    }
	    return NULL;
        }
    
};

