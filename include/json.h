#pragma once

#include <boost/property_tree/json_parser.hpp>

typedef boost::property_tree::ptree JsonNode;

typedef boost::optional<JsonNode&> JsonNodeOp;
typedef boost::optional<const JsonNode&> JsonNodeConstOp;
