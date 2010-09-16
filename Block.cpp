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

int
Block::findEdge(llvm::BasicBlock* block, std::map<llvm::BasicBlock*, pBlock> blocks)
{
    int result = -1;

    std::map<llvm::BasicBlock*, pBlock>::iterator entry = blocks.find(block);
    if (entry != blocks.end()) {
        Nodes nodes = entry->second->getNodes();
        if (nodes.size() > 0) {
            bool found = false;
            for (unsigned int j = 0; j < nodes.size(); j++) {
                if (nodes[j]->getNodeLabel().length() > 0) {
                    result = nodes[j]->getNodeId();
                    found = true;
                    break;
                }
            }

            if (!found) {
                std::map<std::string, llvm::BasicBlock*> i_blocks = nodes[nodes.size() - 1]->getBlockEdges();
                result = findEdge(i_blocks.begin()->second, blocks);
                //result = findEdge(nodes[nodes.size() - 1]->getBlockEdges()["x"], blocks);
            }
        }
    }
    return result;
}

void
Block::processNodes(std::map<llvm::BasicBlock*, pBlock> blocks)
{
    int nextNodeId = -1;
    for (int i = _nodes.size() - 1; i >= 0; i--) {
        if (nextNodeId > 0) {
            if (_nodes[i]->getNodeLabel().length() > 0) {
                char buffer[255];
                sprintf(buffer, "%d", nextNodeId);
                _nodes[i]->addNodeEdge(new Edge(buffer));
                nextNodeId = _nodes[i]->getNodeId();
            }
        } else {
            std::map<std::string, llvm::BasicBlock*> mapping =
                _nodes[i]->getBlockEdges();
            for (std::map<std::string, llvm::BasicBlock*>::iterator it = mapping.begin();
                 it != mapping.end();
                 it++) {
                int edgeId = findEdge(it->second, blocks);
                if (_nodes[i]->getNodeLabel().length() > 0) {
                    char buffer[255];
                    sprintf(buffer, "%d", edgeId);
                    std::string edgeLabel = std::string(buffer);
                    _nodes[i]->addNodeEdge(new Edge(edgeLabel, it->first));
                    nextNodeId = _nodes[i]->getNodeId();
                } else {
                    nextNodeId = edgeId;
                }
            }
        }
    }
}
