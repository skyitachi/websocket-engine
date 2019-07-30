//
// Created by skyitachi on 2019-07-30.
//
#include <gtest/gtest.h>
#include <ws/Buffer.h>

TEST(BufferBase, Case1) {
  Buffer buf(10);
  EXPECT_EQ(buf.readableBytes(), 0);
  buf.write("123", 3);
  EXPECT_EQ(buf.readableBytes(), 3);
  buf.write("123", 3);
  EXPECT_EQ(buf.readableBytes(), 6);
  char tmp[1024];
  buf.read(tmp, 6);
  EXPECT_EQ(std::string(tmp, 6), "123123");
  EXPECT_EQ(buf.readableBytes(), 0);
  
  buf.write("123456", 6);
  EXPECT_EQ(buf.readableBytes(), 6);
  // 不会触发扩容才对
  EXPECT_EQ(buf.size(), 10);
  buf.write("7890", 4);
  // 不会触发扩容才对
  EXPECT_EQ(buf.size(), 10);
  EXPECT_EQ(buf.readableBytes(), 10);
  
  buf.write("111213", 6);
  EXPECT_EQ(buf.readableBytes(), 16);
  EXPECT_EQ(buf.size(), 16);
  buf.read(tmp, 10);
  EXPECT_EQ(std::string(tmp, 10), "1234567890");
  
}

TEST(BufferAdvance, Case2) {
  Buffer buf(10);
  buf.write("123456", 6);
  char tmp[1024];
  buf.read(tmp, 3);
  buf.shrinkToFit();
  EXPECT_EQ(buf.readableBytes(), 3);
  EXPECT_EQ(buf.size(), 3);
  
  buf.read(tmp, 3);
  EXPECT_EQ(std::string(tmp, 3), "456");
}

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}


