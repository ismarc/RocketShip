#include "gtest/gtest.h"

#include "../Edge.h"

TEST(EdgeTest, EdgeGetValues)
{
    Edge* edge = new Edge("test_id", "test_label");
    EXPECT_EQ("test_id", edge->getId());
    EXPECT_EQ("test_label", edge->getLabel());
}
