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

    // The result is the id of the first node that should be displayed starting
    // from the supplied block and traversing nodes (including across blocks)
    // until one is found that should be displayed.
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

            // If a node could not be found, recursively call findEdge for each block.
            if (!found) {
                std::map<std::string, llvm::BasicBlock*> i_blocks = nodes[nodes.size() - 1]->getBlockEdges();
                result = findEdge(i_blocks.begin()->second, blocks);
            }
        }
    }
    return result;
}

void
Block::processNodes(std::map<llvm::BasicBlock*, pBlock> blocks)
{
    // This is ugly, but works (in principle and reality).
    // nextNodeId holds the id of the node to point to.
    // Iterate through the nodes, starting at the end of the list.
    // If we have identified the exit node id (ie, where we
    // jump to another block), simply connect to the nextNodeId.
    // Otherwise, we need to traverse through the connected
    // blocks until we find a node suitable for display.
    int nextNodeId = -1;
    for (int i = _nodes.size() - 1; i >= 0; i--) {
        if (nextNodeId > 0) {
            // Only nodes with existing labels are processed
            if (_nodes[i]->getNodeLabel().length() > 0) {
                // Convert the next node id to a string and assign the
                // current node as the next node since we're working backwards.
                char buffer[255];
                sprintf(buffer, "%d", nextNodeId);
                _nodes[i]->addNodeEdge(new Edge(buffer));
                nextNodeId = _nodes[i]->getNodeId();
            }
        } else {
            if (_nodes[i]->getNodeLabel().length() > 0) {
                nextNodeId = _nodes[i]->getNodeId();
            } else {
                // Determine the blocks the node links to
                std::map<std::string, llvm::BasicBlock*> mapping =
                    _nodes[i]->getBlockEdges();
                // For each node this one links to...
                for (std::map<std::string, llvm::BasicBlock*>::iterator it = mapping.begin();
                     it != mapping.end();
                     it++) {
                    // Find the id for the edge that corresponds to the first node that
                    // would be displayed.  If the node has a label, it is
                    // the next id and the found edge is it's next id.
                    // Otherwise, the next id is the found edge.
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
}
