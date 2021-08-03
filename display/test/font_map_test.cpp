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
#include <display/font_map.h>

using namespace display;

class FontMapTest : public testing::Test {
public:
protected:
  //static void SetUpTestCase() {}
  //static void TearDownTestCase() {}

  void SetUp() override {}
  void TearDown() override {}
};


TEST_F(FontMapTest, descriptorAccessors) {
  CharDescriptor desc1;
  desc1.id(125u);
  desc1.x('\x10');
  desc1.y(20u);
  desc1.width('\x25');
  desc1.height('\x35');
  desc1.offsetX((int8_t)-5);
  desc1.offsetY('\x5');
  desc1.advanceX('\x22');
  desc1.advanceY('\x40');

  EXPECT_EQ((uint32_t)125u, desc1.id());
  EXPECT_EQ((uint8_t)'\x10', desc1.x());
  EXPECT_EQ((uint32_t)20u, desc1.y());
  EXPECT_EQ((uint8_t)'\x25', desc1.width());
  EXPECT_EQ((uint8_t)'\x35', desc1.height());
  EXPECT_EQ((int8_t)-5, desc1.offsetX());
  EXPECT_EQ((uint8_t)'\x5', desc1.offsetY());
  EXPECT_EQ((uint8_t)'\x22', desc1.advanceX());
  EXPECT_EQ((uint8_t)'\x40', desc1.advanceY());

  CharDescriptor desc2 = desc1;
  EXPECT_EQ((uint32_t)125u, desc2.id());
  EXPECT_EQ((uint8_t)'\x10', desc2.x());
  EXPECT_EQ((uint32_t)20u, desc2.y());
  EXPECT_EQ((uint8_t)'\x25', desc2.width());
  EXPECT_EQ((uint8_t)'\x35', desc2.height());
  EXPECT_EQ((int8_t)-5, desc2.offsetX());
  EXPECT_EQ((uint8_t)'\x5', desc2.offsetY());
  EXPECT_EQ((uint8_t)'\x22', desc2.advanceX());
  EXPECT_EQ((uint8_t)'\x40', desc2.advanceY());
}

struct FakeTexture final {
  uint32_t handle = 0;
};

TEST_F(FontMapTest, createFontMap) {
  FontMap<FakeTexture> defaultCtor;
  EXPECT_EQ((uint32_t)0, defaultCtor.spriteSheet().handle);
  EXPECT_EQ((size_t)0, defaultCtor.charCount());
  EXPECT_EQ((uint32_t)0, defaultCtor.baseLineOffset());
  EXPECT_TRUE(defaultCtor.find(0) == nullptr);

  FontMap<FakeTexture> initCtor1(FakeTexture{ 1 }, nullptr, 0, 0);
  EXPECT_EQ((uint32_t)1, initCtor1.spriteSheet().handle);
  EXPECT_EQ((size_t)0, initCtor1.charCount());
  EXPECT_EQ((uint32_t)0, initCtor1.baseLineOffset());
  EXPECT_TRUE(initCtor1.find(0) == nullptr);

  CharDescriptor descArray[3];
  descArray[0].id(125u);
  descArray[0].x('\x10');
  descArray[0].y(20u);
  descArray[0].width('\x25');
  descArray[0].height('\x35');
  descArray[0].offsetX((int8_t)-5);
  descArray[0].offsetY('\x5');
  descArray[0].advanceX('\x22');
  descArray[0].advanceY('\x40');
  descArray[1].id(32u);
  descArray[1].x('\x12');
  descArray[1].y(4u);
  descArray[1].width('\x6');
  descArray[1].height('\x8');
  descArray[1].offsetX((int8_t)'\x5');
  descArray[1].offsetY('\x20');
  descArray[1].advanceX('\x4');
  descArray[1].advanceY('\x6');
  descArray[2].id(9999u);
  descArray[2].x('\x0');
  descArray[2].y(0);
  descArray[2].width('\x20');
  descArray[2].height('\x55');
  descArray[2].offsetX((int8_t)0);
  descArray[2].offsetY('\x0');
  descArray[2].advanceX('\x25');
  descArray[2].advanceY('\x60');

  FontMap<FakeTexture> initCtor2(FakeTexture{ 2 }, descArray, 3, 20);
  EXPECT_EQ((uint32_t)2, initCtor2.spriteSheet().handle);
  EXPECT_EQ((size_t)3, initCtor2.charCount());
  EXPECT_EQ((uint32_t)20, initCtor2.baseLineOffset());
  EXPECT_TRUE(initCtor2.find(0) == nullptr);
  ASSERT_FALSE(initCtor2.find(125) == nullptr);
  auto* descPtr = initCtor2.find(125);
  EXPECT_EQ((uint32_t)125u, descPtr->id());
  EXPECT_EQ((uint8_t)'\x10', descPtr->x());
  EXPECT_EQ((uint32_t)20u, descPtr->y());
  EXPECT_EQ((uint8_t)'\x25', descPtr->width());
  EXPECT_EQ((uint8_t)'\x35', descPtr->height());
  EXPECT_EQ((int8_t)-5, descPtr->offsetX());
  EXPECT_EQ((uint8_t)'\x5', descPtr->offsetY());
  EXPECT_EQ((uint8_t)'\x22', descPtr->advanceX());
  EXPECT_EQ((uint8_t)'\x40', descPtr->advanceY());
  ASSERT_FALSE(initCtor2.find(32) == nullptr);
  descPtr = initCtor2.find(32);
  EXPECT_EQ((uint32_t)32u, descPtr->id());
  EXPECT_EQ((uint8_t)'\x12', descPtr->x());
  EXPECT_EQ((uint32_t)4u, descPtr->y());
  EXPECT_EQ((uint8_t)'\x6', descPtr->width());
  EXPECT_EQ((uint8_t)'\x8', descPtr->height());
  EXPECT_EQ((int8_t)'\x5', descPtr->offsetX());
  EXPECT_EQ((uint8_t)'\x20', descPtr->offsetY());
  EXPECT_EQ((uint8_t)'\x4', descPtr->advanceX());
  EXPECT_EQ((uint8_t)'\x6', descPtr->advanceY());
  ASSERT_FALSE(initCtor2.find(9999) == nullptr);
  descPtr = initCtor2.find(9999);
  EXPECT_EQ((uint32_t)9999u, descPtr->id());
  EXPECT_EQ((uint8_t)'\x0', descPtr->x());
  EXPECT_EQ((uint32_t)0u, descPtr->y());
  EXPECT_EQ((uint8_t)'\x20', descPtr->width());
  EXPECT_EQ((uint8_t)'\x55', descPtr->height());
  EXPECT_EQ((int8_t)0, descPtr->offsetX());
  EXPECT_EQ((uint8_t)'\x0', descPtr->offsetY());
  EXPECT_EQ((uint8_t)'\x25', descPtr->advanceX());
  EXPECT_EQ((uint8_t)'\x60', descPtr->advanceY());
}
