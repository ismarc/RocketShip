#include "Edge.h"

Edge::Edge(std::string id, std::string label):
    _id(id),
    _label(label)
{
}

Edge::~Edge()
{

}

std::string
Edge::getLabel()
{
    return _label;
}

std::string
Edge::getId()
{
    return _id;
}
