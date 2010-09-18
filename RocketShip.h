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
        static std::string getValueName(Value* value);
        
    private:
        /**
         * Generates the nodes and edges for the function and emits them to the
         * output filestream.  This processes a single function at a time.
         * @param F The function to process.
         */
        void processFunction(Function &F);
        /**
         * Generates the nodes and edges for a block.  This processes a single block
         * at a time.
         * @param bblock The LLVM representation of the block to process.
         * @param block The internal representation of the block.
         */
        void processBlock(BasicBlock* bblock, pBlock block);
        /**
         * Populates node data based on the supplied instruction.
         * @param instruction The LLVM representation of the instruction to process.
         * @param node The internal representation of a node to track.
         */
        void processInstruction(Instruction* instruction, pNode node);

        /**
         * Outputs a single node to the output filestream base on the node type and
         * associated edges.
         * @param node The node to emit.
         */
        void emitNode(Node* node);

        /**
         * Determines the string label to use for the supplied instruction.
         * @param instruction The instruction to determine the label for.
         */
        std::string getLabelForNode(Instruction* instruction);
        /**
         * Determines the appropriate label to display for a call instruction.
         * @param instruction the call instruction to determine the label for.
         */
        std::string getCallInstructionLabel(CallInst* instruction);
        /**
         * Determines the appropriate label to display for a switch instruction.
         * @param instruction the switch instruction to determine the label for.
         */
        std::string getSwitchInstLabel(SwitchInst* instruction);
        /**
         * Determines the appropriate label to display for a store isntruction.
         * @param instruction the store instruction to determine the label for.
         */
        std::string getStoreInstLabel(StoreInst* instruction);
        /**
         * Determines the appropriate label to display for a conditional branch
         * instruction.
         * @param instruction the branch instruction to determine the label for.
         */
        std::string getConditionalBranchLabel(BranchInst* instruction);
        /**
         * Determines the appropriate label to display for an invoke instruction.
         * @param instruction The invoke instruction to determine the label for.
         */
        std::string getInvokeInstLabel(InvokeInst* instruction);
        /**
         * Converts a mangled (C++) symbol name to the unmangled version if it
         * is a C++ mangled symbol name.  Otherwise, returns the supplied name
         * unchanged.
         * @param name The string to demangle.
         */
        std::string getDemangledName(std::string name);

        /**
         * Shared pointer collection of Node objects.
         */
        std::vector<pNode> _pnodes;

        /**
         * Stores the next id to use for a node.  Since few nodes will have unique names,
         * and DOT files require each node to have a unique name, the name of the node is
         * the next available integer id.
         */
        int _nodeId;
        /**
         * Stores the next id to use for a block.  Currently unused but would allow
         * for grouping of blocks in the created graphs.
         */
        int _blockId;
        /**
         * Stores the LLVM representation of blocks mapped to the internal
         * representation of blocks.  Provides easy access for determining linkage
         * between blocks.
         */
        std::map<BasicBlock*, pBlock> _blocks;
        /**
         * The output filestream to send graph data to.
         */
        std::ofstream _outputFile;
    };
}
