#include "Node.h"

Node::Node(int id, Type type) :
    _nodeId(id),
    _nodeType(type),
    _nodeName("")
{

}

Node::~Node()
{

}

int
Node::getNodeId()
{
    return _nodeId;
}

Node::Type
Node::getNodeType()
{
    return _nodeType;
}

std::vector<Edge*>
Node::getNodeEdges()
{
    return _edges;
}

std::string
Node::getNodeName()
{
    return _nodeName;
}

std::string
Node::getNodeLabel()
{
    return _nodeLabel;
}

void
Node::setNodeId(int value)
{
    _nodeId = value;
}

void
Node::setNodeType(Type value)
{
    _nodeType = value;
}

void
Node::setNodeName(std::string value)
{
    _nodeName = value;
}

void
Node::setNodeLabel(std::string value)
{
    _nodeLabel = value;
}

void
Node::addNodeEdge(Edge* edge)
{
    // Edges pointing to the same location are not allowed.  Each edge
    // must have a distinct id.
    bool found = false;
    for (std::vector<Edge*>::iterator it = _edges.begin();
         it != _edges.end();
         it++) {
        if ((*it)->getId() == edge->getId()) {
            found = true;
            break;
        }
    }

    if (!found) {
        _edges.push_back(edge);
    }
}

void
Node::removeNodeEdge(Edge* edge)
{
    for (std::vector<Edge*>::iterator it = _edges.begin();
         it != _edges.end();
         it++) {
        if ((*it)->getId() == edge->getId()) {
            _edges.erase(it);
            break;
        }
    }
}

void
Node::addBlockEdge(llvm::BasicBlock* block, std::string label)
{
    _blockEdges.insert(std::pair<std::string, llvm::BasicBlock*>(label, block));
}

std::map<std::string, llvm::BasicBlock*>
Node::getBlockEdges()
{
    return _blockEdges;
}
