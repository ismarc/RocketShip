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
    // Everything is reset per function.  Ideally we would just make
    // this a per-function pass, but extended feature plans make
    // applying this at the module level a better idea.
    _nodeId = 0;
    _blockId = 0;
    _blocks.clear();
    _pnodes.clear();
    
    std::vector<BasicBlock*> blockList;

    std::string functionLabel = F.getName();
    std::string demangledLabel = getDemangledName(functionLabel);

    if (demangledLabel == functionLabel ||
        demangledLabel.length() == 0) {
        functionLabel = F.getReturnType()->getDescription() + " " + functionLabel;
        functionLabel = functionLabel + "(";
        for (Function::arg_iterator arg = F.arg_begin();
             arg != F.arg_end();
             arg++) {
            if (arg != F.arg_begin()) {
                functionLabel = functionLabel + ", ";
            }
            functionLabel = functionLabel + arg->getType()->getDescription() + " " + std::string(arg->getName());
        }
        functionLabel = functionLabel + ")";
    } else {
        functionLabel = demangledLabel;
    }

    // Each block in the function needs to be processed and added to
    // the mapping.
    for (Function::iterator bblock = F.begin();
         bblock != F.end();
         bblock++) {
        pBlock block(new Block(_nodeId++, bblock->getName()));
        _blocks.insert(std::pair<BasicBlock*, pBlock>(bblock, block));
        blockList.push_back(bblock);

        if (bblock == F.begin()) {
            pNode node(new Node(_nodeId++));
            block->appendNode(node);
            node->setNodeLabel(functionLabel);
            node->setNodeType(Node::START);
        }
        processBlock(bblock, block);
    }

    // Each block needs to process its contained nodes and we need to
    // keep a local copy of each node for later processing.
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

    // Generates the function name and filename/output stream.
    std::string functionIdentifier = F.getName();
    char* result = cplus_demangle(functionIdentifier.c_str(), DMGL_ANSI|DMGL_PARAMS);

    std::replace(functionIdentifier.begin(), functionIdentifier.end(), '.', '_');
    _outputFile.open(std::string(functionIdentifier + ".dot").c_str());

    _outputFile << "digraph " << functionIdentifier << " {\n";

    // Emit each node to the output stream.  This should be modified
    // to have the node print itself out by passing in the output
    // stream rather than calling a separate function
    for (Nodes::iterator it = _pnodes.begin();
         it != _pnodes.end();
         it++) {
        if ((*it) == NULL) {
            continue;
        }

        // We only care about nodes with labels since they are what is
        // actually presented.
        if ((*it)->getNodeLabel().length() > 0) {
            emitNode(&(*(*it)));
        }
    }

    _outputFile << "}";
    _outputFile.close();
}

void
RocketShip::processBlock(BasicBlock* bblock, pBlock block)
{
    // Create a node for each instruction in the block and append it
    // to the block.
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
    // Assign the instruction and generate the node label.
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
    // A call instruction is the execution of a function.  The final
    // output format is:
    // call <function name> (<operand 1>, <operand 2>, <operand 3>)
    std::string result = instruction->getOpcodeName();
    std::string calledName = "";

    // Even if we are unable to get the called function, the function
    // signature can be generated later on.
    if (instruction->getCalledFunction() != NULL) {
        calledName = instruction->getCalledFunction()->getName();
    }

    std::string resultName = getDemangledName(calledName);

    result = result + " " + resultName;

    // Append the arguments from the operands.
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
    // Switch instruction labels are handled solely by getValueName to
    // determine the appropriate symbol that is checked.
    std::string label = instruction->getOpcodeName();

    label = label + " " + getValueName(instruction->getCondition());

    return label;
}

std::string
RocketShip::getStoreInstLabel(StoreInst* instruction)
{
    // Assignment/memory storage, uses := to indicate assignment.
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
    // Invoke instructions are identical to call instructions except
    // that they can result in a branch if an exception is
    // thrown/stack should unwind, etc.
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
            label = label + ")";
        }
    }
    
    return label;
}

std::string
RocketShip::getValueName(Value* value)
{
    // Recursively calls itself to resolve the base symbol represented
    // by value.  This is due to the nature of LLVM having "unlimited"
    // registers which results in not all Value's having associated
    // names.  We assume that the first Value in the chain that has a
    // name is the name we want to use.
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
        // Cast instructions get the value name of the base operand
        result = getValueName(castInst->getOperand(0));
    }
    else if (LoadInst* loadInst = dyn_cast<LoadInst>(&*value)) {
        // Load instructions get the value name of the item pointed to
        result = getValueName(loadInst->getPointerOperand());
    }
    else if (SExtInst* sextInst = dyn_cast<SExtInst>(&*value)) {
        // Sign extension instructions get the value name of the base operand.
        result = getValueName(sextInst->getOperand(0));
    }
    else if (ConstantInt* constant = dyn_cast<ConstantInt>(&*value)) {
        // Constant int values get the integer constant as the name,
        // in base-10.
        result = constant->getValue().toString(10, false);
    }
    else if (AllocaInst* allocaInst = dyn_cast<AllocaInst>(&*value)) {
        // Allocation instructions get the string "description" of the type.
        result = allocaInst->getAllocatedType()->getDescription();
    }
    else if (GetElementPtrInst* gepInst = dyn_cast<GetElementPtrInst>(&*value)) {
        // get element pointer instructions are used for dereferencing
        // arrays and other index based data structures.  The pointer
        // operand is used as the name with the value name of the
        // index is used with C-like syntax:
        // <pointer>[<index>]
        std::string value = getValueName(gepInst->getPointerOperand());

        std::string index = getValueName(gepInst->getOperand(gepInst->getNumIndices()));
        value = value + "[" + index + "]";
        result = value;
    }
    else if (BinaryOperator* binOp = dyn_cast<BinaryOperator>(&*value)) {
        // Binary operators are mathematical operations that take two
        // operands.  
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
            // An undefined binary operator instruction results in the
            // operator name and then the two operands it uses.
            result = binOp->getOpcodeName();
            result = result + " " + getValueName(binOp->getOperand(0));
            result = result + " " + getValueName(binOp->getOperand(1));
        }
    }

    // If we have not determined a result at this point, use the
    // description of the value as the identifier.
    if (result.length() == 0) {
        result = value->getType()->getDescription();
    }

    return result;
}

std::string
RocketShip::getDemangledName(std::string name)
{
    // cplus_demangle returns a NULL pointer if the supplied string
    // was not a mangled C++ identifier, or a string holding the
    // demangled symbol.
    std::string result;
    char* demangled = cplus_demangle(name.c_str(), DMGL_ANSI|DMGL_PARAMS);

    if (demangled != NULL) {
        result = std::string(demangled);
        // cplus_demangle allocates the memory and the caller is
        // responsible for freeing the char*.
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
    // Default handling is:
    // <instruction> <operand 1> <operand 2> <operand n>
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
