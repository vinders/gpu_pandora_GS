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
#include <config/types.h>

using namespace config;

class TypesTest : public testing::Test {
public:
protected:
  //static void SetUpTestCase() {}
  //static void TearDownTestCase() {}

  void SetUp() override {}
  void TearDown() override {}
};


TEST_F(TypesTest, charCodeArrayFromUtf8) {
  CharCodeArray empty;
  EXPECT_TRUE(empty.empty());
  EXPECT_EQ(size_t{0}, empty.size());
  EXPECT_TRUE(empty.value() == nullptr);

  CharCodeArray val1("abc", 3);
  EXPECT_FALSE(val1.empty());
  EXPECT_EQ(size_t{3}, val1.size());
  ASSERT_TRUE(val1.value() != nullptr);
  EXPECT_EQ((uint32_t)'a', val1.value()[0]);
  EXPECT_EQ((uint32_t)'b', val1.value()[1]);
  EXPECT_EQ((uint32_t)'c', val1.value()[2]);
}
  
TEST_F(TypesTest, charCodeArrayFromUtf16) {
# ifdef _WINDOWS
  CharCodeArray val2(L"abc", 3);
  EXPECT_FALSE(val2.empty());
  EXPECT_EQ(size_t{3}, val2.size());
  ASSERT_TRUE(val2.value() != nullptr);
  EXPECT_EQ((uint32_t)'a', val2.value()[0]);
  EXPECT_EQ((uint32_t)'b', val2.value()[1]);
  EXPECT_EQ((uint32_t)'c', val2.value()[2]);
# endif
  CharCodeArray val3(u"abc", 3);
  EXPECT_FALSE(val3.empty());
  EXPECT_EQ(size_t{3}, val3.size());
  ASSERT_TRUE(val3.value() != nullptr);
  EXPECT_EQ((uint32_t)'a', val3.value()[0]);
  EXPECT_EQ((uint32_t)'b', val3.value()[1]);
  EXPECT_EQ((uint32_t)'c', val3.value()[2]);

# ifdef _WINDOWS
  const wchar_t* complexUW = L"\x00E9#\x0080*\x00E0 _\x00B0(\x00AB\x00F9\x00BB)}";
  CharCodeArray valComplex2(complexUW, wcslen(complexUW));
  EXPECT_FALSE(valComplex2.empty());
  EXPECT_EQ(size_t{14}, valComplex2.size());
  ASSERT_TRUE(valComplex2.value() != nullptr);
  EXPECT_EQ((uint32_t)0xE9, valComplex2.value()[0]);
  EXPECT_EQ((uint32_t)0x23, valComplex2.value()[1]);
  EXPECT_EQ((uint32_t)0x80, valComplex2.value()[2]);
  EXPECT_EQ((uint32_t)0x2A, valComplex2.value()[3]);
  EXPECT_EQ((uint32_t)0xE0, valComplex2.value()[4]);
  EXPECT_EQ((uint32_t)' ', valComplex2.value()[5]);
  EXPECT_EQ((uint32_t)'_', valComplex2.value()[6]);
  EXPECT_EQ((uint32_t)0xB0, valComplex2.value()[7]);
  EXPECT_EQ((uint32_t)'(', valComplex2.value()[8]);
  EXPECT_EQ((uint32_t)0xAB, valComplex2.value()[9]);
  EXPECT_EQ((uint32_t)0xF9, valComplex2.value()[10]);
  EXPECT_EQ((uint32_t)0xBB, valComplex2.value()[11]);
  EXPECT_EQ((uint32_t)')', valComplex2.value()[12]);
  EXPECT_EQ((uint32_t)'}', valComplex2.value()[13]);
# endif
  const char16_t* complexU16 = u"\x00E9#\x0080*\x00E0 _\x00B0(\x00AB\x00F9\x00BB)}";
  CharCodeArray valComplex3(complexU16, 14);
  EXPECT_FALSE(valComplex3.empty());
  EXPECT_EQ(size_t{14}, valComplex3.size());
  ASSERT_TRUE(valComplex3.value() != nullptr);
  EXPECT_EQ((uint32_t)0xE9, valComplex3.value()[0]);
  EXPECT_EQ((uint32_t)0x23, valComplex3.value()[1]);
  EXPECT_EQ((uint32_t)0x80, valComplex3.value()[2]);
  EXPECT_EQ((uint32_t)0x2A, valComplex3.value()[3]);
  EXPECT_EQ((uint32_t)0xE0, valComplex3.value()[4]);
  EXPECT_EQ((uint32_t)' ', valComplex3.value()[5]);
  EXPECT_EQ((uint32_t)'_', valComplex3.value()[6]);
  EXPECT_EQ((uint32_t)0xB0, valComplex3.value()[7]);
  EXPECT_EQ((uint32_t)'(', valComplex3.value()[8]);
  EXPECT_EQ((uint32_t)0xAB, valComplex3.value()[9]);
  EXPECT_EQ((uint32_t)0xF9, valComplex3.value()[10]);
  EXPECT_EQ((uint32_t)0xBB, valComplex3.value()[11]);
  EXPECT_EQ((uint32_t)')', valComplex3.value()[12]);
  EXPECT_EQ((uint32_t)'}', valComplex3.value()[13]);
}
