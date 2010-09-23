#include "gtest/gtest.h"

#include "../Node.h"
#include "llvm/LLVMContext.h"
#include "llvm/Instructions.h"
#include "llvm/Function.h"

TEST(NodeTest, NodeConstructor)
{
    Node node(1, Node::START);
    EXPECT_EQ(1, node.getNodeId());
    EXPECT_EQ(Node::START, node.getNodeType());
}

TEST(NodeTest, NodeConstructorDefaultType)
{
    Node node(1);
    EXPECT_EQ(1, node.getNodeId());
    EXPECT_EQ(Node::ACTIVITY, node.getNodeType());
}

TEST(NodeTest, NodeConstructorDefaults)
{
    Node node;
    EXPECT_EQ(0, node.getNodeId());
    EXPECT_EQ(Node::ACTIVITY, node.getNodeType());
}

TEST(NodeTest, NodeLabels)
{
    Node node(1, Node::ACTIVITY);
    node.setNodeLabel("test_label");
    node.setNodeName("test_name");
    ASSERT_EQ("test_label", node.getNodeLabel());
    ASSERT_EQ("test_name", node.getNodeName());
}

TEST(NodeTest, NodeEdge)
{
    Node node;
    node.addNodeEdge(new Edge("test_edge"));
    std::vector<Edge*> edges = node.getNodeEdges();
    ASSERT_EQ(1, edges.size());
    ASSERT_EQ("test_edge", edges[0]->getId());
}

TEST(NodeTest, NullInstructionBlockEdges)
{
    Node node;
    ASSERT_EQ(0, node.getBlockEdges().size());
}

TEST(NodeTest, NoBlockEdgesInstruction)
{
    Node node;
    // Use an Allocation instruction since they won't ever result in
    // block edges (branches).
    llvm::LLVMContext context;
    llvm::AllocaInst* instruction = new llvm::AllocaInst(llvm::Type::getInt32Ty(context));
    node.setInstruction(instruction);
    ASSERT_EQ(0, node.getBlockEdges().size());
}

TEST(NodeTest, UnconditionalBranchInstruction)
{
    Node node;
    llvm::LLVMContext context;
    llvm::BasicBlock* source = llvm::BasicBlock::Create(context);
    llvm::BasicBlock* target = llvm::BasicBlock::Create(context);
    llvm::BranchInst* instruction = llvm::BranchInst::Create(target);
    source->getInstList().push_back(instruction);
    node.setInstruction(instruction);
    ASSERT_EQ(1, node.getBlockEdges().size());
    ASSERT_EQ(target, node.getBlockEdges()["x"]);
}

TEST(NodeTest, ConditionalBranchInstruction)
{
    Node node;
    llvm::LLVMContext context;
    llvm::BasicBlock* source = llvm::BasicBlock::Create(context);
    llvm::BasicBlock* true_target = llvm::BasicBlock::Create(context);
    llvm::BasicBlock* false_target = llvm::BasicBlock::Create(context);
    llvm::ConstantInt* v1 = llvm::ConstantInt::get(llvm::Type::getInt32Ty(context),
                                                   1, false);
    llvm::ConstantInt* v2 = llvm::ConstantInt::get(llvm::Type::getInt32Ty(context),
                                                   0, false);
    llvm::ICmpInst* comparison = new llvm::ICmpInst(llvm::CmpInst::ICMP_EQ, v1, v2);
    llvm::BranchInst* instruction = llvm::BranchInst::Create(true_target,
                                                             false_target,
                                                             comparison);
    source->getInstList().push_back(instruction);
    node.setInstruction(instruction);
    ASSERT_EQ(2, node.getBlockEdges().size());
    ASSERT_EQ(true_target, node.getBlockEdges()["true"]);
    ASSERT_EQ(false_target, node.getBlockEdges()["false"]);
}

TEST(NodeTest, SwitchInstruction)
{
    Node node;
    llvm::LLVMContext context;
    llvm::BasicBlock* source = llvm::BasicBlock::Create(context);
    llvm::BasicBlock* default_target = llvm::BasicBlock::Create(context);
    llvm::BasicBlock* target_one = llvm::BasicBlock::Create(context);
    llvm::BasicBlock* target_two = llvm::BasicBlock::Create(context);
    llvm::ConstantInt* v1 = llvm::ConstantInt::get(llvm::Type::getInt32Ty(context),
                                                   0, false);
    llvm::ConstantInt* v2 = llvm::ConstantInt::get(llvm::Type::getInt32Ty(context),
                                                   1, false);
    llvm::ConstantInt* v3 = llvm::ConstantInt::get(llvm::Type::getInt32Ty(context),
                                                   2, false);
    llvm::SwitchInst* instruction = llvm::SwitchInst::Create(v1, default_target,
                                                             2);
    instruction->addCase(v2, target_one);
    instruction->addCase(v3, target_two);
    source->getInstList().push_back(instruction);
    node.setInstruction(instruction);

    ASSERT_EQ(3, node.getBlockEdges().size());
    ASSERT_EQ(default_target, node.getBlockEdges()["default"]);
    ASSERT_EQ(target_one, node.getBlockEdges()["1"]);
    ASSERT_EQ(target_two, node.getBlockEdges()["2"]);
}

TEST(NodeTest, InvokeInstruction)
{
    Node node;
    llvm::LLVMContext context;
    llvm::BasicBlock* source = llvm::BasicBlock::Create(context);
    llvm::BasicBlock* normal_target = llvm::BasicBlock::Create(context);
    llvm::BasicBlock* unwind_target = llvm::BasicBlock::Create(context);
    llvm::FunctionType* function_type = llvm::FunctionType::get(llvm::Type::getVoidTy(context),false);
    llvm::Function* function = llvm::Function::Create(function_type, llvm::GlobalValue::InternalLinkage);
    llvm::SmallVector<llvm::Value*, 8> args;
    llvm::InvokeInst* instruction = llvm::InvokeInst::Create(function,
                                                             normal_target,
                                                             unwind_target,
                                                             args.begin(),
                                                             args.end());
    source->getInstList().push_back(instruction);
    node.setInstruction(instruction);
    ASSERT_EQ(2, node.getBlockEdges().size());
    ASSERT_EQ(unwind_target, node.getBlockEdges()["unwind"]);
    ASSERT_EQ(normal_target, node.getBlockEdges()[""]);
}
