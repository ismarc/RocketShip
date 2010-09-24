#include "gtest/gtest.h"

#include "../Block.h"
#include "llvm/LLVMContext.h"
#include "llvm/Instructions.h"

TEST(BlockTest, BlockConstructor)
{
    Block block(0, "test_block");
    EXPECT_EQ(0, block.getId());
    EXPECT_EQ("test_block", block.getLabel());
}

TEST(BlockTest, NodeConstructorDefaultLabel)
{
    Block block(0);
    EXPECT_EQ(0, block.getId());
    EXPECT_EQ("", block.getLabel());
}

TEST(BlockTest, BlockLabels)
{
    Block block(0, "test_block");

    block.setId(1);
    ASSERT_EQ(1, block.getId());
    block.setLabel("new_label");
    ASSERT_EQ("new_label", block.getLabel());
}

TEST(BlockTest, GetEmptyNodes)
{
    Block block(0);

    ASSERT_EQ(0, block.getNodes().size());
}

TEST(BlockTest, AppendNode)
{
    Block block(0);
    pNode node(new Node(1));
    block.appendNode(node);
    ASSERT_EQ(1, block.getNodes().size());
    ASSERT_EQ(1, block.getNodes()[0]->getNodeId());
}

TEST(BlockTest, FindEdgeOneDeep)
{
    int blockId = 0;
    int nodeId = 1;
    pBlock block(new Block(blockId));
    pNode node(new Node(nodeId));
    llvm::LLVMContext context;
    llvm::BasicBlock* source = llvm::BasicBlock::Create(context);
    llvm::BasicBlock* target = llvm::BasicBlock::Create(context);
    llvm::BranchInst* instruction = llvm::BranchInst::Create(target);
    std::map<llvm::BasicBlock*, pBlock> blocks;
    blocks.insert(std::pair<llvm::BasicBlock*, pBlock>(target, block));
    block->appendNode(node);
    source->getInstList().push_back(instruction);
    node->setInstruction(instruction);
    node->setNodeLabel("x");

    ASSERT_EQ(nodeId, block->findEdge(target, blocks));
}

TEST(BlockTest, FindEdgeTwoDeep)
{
    int blockId = 0;
    int node_one_id = 1;
    int node_two_id = 2;
    pBlock block(new Block(blockId));
    pNode node_one(new Node(node_one_id));
    pNode node_two(new Node(node_two_id));

    llvm::LLVMContext context;
    llvm::BasicBlock* source = llvm::BasicBlock::Create(context);
    llvm::BasicBlock* target = llvm::BasicBlock::Create(context);
    llvm::BranchInst* instruction = llvm::BranchInst::Create(target);
    source->getInstList().push_back(instruction);
    node_one->setInstruction(instruction);
    node_two->setInstruction(instruction);
    node_two->setNodeLabel("x");
    std::map<llvm::BasicBlock*, pBlock> blocks;
    blocks.insert(std::pair<llvm::BasicBlock*, pBlock>(target, block));
    block->appendNode(node_one);
    block->appendNode(node_two);

    ASSERT_EQ(node_two_id, block->findEdge(target, blocks));
}

TEST(BlockTest, FindEdgeRecursive)
{
    // To recurse, need two entries in block map, no instructions in
    // the block that have a name, a branch instruction to the second
    // block and a named instruction in the second block.  When
    // calling findEdge with the first block, the id of the
    // instruction with a name in the second block should be returned.

    llvm::LLVMContext context;
    llvm::BasicBlock* fbblock = llvm::BasicBlock::Create(context);
    llvm::BasicBlock* sbblock = llvm::BasicBlock::Create(context);
    llvm::BranchInst* finstruction = llvm::BranchInst::Create(sbblock);
    llvm::BranchInst* sinstruction = llvm::BranchInst::Create(fbblock);

    pBlock fblock(new Block(0));
    pBlock sblock(new Block(1));
    pNode fnode(new Node(0));
    pNode snode(new Node(1));

    fbblock->getInstList().push_back(finstruction);
    sbblock->getInstList().push_back(sinstruction);
    fblock->appendNode(fnode);
    sblock->appendNode(snode);
    fnode->setInstruction(finstruction);
    snode->setInstruction(sinstruction);
    snode->setNodeLabel("test_label");

    std::map<llvm::BasicBlock*, pBlock> blocks;
    blocks.insert(std::pair<llvm::BasicBlock*, pBlock>(fbblock, fblock));
    blocks.insert(std::pair<llvm::BasicBlock*, pBlock>(sbblock, sblock));
    ASSERT_EQ(1, fblock->findEdge(fbblock, blocks));
}

TEST(BlockTest, ProcessNodesContiguous)
{
    pBlock block(new Block(0));
    pNode node_one(new Node(0));
    pNode node_two(new Node(1));
    pNode node_three(new Node(2));
    pNode node_four(new Node(3));
    std::map<llvm::BasicBlock*, pBlock> blocks;
    node_one->setNodeLabel("node_one");
    node_two->setNodeLabel("node_two");
    node_three->setNodeLabel("node_three");
    block->appendNode(node_one);
    block->appendNode(node_two);
    block->appendNode(node_four);
    block->appendNode(node_three);
    block->processNodes(blocks);
    ASSERT_EQ(0, node_four->getNodeEdges().size());
    ASSERT_EQ(0, node_three->getNodeEdges().size());
    ASSERT_EQ(1, node_two->getNodeEdges().size());
    ASSERT_EQ(1, node_one->getNodeEdges().size());
    ASSERT_EQ("1", node_one->getNodeEdges()[0]->getId());
    ASSERT_EQ("2", node_two->getNodeEdges()[0]->getId());
}

TEST(BlockTest, ProcessNodesBlockEdges)
{
    // Block 1 -> unconditional branch to bblock 1, with label
    // Block 2 -> unconditional branch to bblock 2, no label
    // Block 3 -> blank node with label
    pBlock block_one(new Block(0));
    pBlock block_two(new Block(1));
    pBlock block_three(new Block(2));
    pNode node_one(new Node(0));
    pNode node_two(new Node(1));
    pNode node_three(new Node(2));

    llvm::LLVMContext context;
    llvm::BasicBlock* bblock_one = llvm::BasicBlock::Create(context);
    llvm::BasicBlock* bblock_two = llvm::BasicBlock::Create(context);
    llvm::BranchInst* instruction_one = llvm::BranchInst::Create(bblock_one);
    llvm::BranchInst* instruction_two = llvm::BranchInst::Create(bblock_two);

    node_one->setInstruction(instruction_one);
    node_one->setNodeLabel("node_one");
    node_two->setInstruction(instruction_two);
    node_three->setNodeLabel("node_three");
    block_one->appendNode(node_one);
    block_two->appendNode(node_two);
    block_three->appendNode(node_three);

    std::map<llvm::BasicBlock*, pBlock> blocks;
    blocks.insert(std::pair<llvm::BasicBlock*, pBlock>(bblock_one, block_two));
    blocks.insert(std::pair<llvm::BasicBlock*, pBlock>(bblock_two, block_three));
    block_one->processNodes(blocks);
    block_two->processNodes(blocks);
    block_three->processNodes(blocks);

    ASSERT_EQ(1, node_one->getNodeEdges().size());
    ASSERT_EQ("2", node_one->getNodeEdges()[0]->getId());
    ASSERT_EQ(0, node_two->getNodeEdges().size());
    ASSERT_EQ(0, node_three->getNodeEdges().size());
}
