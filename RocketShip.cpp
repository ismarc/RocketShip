#include "RocketShip.h"

#include "llvm/Pass.h"
#include "llvm/Module.h"
#include "llvm/Function.h"
#include "llvm/Instructions.h"
#include "llvm/Support/raw_ostream.h"

#include "Node.h"
#include "Edge.h"

#include <vector>
#include <fstream>
#include <stdio.h>

extern "C" {
#include <demangle.h>
}

using namespace llvm;
using namespace rocketship;

bool
RocketShip::runOnModule(Module &M) 
{
    // Uncomment below to send the LLVM assembly to stderr when run
    //M.dump();

    /**
     * The moduleIdentifier is used to uniquely identify the chart.
     * Unfortunately, DOT files can't handle any graph names or node
     * names that contain '.'.  so we just swap it out for '_'.
     */
    std::string moduleIdentifier = M.getModuleIdentifier();
    std::replace(moduleIdentifier.begin(), moduleIdentifier.end(), '.', '_');

    Module::iterator funcStart;

    // processFunction generates an entry in _nodes for each contained
    // node.  Each node has it's edges defined.  This builds out the
    // list of nodes for each function to be emitted at a later time.
    for (funcStart = M.begin();
         funcStart != M.end();
         funcStart++) {
        processFunction(*funcStart);
    }

    // Return false to indicate that we didn't alter the AST or module
    // at all.
    return false;
}

void
RocketShip::processFunction(Function &F) {
    _nodeId = 0;
    _blockId = 0;
    _blocks.clear();
    _pnodes.clear();
    
    std::vector<BasicBlock*> blockList;
    
    for (Function::iterator bblock = F.begin();
         bblock != F.end();
         bblock++) {
        pBlock block(new Block(_nodeId++, bblock->getName()));
        _blocks.insert(std::pair<BasicBlock*, pBlock>(bblock, block));
        blockList.push_back(bblock);
        processBlock(bblock, block);
    }

    for (std::map<BasicBlock*, pBlock>::iterator it = _blocks.begin();
         it != _blocks.end();
         it++) {
        it->second->processNodes(_blocks);
        Nodes nodes = it->second->getNodes();
        for (Nodes::iterator node = nodes.begin();
             node != nodes.end();
             node++) {
            _pnodes.push_back(*node);
        }
    }

    std::string functionIdentifier = F.getName();
    char* result = cplus_demangle(functionIdentifier.c_str(), DMGL_ANSI|DMGL_PARAMS);

    std::replace(functionIdentifier.begin(), functionIdentifier.end(), '.', '_');
    _outputFile.open(std::string(functionIdentifier + ".dot").c_str());

    _outputFile << "digraph " << functionIdentifier << " {\n";
    
    for (Nodes::iterator it = _pnodes.begin();
         it != _pnodes.end();
         it++) {
        if ((*it) == NULL) {
            continue;
        }
        
        emitNode(&(*(*it)));
    }

    _outputFile << "}";
    _outputFile.close();
}

void
RocketShip::processBlock(BasicBlock* bblock, pBlock block)
{
    for (BasicBlock::iterator instruction = bblock->begin();
         instruction != bblock->end();
         instruction++) {
        pNode node(new Node(_nodeId++));
        block->appendNode(node);
        processInstruction(instruction, node);
    }
}

void
RocketShip::processInstruction(Instruction* instruction, pNode node)
{
    node->setInstruction(instruction);
    node->setNodeLabel(getLabelForNode(instruction));
}

