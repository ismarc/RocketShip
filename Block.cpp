#include "Block.h"

#include <stdio.h>

Block::Block(unsigned int identifier, std::string label) :
    _id(identifier),
    _label(label)
{
}

Block::~Block()
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

void
Block::processNodes(std::map<llvm::BasicBlock*, pBlock> blocks)
{
    for (unsigned int i = 0; i < _nodes.size(); i++) {
        if (i != (_nodes.size() - 1)) {
            char buffer[255];
            sprintf(buffer, "%d", _nodes[i]->getNodeId());
            _nodes[i]->addNodeEdge(new Edge(buffer));
        } else {
            std::map<std::string, llvm::BasicBlock*> mapping = _nodes[i]->getBlockEdges();
            for (std::map<std::string, llvm::BasicBlock*>::iterator it = mapping.begin();
                 it != mapping.end();
                 it++) {
                std::map<llvm::BasicBlock*, pBlock>::iterator entry = blocks.find(it->second);
                if (entry != blocks.end()) {
                    char buffer[255];
                    sprintf(buffer, "%d", entry->second->getId());
                    _nodes[i]->addNodeEdge(new Edge(std::string(buffer), it->first));
                }
            }
        }
    }
}
