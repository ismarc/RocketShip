#include "gtest/gtest.h"

#include "../Block.h"

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
