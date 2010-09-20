#include "Node.h"
#include "RocketShip.h"

#include "llvm/Instructions.h"
#include "llvm/Support/raw_ostream.h"

Node::Node(int identifier, Type type) :
    _nodeId(identifier),
    _nodeType(type),
    _nodeName(""),
    _instruction(NULL)
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
Node::setInstruction(llvm::Instruction* instruction)
{
    _instruction = instruction;
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

std::map<std::string, llvm::BasicBlock*>
Node::getBlockEdges()
{
    std::map<std::string, llvm::BasicBlock*> result;

    if (_instruction) {
        // Get Branch Instruction edges
        if (llvm::BranchInst* instruction = llvm::dyn_cast<llvm::BranchInst>(&*_instruction)) {
            if (instruction->isConditional()) {
                setNodeType(Node::DECISION);
                llvm::BasicBlock *Dest1 = llvm::dyn_cast<llvm::BasicBlock>(instruction->getOperand(1));
                llvm::BasicBlock *Dest2 = llvm::dyn_cast<llvm::BasicBlock>(instruction->getOperand(2));

                result.insert(std::pair<std::string, llvm::BasicBlock*>("false", Dest1));
                result.insert(std::pair<std::string, llvm::BasicBlock*>("true", Dest2));
            } else {
                llvm::BasicBlock *Dest = llvm::dyn_cast<llvm::BasicBlock>(instruction->getOperand(0));
                result.insert(std::pair<std::string, llvm::BasicBlock*>("x", Dest));
            }
        }
        // Get Switch instruction edges
        else if (llvm::SwitchInst* instruction = llvm::dyn_cast<llvm::SwitchInst>(&*_instruction)) {
            setNodeType(Node::DECISION);
            llvm::BasicBlock *defaultDest = instruction->getDefaultDest();
            result.insert(std::pair<std::string, llvm::BasicBlock*>("default",defaultDest));

            for (unsigned int i = instruction->getNumSuccessors() - 1; i > 0; i--) {
                llvm::BasicBlock* destination = instruction->getSuccessor(i);
                std::string label = rocketship::RocketShip::getValueName(instruction->getCaseValue(i));
                result.insert(std::pair<std::string, llvm::BasicBlock*>(label, destination));
            }
        }
        // Get Invoke instruction edges
        else if (llvm::InvokeInst* instruction = llvm::dyn_cast<llvm::InvokeInst>(&*_instruction)) {
            llvm::BasicBlock *normal = instruction->getNormalDest();
            llvm::BasicBlock *unwind = instruction->getUnwindDest();

            result.insert(std::pair<std::string, llvm::BasicBlock*>("unwind", unwind));
            result.insert(std::pair<std::string, llvm::BasicBlock*>("", normal));
        }
    }
    return result;
}