void
RocketShip::emitNode(Node* node)
{
    /**
     * This is all kinds of hacky.  The entire processing structure
     * and internal storage of nodes should be modified, but it'd be
     * hard to beat the speed of this, seeing as it's O(n).  This
     * works because of how DOT files are specified.  Node
     * "definitions" can occur anywhere and "node edge definitions"
     * can occur anywhere.  In practice, the current model is to
     * generate the definition of the node, followed by the edges
     * leading away from the node.
     */

    std::string name = node->getNodeName();
    std::vector<Edge*> edges = node->getNodeEdges();
    
    // First, DOT files can't have '.' as identifiers, so all '.'s are
    // replaced with '_'.
    std::replace(name.begin(), name.end(), '.', '_');

    /**
     * This begins the node definition in the file.  The node
     * definition includes the identifier (name or id), the label to
     * display for it and the shape of the node.  The format used is
     * node_identifier [label="<label>" shape="<shape>"]
     */ 
    // If the node has a name assigned to it (in practice, only
    // functions have names assigned), emit the name, otherwise,
    // use the node id that was assigned.
    if (name.length() > 0) {
        _outputFile << name;
    } else {
        _outputFile << node->getNodeId();
    }

    // Again, nice and hacky.  If a node doesn't have any edges to
    // follow (remember, this is a directed graph), it must be an end
    // node.
    if (edges.size() == 0) {
        node->setNodeType(Node::END);
    }

    // Every node has a label, even if that label is an empty string.
    // This greatly simplifies processing, but requires getNodeLable()
    // to return an empty string rather than NULL if a label hasn't
    // been assigned.
    _outputFile << " [label=\"" << node->getNodeLabel() << "\"";
    // Emit the shape to draw for the node.  To match the graphs,
    // start should technically be a filled circle with no name, end
    // should be a filled circle with a concentric circle with no
    // name.  The default is box since we don't have a way of knowing
    // what actual node type it is (makes it easy to add new node
    // types without needing special handling until it's known).
    _outputFile << " shape=";
    switch(node->getNodeType()) {
    case Node::START:
        //_outputFile << "circle";
        _outputFile << "none";
        break;
    case Node::END:
        //_outputFile << "doublecircle";
        _outputFile << "none";
        break;
    case Node::DECISION:
        _outputFile << "diamond";
        break;
    case Node::ACTIVITY:
    default:
        _outputFile << "box";
    }

    _outputFile << "]\n";
    /**
     * This ends the node definition portion.  The node will be
     * displayed in the graph and potentially have edges leading to
     * it.  At this point, no edges lead away from the node.
     */

    /**
     * This begins the node edge definition portion.  
     */
    // An entry in the file needs to occur with the following format
    // for the edges leading away from the node:
    // node_identifier -> subsequent_node_identifier [label="<label>"]
    // <label> is the label to apply to the edge, not to a node.
    for (std::vector<Edge*>::iterator i = edges.begin();
         i != edges.end();
         i++) {
        // Again, output the name or the id associated with the node.
        // This should theoretically be stored and reused, or linked
        // to a single call, or something.
        if (name.length() > 0) {
            _outputFile << name << " -> ";
        } else {
            _outputFile << node->getNodeId() << " -> ";
        }

        // Naming conventions of classes fail here.  Edge defines a
        // toId() method that returns the associated id, whether it is
        // the unique integer id of the node or a name associated with
        // it.
        _outputFile << (*i)->getId();
        
        // The label associated with the edge, typically empty but is
        // currently true/false for edges leading from decision nodes.
        _outputFile << "[label=\"" << (*i)->getLabel() << "\"]";

        _outputFile << "\n";
    }
}

std::string
RocketShip::getCallInstructionLabel(CallInst* instruction)
{
    std::string result = instruction->getOpcodeName();
    std::string calledName = "";

    if (instruction->getCalledFunction() != NULL) {
        calledName = instruction->getCalledFunction()->getName();
    }

    std::string resultName = getDemangledName(calledName);

    result = result + " " + resultName;

    if (calledName.compare(resultName) == 0 &&
        instruction->getCalledFunction() != NULL) {
        result = result + " (";

        for (unsigned int i = 1; i < instruction->getNumOperands(); i++) {
            if (i != 1) {
                result = result + ", ";
            }

            result = result + getValueName(instruction->getOperand(i));
        }

        result = result + ")";
    }
 
    return result;
}

std::string
RocketShip::getSwitchInstLabel(SwitchInst* instruction)
{
    std::string label = instruction->getOpcodeName();

    label = label + " " + getValueName(instruction->getCondition());

    return label;
}

std::string
RocketShip::getStoreInstLabel(StoreInst* instruction)
{
    std::string label = getValueName(instruction->getPointerOperand());

    label = label + " := ";

    label = label + getValueName(instruction->getOperand(0));

    return label;
}

