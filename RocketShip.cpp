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

using namespace llvm;
using namespace rocketship;

bool
RocketShip::runOnModule(Module &M) {
    // Uncomment below to send the LLVM assembly to stderr when run
    M.dump();

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
    // F.size() corresponds to the number of BasicBlocks contained in
    // the function.  This excludes functions that don't have any
    // BasicBlocks.  Why draw a boring chart with only the function
    // name?
    if (F.size() != 0) {
        std::string functionIdentifier = F.getName();
        std::replace(functionIdentifier.begin(), functionIdentifier.end(), '.', '_');
        _outputFile.open(std::string(functionIdentifier + ".dot").c_str());

        _outputFile << "digraph " << functionIdentifier << " {\n";
    
        // Generate the node that the structure of this function will
        // use as the root node.
        Node* funcNode = new Node(_nodeId++, Node::START);
        funcNode->setNodeName(F.getName());

        std::string label = F.getReturnType()->getDescription();
        label = label + " " + std::string(F.getName());
        label = label + "(";
        std::string arguments = "";
        
        Function::arg_iterator arg;
        for (arg = F.arg_begin();
             arg != F.arg_end();
             arg++) {
            if (arguments.length() != 0) {
                arguments = arguments + ", ";
            }
            arguments = arguments + std::string((*arg).getType()->getDescription());
            arguments = arguments + " " + std::string((*arg).getName());
        }
        label = label + arguments + ")";
        funcNode->setNodeLabel(label);
        _nodes.push_back(funcNode);

        // Since all nodes get pushed to _nodes, no result value needs
        // to be saved or passed back up, only the processing of the
        // structure.
        generateNodeStructure(funcNode, F);

        // Spit out the node definition and all it's edges for each node
        // in _nodes.  There will be exactly one entry for each node
        // displayed.  Check for NULL since it is theoretically possible
        // that a node entry is NULL because of terminators.
        for (std::vector<Node*>::iterator it = _nodes.begin();
             it != _nodes.end();
             it++) {
            if ((*it) == NULL) {
                continue;
            }
        
            emitNode((*it));
        }

        // All entries in _nodes have been processed, clear the list
        // and close out the file.
        _nodes.clear();
        _outputFile << "}";
        _outputFile.close();
    }

}

