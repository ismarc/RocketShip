#include "Block.h"

Block::Block(unsigned int identifier, std::string label) :
    _id(identifier),
    _label(label)
{
}

Block::~Block()
{
}

void
Block::processBlock()
{

}

unsigned int
Block::getId()
{
    return _id;
}

std::string
Block::getLabel()
{
    return _label;
}

Nodes
Block::getNodes()
{
    return _nodes;
}

void
Block::setId(unsigned int value)
{
    _id = value;
}

void
Block::setLabel(std::string value)
{
    _label = value;
}

void
Block::appendNode(pNode node)
{
    _nodes.push_back(node);
}