std::string
RocketShip::getConditionalBranchLabel(BranchInst* instruction)
{
    std::string label = instruction->getOpcodeName();

    if (CmpInst *condition = dyn_cast<CmpInst>(instruction->getCondition())) {
        label = "";

        // Determine the name to use for the first value for comparison
        label = getValueName(condition->getOperand(0));

        // The comparison predicate is the method in which the two
        // values are compared. ICMP is integer comparison, FCMP is
        // floating point comparison.  For the purposes of generating
        // the graph, the difference between the two is meaningless.
        // Instead, simply convert the type of comparison to general
        // C-like comparison operators.
        switch (condition->getPredicate()) {
        // Equality comparison
        case CmpInst::ICMP_EQ:
        case CmpInst::FCMP_OEQ:
            label = label + " == ";
            break;
        // Inequality comparison
        case CmpInst::ICMP_NE:
        case CmpInst::FCMP_ONE:
            label = label + " != ";
            break;
        // Greater than signed/unsigned comparison
        case CmpInst::ICMP_UGT:
        case CmpInst::ICMP_SGT:
        case CmpInst::FCMP_OGT:
            label = label + " > ";
            break;
        // Greater than or equal signed/unsigned comparison
        case CmpInst::ICMP_UGE:
        case CmpInst::ICMP_SGE:
        case CmpInst::FCMP_OGE:
            label = label + " >= ";
            break;
        // Less than signed/unsigned comparison
        case CmpInst::ICMP_ULT:
        case CmpInst::ICMP_SLT:
        case CmpInst::FCMP_OLT:
            label = label + " < ";
            break;
        // Less than or equal signed/unsigned comparison
        case CmpInst::ICMP_ULE:
        case CmpInst::ICMP_SLE:
        case CmpInst::FCMP_OLE:
            label = label + " <= ";
            break;
        // Floating point comparisons that haven't been mapped into
        // the current model due to them specifying handling of NaN
        // and Infinity values.  Should decide about these eventually
        // and add them.
        case CmpInst::FCMP_FALSE:
        case CmpInst::FCMP_ORD:
        case CmpInst::FCMP_UNO:
        case CmpInst::FCMP_UEQ:
        case CmpInst::FCMP_UGT:
        case CmpInst::FCMP_UGE:
        case CmpInst::FCMP_ULT:
        case CmpInst::FCMP_ULE:
        case CmpInst::FCMP_UNE:
        case CmpInst::FCMP_TRUE:
            break;
        default:
            break;
        }

        // Add the second value that is being compared against.
        label = label + getValueName(condition->getOperand(1));
    }

    return label;
}

std::string
RocketShip::getInvokeInstLabel(InvokeInst* instruction)
{
    std::string label = "invoke";
    std::string calledName = "";

    if (instruction->getCalledFunction() != NULL) {
        calledName = instruction->getCalledFunction()->getName();
    }

    if (calledName.length() > 0) {
        char* demangled = cplus_demangle(calledName.c_str(), DMGL_ANSI|DMGL_PARAMS);

        if (demangled != NULL) {
            label = label + " " + std::string(demangled);
            free(demangled);
        } else {
            label = label + " " + std::string(instruction->getCalledFunction()->getName())
                + "(";

            for (unsigned int i = 1; i < instruction->getNumOperands(); i++) {
                if (i != 1) {
                    label = label + ", ";
                }
                label = label + getValueName(instruction->getOperand(i));
            }
        }
    }
    
    return label;
}

std::string
RocketShip::getValueName(Value* value)
{
    std::string result;
    
    if (value->hasName()) {
        result = value->getName();

        char* demangled = cplus_demangle(result.c_str(), DMGL_ANSI|DMGL_PARAMS);
        if (demangled != NULL) {
            result = std::string(demangled);
            free(demangled);
        }
        return result;
    }

    if (CastInst* castInst = dyn_cast<CastInst>(&*value)) {
        result = getValueName(castInst->getOperand(0));
    }
    else if (LoadInst* loadInst = dyn_cast<LoadInst>(&*value)) {
        result = getValueName(loadInst->getPointerOperand());
    }
    else if (SExtInst* sextInst = dyn_cast<SExtInst>(&*value)) {
        result = getValueName(sextInst->getOperand(0));
    }
    else if (ConstantInt* constant = dyn_cast<ConstantInt>(&*value)) {
        result = constant->getValue().toString(10, false);
    }
    else if (AllocaInst* allocaInst = dyn_cast<AllocaInst>(&*value)) {
        result = allocaInst->getAllocatedType()->getDescription();
    }
    else if (GetElementPtrInst* gepInst = dyn_cast<GetElementPtrInst>(&*value)) {
        std::string value = getValueName(gepInst->getPointerOperand());

        std::string index = getValueName(gepInst->getOperand(gepInst->getNumIndices()));
        value = value + "[" + index + "]";
        result = value;
    }
    else if (BinaryOperator* binOp = dyn_cast<BinaryOperator>(&*value)) {
        switch (binOp->getOpcode()) {
        case Instruction::SRem:
            result = getValueName(binOp->getOperand(0)) + " % "
                + getValueName(binOp->getOperand(1));
            break;
        case Instruction::Sub:
            result = getValueName(binOp->getOperand(0)) + " - "
                + getValueName(binOp->getOperand(1));
            break;
        case Instruction::Add:
            result = getValueName(binOp->getOperand(0)) + " + "
                + getValueName(binOp->getOperand(1));
            break;
        case Instruction::Mul:
            result = getValueName(binOp->getOperand(0)) + " * "
                + getValueName(binOp->getOperand(1));
            break;
        case Instruction::SDiv:
            result = getValueName(binOp->getOperand(0)) + " / "
                + getValueName(binOp->getOperand(1));
            break;
        default:
            result = binOp->getOpcodeName();
            result = result + " " + getValueName(binOp->getOperand(0));
            result = result + " " + getValueName(binOp->getOperand(1));
        }
    }

    if (result.length() == 0) {
        result = value->getType()->getDescription();
    }

    return result;
}

