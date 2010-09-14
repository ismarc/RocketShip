#include "Node.h"
#include "Block.h"

#include <vector>
#include <map>
#include <fstream>

#include "llvm/Function.h"
#include "llvm/Instructions.h"
#include "llvm/Module.h"
#include "llvm/Pass.h"

using namespace llvm;

namespace rocketship {
    struct RocketShip : public ModulePass {
    public:
        /**
         * Each ModulePass has its own unique ID assigned at runtime, used internally
         * by LLVM.
         */
        static char ID;

        /**
         * Construtor, pass everything up to parent class.
         */
        RocketShip() : ModulePass(&ID), _nodeId(0) {}

        /**
         * Called for each module processed by the optimizer.  Each module has its own 
         * file to output to and its own distinct graph.  The graph is generated with
         * each function as its own distinct section of the graph.
         * TODO: Make each function its own graph file.
         * @param M The module to process.
         */
        virtual bool runOnModule(Module &M);

    private:
        /**
         * Generates the nodes and edges for the function and emits them to the
         * output filestream.
         * @param F The function to process.
         */
        void processFunction(Function &F);
        void processBlock(BasicBlock* bblock, pBlock block);
        void processInstruction(Instruction* instruction, pNode node);
        void processFunctionOld(Function &F);
        /**
         * Generates the node structure of the given function with the supplied node
         * as the base node.  Each block in the function is processed in the order it
         * appears in the bytecode and linked together by branch and unconditional
         * branch instructions.
         * @param funcNode The node object representing the base node to use.
         * @param F The function to generate the node structure for.
         */
        void generateNodeStructure(Node* funcNode, Function &F);

        /**
         * Outputs a single node to the output filestream base on the node type and
         * associated edges.
         * @param node The node to emit.
         */
        void emitNode(Node* node);

        void processFunctionBlocks(Node* funcNode, BasicBlock* block);
        /**
         * Processes a single block for a function.  Creates a node for each appropriate
         * instruction in the block and edges connecting them.  The node is appended to
         * the module _nodes list for emitting later.
         * @param funcNode The base node of the function.
         * @param currentNode The current node when the block begins processing.
         * @param block The code block to process.
         */
        void processFunctionBlock(Node* funcNode, Node* currentNode, BasicBlock* block);

        /**
         * Handles a call instruction encountered in the function block.  Call
         * instructions have special display formatting to more easily present the
         * actual call that is being made.
         * @param instructionNode The node representing the call instruction.
         * @param currentNode The current node prior to the call instruction.
         * @param callInst The internal representation of the call instruction.
         * @return The next node to use in the graph.
         */
        Node* processCallInstruction(Node* instructionNode,
                                     Node* currentNode,
                                     CallInst* callInst);

        std::string getConditionalBranchLabel(BranchInst* instruction);
        std::string getInvokeInstLabel(InvokeInst* instruction);
        /**
         * Handles a conditional branch instruction encountered in the function block.
         * Conditional branch instructions represent a branch based on a comparison that
         * resolves to a true or false result.  Both possible subsequent nodes need to
         * be added to the graph.
         * @param instructionNode The node representating the conditional branch.
         * @param currentNode The current node prior to the branch instruction.
         * @param branchInst The internal representation of the branch instruction.
         * @param functionName The name of the function currently being processed
         *                     (used as part of uniquely identifying blocks).
         * @return The next node to use in the graph.
         */
        Node* processConditionalBranch(Node* instructionNode,
                                       Node* currentNode,
                                       BranchInst* branchInst,
                                       std::string functionName);

        /**
         * Handles an unconditional branch instruction (similar to a jump) in the
         * function block.  Unconditional branches are jumps to the label specified.
         * Displaying them within the graph is extraneous information, so the label is
         * pushed into the graph rather than the branch node.
         * @param instructionNode The node representating the unconditional branch.
         * @param currentNode The current node prior to the branch instruction.
         * @param branchInst The internal representation of the branch instruction.
         * @param functionName The name of the function currently being processed
         *                     (used as part of uniquely identifying blocks).
         * @return The next node to use in the graph.
         */
        Node* processUnconditionalBranch(Node* instructionNode,
                                         Node* currentNode,
                                         BranchInst* branchInst,
                                         std::string functionName);

        /**
         * Handles a store instruction in the function block.  Store instructions have
         * a special label display to remove ambiguities of what value is stored in which
         * location.
         * @param instructionNode The node representing the store instruction.
         * @param currentNode The current node prior to the store instruction.
         * @param storeInst The internal representation of the store instruction.
         * @return The next node to use in the graph.
         */
        Node* processStoreInstruction(Node* instructionNode,
                                      Node* currentNode,
                                      StoreInst* storeInst);

        /**
         * Handles switch instructions in the function block.  A
         * switch instruction indicates a multi-path branch based on
         * the input value.  The decision node with indicate the value
         * that is checked and the edge will indicate the branch that
         * is followed if the value matches.
         * @param instructionNode The node representing the switch instruction.
         * @param currentNode The current node prior to the switch instruction.
         * @param switchInst The internal representation of the switch instruction
         * @param functionName The name of the function currently being processed
         *                     (used as part of uniquely identifying blocks).
         * @return The next node to use in the graph.
         */
        Node* processSwitchInstruction(Node* instructionNode,
                                       Node* currentNode,
                                       SwitchInst* switchInst,
                                       std::string functionName);
        /**
         * Handles all instructions that do not have a special case.  A general
         * instruction is considered an activity and has a label that is defined as:
         * opcode operand(0) operand(1) operand(n).
         * @param instructionNode The node representing the instruction.
         * @param currentNode The current node prior to the instruction.
         * @param instruction The internal representation of the instruction.
         * @return The next node to use in the graph.
         */
        Node* processDefaultInstruction(Node* instructionNode,
                                        Node* currentNode,
                                        Instruction* instruction);

        /**
         * LLVM bytecode typically gets compiled down using temporary
         * Values (most 'things' derive from Value).  Temporary Values
         * don't have a name associated with them.  Some operations,
         * such as sext (sign extend) and load (load data from memory)
         * and bitcast (convert types) don't alter the fundamental
         * behavior or stored values.  This traverses the hierarchy
         * of instructions until it finds a Value with a name.  If 
         * an instruction that modifies values is encountered, it
         * returns an empty string.
         */
        std::string getValueName(Value* value);

        /**
         * Stores the list of nodes to draw in the graph.  DOT file formats use a
         * labelling mechanism so the ordering of entries in _nodes doesn't matter.  This
         * simplifies the process of building the graph since the actual graph structure
         * doesn't need represented.  Only a list of nodes and the edges the nodes have is
         * needed.
         */
        std::vector<Node*> _nodes;

        /**
         * Stores the list of blocks that have been processed for the
         * current function.
         */
        std::map<BasicBlock*, int> _processedBlocks;

        std::map<int, BasicBlock*> _pendingEdges;
        /**
         * Stores the next id to use for a node.  Since few nodes will have unique names,
         * and DOT files require each node to have a unique name, the name of the node is
         * the next available integer id.
         */
        int _nodeId;
        int _blockId;
        std::map<BasicBlock*, pBlock> _blocks;
        /**
         * The output filestream to send graph data to.
         */
        std::ofstream _outputFile;
    };
}
