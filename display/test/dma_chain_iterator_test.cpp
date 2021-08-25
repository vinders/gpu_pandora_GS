/*******************************************************************************
Pandora GS - PSEmu-compatible GPU driver
Copyright (C) 2021  Romain Vinders

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation, version 2 of the License.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details (LICENSE file).
*******************************************************************************/
#include <gtest/gtest.h>
#include <vector>
#include <display/dma_chain_iterator.h>

using namespace display;

class DmaChainIteratorTest : public testing::Test {
public:
protected:
  //static void SetUpTestCase() {}
  //static void TearDownTestCase() {}

  void SetUp() override {}
  void TearDown() override {}
};


TEST_F(DmaChainIteratorTest, accessorsTest) {
  unsigned long* buffer = nullptr;
  int bufferSize = 0;

  DmaChainIterator<0x200000u> psxIt;
  EXPECT_EQ((unsigned long)0xFFFFFFu, psxIt.endIndexBits());
  EXPECT_EQ((unsigned long)0x1FFFFCu, psxIt.addressMask());
  EXPECT_EQ((unsigned long)((0x200000u - psxBiosSize()) >> 2) + 1u, psxIt.maxCounter());
  EXPECT_FALSE(psxIt.readNext(&buffer, bufferSize));

  DmaChainIterator<0x800000u> znIt;
  EXPECT_EQ((unsigned long)0xFFFFFFu, znIt.endIndexBits());
  EXPECT_EQ((unsigned long)0x7FFFFCu, znIt.addressMask());
  EXPECT_EQ((unsigned long)((0x800000u - psxBiosSize()) >> 2) + 1u, znIt.maxCounter());
  EXPECT_FALSE(znIt.readNext(&buffer, bufferSize));
}

TEST_F(DmaChainIteratorTest, emptyChainTest) {
  uint32_t chain[] { 0 };
  unsigned long* buffer = nullptr;
  int bufferSize = 0;

  DmaChainIterator<0x200000u> psxIt((unsigned long*)chain, 0xFFFFFFFF);
  EXPECT_FALSE(psxIt.readNext(&buffer, bufferSize));
}

TEST_F(DmaChainIteratorTest, singleItemChainTest) {
  uint32_t chain[] { 0x01FFFFFFu, 42u };
  unsigned long* buffer = nullptr;
  int bufferSize = 0;

  DmaChainIterator<0x200000u> psxIt((unsigned long*)chain, 0);
  EXPECT_TRUE(psxIt.readNext(&buffer, bufferSize));
  ASSERT_EQ((int)1, bufferSize);
  ASSERT_TRUE(buffer != nullptr);
  EXPECT_EQ((unsigned long)42u, *buffer);
  EXPECT_FALSE(psxIt.readNext(&buffer, bufferSize));
}

TEST_F(DmaChainIteratorTest, validChainTest) {
  uint32_t chain[] {
    0x01FFFFFFu, 1, 0x02000000u, 42u, 0
  };
  unsigned long* buffer = nullptr;
  int bufferSize = 0;

  DmaChainIterator<0x200000u> psxIt((unsigned long*)chain, 2*sizeof(uint32_t));
  EXPECT_TRUE(psxIt.readNext(&buffer, bufferSize));
  ASSERT_EQ((int)2, bufferSize);
  ASSERT_TRUE(buffer != nullptr);
  EXPECT_EQ((unsigned long)42u, *buffer);
  EXPECT_EQ((unsigned long)0, buffer[1]);
  EXPECT_TRUE(psxIt.readNext(&buffer, bufferSize));
  ASSERT_EQ((int)1, bufferSize);
  ASSERT_TRUE(buffer != nullptr);
  EXPECT_EQ((unsigned long)1, *buffer);
  EXPECT_FALSE(psxIt.readNext(&buffer, bufferSize));
}

TEST_F(DmaChainIteratorTest, nullChainTest) {
  unsigned long* buffer = nullptr;
  int bufferSize = 0;

  DmaChainIterator<0x200000u> psxIt(nullptr, 42);
  EXPECT_FALSE(psxIt.readNext(&buffer, bufferSize));
}

TEST_F(DmaChainIteratorTest, selfRefItemTest) {
  uint32_t chain[] { 0x01000000u, 42u }; // self-referenced: faster detection with 'lower'/'greater' history
  unsigned long* buffer = nullptr;
  int bufferSize = 0;

  DmaChainIterator<0x200000u> psxIt((unsigned long*)chain, 0);
  EXPECT_TRUE(psxIt.readNext(&buffer, bufferSize));
  ASSERT_EQ((int)1, bufferSize);
  ASSERT_TRUE(buffer != nullptr);
  EXPECT_EQ((unsigned long)42u, *buffer);
  EXPECT_FALSE(psxIt.readNext(&buffer, bufferSize));

  uint32_t chain2[] { 0x01000000u, 42u, 0x02000000u, 42u, 0 };
  DmaChainIterator<0x200000u> psxIt2((unsigned long*)chain2, 2*sizeof(uint32_t));
  EXPECT_TRUE(psxIt2.readNext(&buffer, bufferSize));
  ASSERT_EQ((int)2, bufferSize);
  ASSERT_TRUE(buffer != nullptr);
  EXPECT_EQ((unsigned long)42u, *buffer);
  EXPECT_EQ((unsigned long)0, buffer[1]);
  EXPECT_TRUE(psxIt2.readNext(&buffer, bufferSize));
  ASSERT_EQ((int)1, bufferSize);
  ASSERT_TRUE(buffer != nullptr);
  EXPECT_EQ((unsigned long)42u, *buffer);
  EXPECT_FALSE(psxIt2.readNext(&buffer, bufferSize));
}

