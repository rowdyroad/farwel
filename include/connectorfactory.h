#pragma once
#include "connector.h"

#include "connectors/dummy.h"

class ConnectorFactory
{
    public:
        Connector* Create(const JSONNode& config, const Real& real, FdManager& manager)
	{
	    if (config["type"].as_string() == "dummy") {
		return new Dummy(config, real, manager);
	    }
	    return NULL;
        }
    
};