std::string
RocketShip::getDemangledName(std::string name)
{
    std::string result;
    char* demangled = cplus_demangle(name.c_str(), DMGL_ANSI|DMGL_PARAMS);

    if (demangled != NULL) {
        result = std::string(demangled);
        free(demangled);
    } else {
        result = name;
    }

    return result;
}

std::string
RocketShip::getLabelForNode(Instruction* instruction)
{
    std::string result = "";

    // Operations that have no display label
    if (isa<CmpInst>(&*instruction) ||
        isa<AllocaInst>(&*instruction) ||
        isa<CastInst>(&*instruction) ||
        isa<LoadInst>(&*instruction) ||
        isa<BinaryOperator>(&*instruction) ||
        isa<GetElementPtrInst>(&*instruction)) {
        result = "";
        // Comparison instructions are not displayed at all.  The
        // conditional branch instructions link to the comparison
        // instruction associated.  The decision node for a
        // conditional branch then displays the actual comparison
        // being made.

        // Allocation instructions are currently not displayed.
        // In the future, this should probably be modified to show
        // what memory is being allocated for each "thing".

        // Bitcast instructions cast from type A to type B but
        // guarantee there is no change in the value.  The
        // operation is not necessary to display in the graph.

        // Load instructions pull a value from memory.  This is
        // inconsequential for every language source except for
        // assembly.

        // Binary Operators are things like mul, (s|u)div, etc.
        // These values are assigned or used later, so they do not
        // need explicit display.

        // GetElementPtrInst references an index in a pointer.
        // This indexing will be referenced by other operations,
        // so it is redundant to display the box.
    }
    // Call Instructions
    else if (CallInst* callInst = dyn_cast<CallInst>(&*instruction)) {
        result = getCallInstructionLabel(callInst);
    }
    // Branch Instructions
    else if (BranchInst* branch = dyn_cast<BranchInst>(&*instruction)) {
        if (branch->isConditional()) {
            result = getConditionalBranchLabel(branch);
        } else {
            // Unconditional branches don't get displayed
        }
    }
    // Invoke Instructions
    else if (InvokeInst* invoke = dyn_cast<InvokeInst>(&*instruction)) {
        result = getInvokeInstLabel(invoke);
    }
    // Switch Instructions
    else if (SwitchInst* switchInstruction = dyn_cast<SwitchInst>(&*instruction)) {
        result = getSwitchInstLabel(switchInstruction);
    }
    // Store Instructions
    else if (StoreInst* store = dyn_cast<StoreInst>(&*instruction)) {
        result = getStoreInstLabel(store);
    }
    // Default handling
    else {
        result = instruction->getOpcodeName();

        for (unsigned int i = 0; i < instruction->getNumOperands(); i++) {
            result = result + " "
                + std::string(instruction->getOperand(i)->getName());
        }
    }
    
    return result;
}

/**
 * These are required by LLVM for each pass that's defined.
 * ID is assigned at runtime, but needs an initial assignment.
 * RegisterPass<T> registers the actual pass with the optimizer so it
 * is available to be called.
 */
char RocketShip::ID = 0;
static RegisterPass<RocketShip>
Y("rocketship", "RocketShip Pass");
