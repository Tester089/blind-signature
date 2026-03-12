#include <gtest/gtest.h>
#include "../core.h"

TEST(BigIntTest, AddBasic) {
  BigInt a(100), b(200);
  BigInt c = add(a, b);
  EXPECT_EQ(to_hex(c), to_hex(BigInt(300)));
}

TEST(BigIntTest, MulBasic) {
  BigInt a(12), b(13);
  BigInt c = mul(a, b);
  EXPECT_EQ(to_hex(c), to_hex(BigInt(156)));
}