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

TEST_F(DmaChainIteratorTest, endlessChain1Test) {
  uint32_t chain[] {
    0x00000008u, 0x0000000Cu, 0x00000004u, 0x00000010u, 0x00000000u
  };
  unsigned long* buffer = nullptr;
  int bufferSize = 0;

  DmaChainIterator<0x200000u> psxIt((unsigned long*)chain, 0);
  EXPECT_TRUE(psxIt.readNext(&buffer, bufferSize));
  EXPECT_EQ((int)0, bufferSize);
  EXPECT_TRUE(psxIt.readNext(&buffer, bufferSize));
  EXPECT_EQ((int)0, bufferSize);
  EXPECT_TRUE(psxIt.readNext(&buffer, bufferSize));
  EXPECT_EQ((int)0, bufferSize);
  EXPECT_TRUE(psxIt.readNext(&buffer, bufferSize));
  EXPECT_EQ((int)0, bufferSize);
  EXPECT_TRUE(psxIt.readNext(&buffer, bufferSize));
  EXPECT_EQ((int)0, bufferSize);
  EXPECT_FALSE(psxIt.readNext(&buffer, bufferSize));
}

TEST_F(DmaChainIteratorTest, endlessChain2Test) {
  uint32_t chain[] { // note: with only 'lower'/'greater' index history, this chain would loop until max counter
    0x00000008u, 0x0000000Cu, 0x00000004u, 0x00000010u, 0x00000018u, 0x0000001Cu, 0x00000014u, 0x00000010u
  };
  unsigned long* buffer = nullptr;
  int bufferSize = 0;

  int retry = 0;
  DmaChainIterator<0x200000u> psxIt((unsigned long*)chain, 0);
  while (psxIt.readNext(&buffer, bufferSize) && ++retry < 20) {
    EXPECT_EQ((int)0, bufferSize);
  }
  EXPECT_TRUE(retry < 20);
}

TEST_F(DmaChainIteratorTest, endlessChain3Test) {
  uint32_t chain[] {
    0x0000001Cu, 0x0000000Cu, 0x00000004u, 0x00000010u, 0x00000018u, 0x00000008u, 0x00000014u, 0x00000008u
  };
  unsigned long* buffer = nullptr;
  int bufferSize = 0;

  int retry = 0;
  DmaChainIterator<0x200000u> psxIt((unsigned long*)chain, 0);
  while (psxIt.readNext(&buffer, bufferSize) && ++retry <= psxIt.maxCounter() + 1) {
    EXPECT_EQ((int)0, bufferSize);
  }
  EXPECT_TRUE(retry <= psxIt.maxCounter());
}

TEST_F(DmaChainIteratorTest, selfRefItemTest) {
  uint32_t chain[] { 0x01000000u, 42u };
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
