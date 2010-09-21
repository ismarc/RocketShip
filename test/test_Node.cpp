#include "gtest/gtest.h"

#include "../Node.h"
#include "llvm/LLVMContext.h"

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

TEST(NodeTest, NullInstructionTest)
{

}