TEST_F(DmaChainIteratorTest, endlessChain1Test_simple) {
  uint32_t chain[] { // easily detected with both techniques
    0x00000008u, 0x0000000Cu, 0x00000004u, 0x00000010u, 0x00000000u
  };
  unsigned long* buffer = nullptr;
  int bufferSize = 0;

  int itemsRead = 0;
  DmaChainIterator<0x200000u> psxIt((unsigned long*)chain, 0);
  while (psxIt.readNext(&buffer, bufferSize) && ++itemsRead < 10) {
    EXPECT_EQ((int)0, bufferSize);
  }
  EXPECT_TRUE(itemsRead >= sizeof(chain)/sizeof(*chain));
  EXPECT_TRUE(itemsRead < 10);
}

TEST_F(DmaChainIteratorTest, endlessChain2Test_alternateMoves) {
  uint32_t chain[] { // only detected with slower moving index (with 'lower'/'greater' history, this chain would loop until max counter)
    0x00000008u, 0x0000000Cu, 0x00000004u, 0x00000010u, 0x00000018u, 0x0000001Cu, 0x00000014u, 0x00000010u
  };
  unsigned long* buffer = nullptr;
  int bufferSize = 0;

  int itemsRead = 0;
  DmaChainIterator<0x200000u> psxIt((unsigned long*)chain, 0);
  while (psxIt.readNext(&buffer, bufferSize) && ++itemsRead < 30) {
    EXPECT_EQ((int)0, bufferSize);
  }
  EXPECT_TRUE(itemsRead >= sizeof(chain)/sizeof(*chain));
  EXPECT_TRUE(itemsRead < 30);
}

TEST_F(DmaChainIteratorTest, endlessChain3Test_minMaxScattered) {
  uint32_t chain[] { // only detected with slower moving index (with 'lower'/'greater' history, this chain would loop until max counter)
    0x0000001Cu, 0x0000000Cu, 0x00000004u, 0x00000010u, 0x00000018u, 0x00000008u, 0x00000014u, 0x00000008u
  };
  unsigned long* buffer = nullptr;
  int bufferSize = 0;

  int itemsRead = 0;
  DmaChainIterator<0x200000u> psxIt((unsigned long*)chain, 0);
  while (psxIt.readNext(&buffer, bufferSize) && ++itemsRead < 30) {
    EXPECT_EQ((int)0, bufferSize);
  }
  EXPECT_TRUE(itemsRead >= sizeof(chain)/sizeof(*chain));
  EXPECT_TRUE(itemsRead < 30);
}

#define __LONG_CHAIN_BYTE_SIZE 0x80000

TEST_F(DmaChainIteratorTest, endlessChain4Test_orderedLongChain) {
  uint32_t chainSize = (__LONG_CHAIN_BYTE_SIZE >> 2);
  std::vector<uint32_t> chain; // ordered: much faster detection with 'lower'/'greater' history
  for (uint32_t i = sizeof(uint32_t); i < __LONG_CHAIN_BYTE_SIZE; i += sizeof(uint32_t))
    chain.push_back(i);
  chain.push_back(0);

  unsigned long* buffer = nullptr;
  int bufferSize = 0;

  int itemsRead = 0;
  DmaChainIterator<0x200000u> psxIt((unsigned long*)&chain[0], 0);
  while (psxIt.readNext(&buffer, bufferSize) && ++itemsRead < 4*(int)chainSize) {
    EXPECT_EQ((int)0, bufferSize);
  }
  EXPECT_TRUE(itemsRead >= (int)chainSize);
  EXPECT_TRUE(itemsRead < 2*(int)chainSize);
}

TEST_F(DmaChainIteratorTest, endlessChain5Test_unorderedLongChain) {
  uint32_t chainSize = (__LONG_CHAIN_BYTE_SIZE >> 2);
  std::vector<uint32_t> chain; // unordered: only detected with slower moving index
  for (uint32_t i = sizeof(uint32_t); i <= __LONG_CHAIN_BYTE_SIZE/2; i += sizeof(uint32_t))
    chain.push_back(__LONG_CHAIN_BYTE_SIZE - i);
  chain.push_back(0);
  for (uint32_t i = sizeof(uint32_t); i < __LONG_CHAIN_BYTE_SIZE/2; i += sizeof(uint32_t))
    chain.push_back(__LONG_CHAIN_BYTE_SIZE/2 - i);

  unsigned long* buffer = nullptr;
  int bufferSize = 0;

  int itemsRead = 0;
  DmaChainIterator<0x200000u> psxIt((unsigned long*)&chain[0], 0);
  while (psxIt.readNext(&buffer, bufferSize) && ++itemsRead < 4*(int)chainSize) {
    EXPECT_EQ((int)0, bufferSize);
  }
  EXPECT_TRUE(itemsRead >= (int)chainSize);
  EXPECT_TRUE(itemsRead < 2*(int)chainSize);
}