void
RocketShip::generateNodeStructure(Node* funcNode, Function &F)
{
    // currentNode contains the node that was previously processed.
    // This makes sense since each point of processing is essentially
    // "look-ahead", as in nodes aren't added to the structure until
    // they have been linked to by a previous node.
    Node* currentNode = NULL;
    Function::iterator block;

    // Each block should be processed, having the currentNode passed
    // in.  currentNode may be NULL, or may have another node as its
    // contents.  In practice, each block has a branch instruction as
    // the last entry so currentNode will always result in a NULL
    // value between processFunctionBlock calls.
    for (block = F.begin();
         block != F.end();
         block++) {
        processFunctionBlock(funcNode, currentNode, &(*block));
    }
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

void
RocketShip::processFunctionBlock(Node* funcNode, Node* currentNode, BasicBlock* block)
{
    // The blockNode corresponds to the label definition of a block.
    // It doesn't technically exist as as an actually activity, but
    // represents a label that other nodes can point to.  Eventually
    // this should be its own type and excluded when drawing the
    // graph.
    Node* blockNode = new Node(_nodeId++);

    // The name and label of the node derives from the identifier for
    // the BasicBlock it represents and the function it is contained
    // by.  The result becomes:
    // <function_name><basic_block_id> [label="<basic_block_id>"]
    // This allows the block to be uniquely identified between 
    // functions.
    blockNode->setNodeName(funcNode->getNodeName()
                           + std::string(block->getName()));
    blockNode->setNodeLabel(block->getName());
    _nodes.push_back(blockNode);

    // Currently, currentNode will always be NULL when it reaches
    // this point.  However, this behavior may change.
    if (currentNode == NULL) {
        currentNode = blockNode;
    } 

    // Control in a function begins with the first listed label.  if
    // there is not an edge leading from the function start yet, this
    // block becomes the first edge.
    if (funcNode->getNodeEdges().size() == 0) {
        funcNode->addNodeEdge(new Edge(currentNode->getNodeName()));
    }

    // Each instruction needs to be processed.  Some instructions do
    // not generate any nodes but add edges to existing nodes (such
    // as unconditional branches) while others have multiple edges
    // assigned.  All built edges are stored in _nodes.
    BasicBlock::iterator instruction;
    for (instruction = block->begin();
         instruction != block->end();
         instruction++) {
        Node* instructionNode = new Node(_nodeId++);

        // call instructions
        if (CallInst* callInst = dyn_cast<CallInst>(&*instruction)) {
            // Call instructions represent an external function call.
            currentNode = processCallInstruction(instructionNode,
                                                 currentNode,
                                                 callInst);
        }
        // branch instructions
        else if (BranchInst* branchInst = dyn_cast<BranchInst>(&*instruction)) {
            if (branchInst->isConditional()) {
                // Conditional branch instructions represent decision
                // nodes and need to be handled differently from
                // unconditional branches.
                currentNode = processConditionalBranch(instructionNode,
                                                       currentNode,
                                                       branchInst,
                                                       funcNode->getNodeName());
            } else {
                // Unconditional branches do not generate a node
                // themselves, instead linking the current node to the
                // node that represents the label associated.
                currentNode = processUnconditionalBranch(instructionNode,
                                                         currentNode,
                                                         branchInst,
                                                         funcNode->getNodeName());
            }
        }
        // Ignored instructions
        // These should be revisted at some point to see if they
        // should be included.
        else if (isa<CmpInst>(&*instruction) ||
                 isa<AllocaInst>(&*instruction) ||
                 isa<CastInst>(&*instruction) ||
                 isa<LoadInst>(&*instruction) ||
                 isa<BinaryOperator>(&*instruction) ||
                 isa<GetElementPtrInst>(&*instruction)) {
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
            // These values are assigned or used later, so the do not
            // need explicit display.

            // GetElementPtrInst references an index in a pointer.
            // This indexing will be referenced by other operations,
            // so it is redundant to display the box.
        }
        // store instructions
        else if (StoreInst* storeInst = dyn_cast<StoreInst>(&*instruction)) {
            // Store instructions have a special label formatting to
            // make it more obvious what value is being assigned
            // where.
            currentNode = processStoreInstruction(instructionNode,
                                                  currentNode,
                                                  storeInst);
        }
        // switch statement instructions
        else if (SwitchInst* switchInst = dyn_cast<SwitchInst>(&*instruction)) {
            currentNode = processSwitchInstruction(instructionNode,
                                                   currentNode,
                                                   switchInst,
                                                   funcNode->getNodeName());
        }
        // default
        else {
            // Default instruction handling, if nothing special is to
            // be done, or if no special handling has been defined,
            // generate a node with a label of:
            // <opcodename> <operand(0)> <operand(1)> <operand(n)>
            currentNode = processDefaultInstruction(instructionNode,
                                                    currentNode,
                                                    instruction);
        }
    }

    // currentNode becomes NULL because every instruction has been
    // processed and the block is "closed"
    currentNode = NULL;
}

Node*
RocketShip::processCallInstruction(Node* instructionNode,
                                   Node* currentNode,
                                   CallInst* callInst)
{
    // This is a nice hacky way to generate the label to use for a
    // call instruction.  The final result will look like:
    // call <function_name>(<operand(0)>, <operand(1)>, <operand(n)>)

    // This could theoretically be replaced with just "call", but it
    // is likely that there are other methods of invoking functions
    // that will use it.
    std::string label = callInst->getOpcodeName();
    
    // A call instruction should theoretically always have a function
    // that was called, but there are cases where the called function
    // hasn't been populated.
    if (callInst->getCalledFunction() != NULL) {
        label = label + " "
            + std::string(callInst->getCalledFunction()->getName()) + "(";
    }

    // List each operand (values passed to the function) as their
    // name, constant value or type in that order.
    for (unsigned int i = 1; i < callInst->getNumOperands(); i++) {
        if (i != 1) {
            label = label + ", ";
        }
        label = label + getValueName(callInst->getOperand(i));
    }
            
    label = label + ")";
            
    instructionNode->setNodeLabel(label);
                
    _nodes.push_back(instructionNode);

    // call instructions don't have a unique name so they are
    // referenced by id.  Generate the std::string from the integer id
    // to populate for the edge linking to the call instruction node.
    char buffer[255];
    sprintf(buffer, "%d", instructionNode->getNodeId());

    if (currentNode != NULL) {
        currentNode->addNodeEdge(new Edge(std::string(buffer)));
    }

    // An edge has been added to the instruction node so it should be
    // considered the current node.
    return instructionNode;
}

Node*
RocketShip::processConditionalBranch(Node* instructionNode,
                                     Node* currentNode,
                                     BranchInst* branchInst,
                                     std::string functionName)
{
    /**
     * Conditional branch instructions always have two edges leading
     * away from it, one for true values and one for false values.
     * The ordering is somewhat unintuitive.  The layout of branch
     * instructions are:
     * 0 -- The value that is checked for truth
     * 1 -- The basic block that is jumped to if the value is false
     * 2 -- The basic block that is jumped to if the value is true
     */
    instructionNode->setNodeType(Node::DECISION);
    std::string label = branchInst->getOpcodeName();

    // Get the BasicBlocks that the conditional branch has edges to.
    BasicBlock *Dest1 = dyn_cast<BasicBlock>(branchInst->getOperand(1));
    BasicBlock *Dest2 = dyn_cast<BasicBlock>(branchInst->getOperand(2));

    // Add the edges that the instruction node will lead to, setting
    // the appropriate label for the edges
    instructionNode->addNodeEdge(new Edge(functionName +
                                          std::string(Dest1->getName()), "false"));
    instructionNode->addNodeEdge(new Edge(functionName +
                                          std::string(Dest2->getName()), "true"));

    // This entire section is for building the label that is used in
    // the node.  It loads the comparison instruction that is used for
    // setting the value that is checked by the branch instruction.
    if (CmpInst *condition = dyn_cast<CmpInst>(branchInst->getCondition())) {
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

    instructionNode->setNodeLabel(label);
                
    _nodes.push_back(instructionNode);
    char buffer[255];
    sprintf(buffer, "%d", instructionNode->getNodeId());
    if (currentNode != NULL) {
        currentNode->addNodeEdge(new Edge(std::string(buffer)));
    }

    // currentNode becomes NULL because the jump is to a different
    // block and any instruction in the current block after this one
    // will not be executed afterwards.
    return NULL;
}

Node*
RocketShip::processUnconditionalBranch(Node* instructionNode,
                                       Node* currentNode,
                                       BranchInst* branchInst,
                                       std::string functionName)
{
    // Unconditional branches are just a jump to a specific label, so
    // the current node only needs to have the appropriate edge
    // assigned.  It should also probably be set to NULL, but it
    // doesn't matter yet as unconditional branches only occur at the
    // end of blocks right now.
    BasicBlock *Dest = dyn_cast<BasicBlock>(branchInst->getOperand(0));
    if (currentNode != NULL) {
        currentNode->addNodeEdge(new Edge(functionName
                                          + std::string(Dest->getName())));
    }

    // No new nodes have been appended to the list, so the current
    // node doesn't change.
    return currentNode;
}

Node*
RocketShip::processStoreInstruction(Node* instructionNode,
                                    Node* currentNode,
                                    StoreInst* storeInst)
{
    /**
     * Store instructions are assignment to a memory location.  In
     * practice this is assignment to a variable (reference to a
     * memory location).  := syntax is used to clearly indicate
     * assignment and not equality.  The format becomes:
     * <value_location_name> := <value>
     */
    std::string label = "";

    // Get the value for the left-hand side of the store operation.
    label = label + getValueName(storeInst->getPointerOperand());

    label = label + " := ";

    // Get the value for the right-hand side of the store operation
    label = label + getValueName(storeInst->getOperand(0));

    _nodes.push_back(instructionNode);
    instructionNode->setNodeLabel(label);

    char buffer[255];
    sprintf(buffer, "%d", instructionNode->getNodeId());

    if (currentNode != NULL) {
        currentNode->addNodeEdge(new Edge(std::string(buffer)));
    }
    return instructionNode;
}

Node*
RocketShip::processSwitchInstruction(Node* instructionNode,
                                     Node* currentNode,
                                     SwitchInst* switchInst,
                                     std::string functionName)
{
    /**
     * Switch instructions are special case conditional branches.
     * They compare a single value against "case statements" that
     * determine which block to execute.
     */
    instructionNode->setNodeType(Node::DECISION);
    std::string label = switchInst->getOpcodeName();

    label = label + " " + getValueName(switchInst->getCondition());

    // Set the default destination
    BasicBlock* defaultDest = switchInst->getDefaultDest();
    instructionNode->addNodeEdge(new Edge(functionName +
                                          std::string(defaultDest->getName()), "default"));

    // Each successor indicates a block to branch to if the specific
    // value is met.  An edge needs to be added for each potential
    // branch location with a label indicating the value being met.
    for (unsigned int i = switchInst->getNumSuccessors() - 1; i > 0; i--) {
        BasicBlock* destination = switchInst->getSuccessor(i);
        instructionNode->addNodeEdge(new Edge(functionName +
                                              std::string(destination->getName()),
                                              getValueName(switchInst->getCaseValue(i))));
    }

    instructionNode->setNodeLabel(label);

    _nodes.push_back(instructionNode);
    char buffer[255];
    sprintf(buffer, "%d", instructionNode->getNodeId());
    if (currentNode != NULL) {
        currentNode->addNodeEdge(new Edge(std::string(buffer)));
    }

    return instructionNode;
}

Node*
RocketShip::processDefaultInstruction(Node* instructionNode,
                                      Node* currentNode,
                                      Instruction* instruction)
{
    /**
     * Default instruction handling is of the format:
     * <opcode_name> <operand(0)> <operand(1)> <operand(n)>
     * Any instructions that don't have specific processing associated
     * will use this method.
     */
    std::string label = instruction->getOpcodeName();

    // Append each operand's name, constant value or type name
    for (unsigned int i = 0; i < instruction->getNumOperands(); i++) {
        label = label + " "
            + getValueName(instruction->getOperand(i));
    }

    _nodes.push_back(instructionNode);
            
    instructionNode->setNodeLabel(label);
    char buffer[255];
    sprintf(buffer, "%d", instructionNode->getNodeId());

    if (currentNode != NULL) {
        currentNode->addNodeEdge(new Edge(std::string(buffer)));
    } 
    return instructionNode;
}

std::string
RocketShip::getValueName(Value* value)
{
    std::string result;
    
    if (value->hasName()) {
        return value->getName();
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

/**
 * These are required by LLVM for each pass that's defined.
 * ID is assigned at runtime, but needs an initial assignment.
 * RegisterPass<T> registers the actual pass with the optimizer so it
 * is available to be called.
 */
char RocketShip::ID = 0;
static RegisterPass<RocketShip>
Y("rocketship", "RocketShip Pass");
