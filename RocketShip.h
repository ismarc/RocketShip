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
         * output filestream.
         * @param F The function to process.
         */
        void processFunction(Function &F);
        void processBlock(BasicBlock* bblock, pBlock block);
        void processInstruction(Instruction* instruction, pNode node);

        /**
         * Outputs a single node to the output filestream base on the node type and
         * associated edges.
         * @param node The node to emit.
         */
        void emitNode(Node* node);

        std::string getConditionalBranchLabel(BranchInst* instruction);
        std::string getInvokeInstLabel(InvokeInst* instruction);

        std::string getDemangledName(std::string name);
        std::string getLabelForNode(Instruction* instruction);

        std::string getCallInstructionLabel(CallInst* instruction);
        std::string getSwitchInstLabel(SwitchInst* instruction);
        std::string getStoreInstLabel(StoreInst* instruction);

        std::vector<pNode> _pnodes;

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
