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
#include <config/config.h>

using namespace config;

class ConfigTest : public testing::Test {
public:
protected:
  //static void SetUpTestCase() {}
  //static void TearDownTestCase() {}

  void SetUp() override {}
  void TearDown() override {}
};


TEST_F(ConfigTest, actionsConfigDefaultMapping) {
  ActionsConfig cfg;
  cfg.initDefaultMapping();
  for (size_t i = 0; i < keyboardMap::length(); ++i) {
    EXPECT_NE(disabledKey(), cfg.keyboardMapping[i]);
  }
  if (cfg.controllerHotkey != disabledKey()) { // if controller support
    for (size_t i = 0; i < controllerMap::length(); ++i) {
      EXPECT_NE(disabledKey(), cfg.controllerMapping[i]);
    }
  }
}

TEST_F(ConfigTest, profileMenuTileDefault) {
  ProfileMenuTile empty;
  EXPECT_TRUE(empty.file.empty());
  EXPECT_TRUE(empty.name.empty());
  EXPECT_EQ(1.f, empty.tileColor[0]);
  EXPECT_EQ(1.f, empty.tileColor[1]);
  EXPECT_EQ(1.f, empty.tileColor[2]);

  ProfileMenuTile val1;
  val1.file = __UNICODE_STR("abc");
  val1.name = CharCodeArray("def", 3);
  val1.tileColor[0] = 0.f;
  val1.tileColor[2] = 2.f;

  ProfileMenuTile copied(val1);
  EXPECT_FALSE(copied.file.empty());
  EXPECT_STREQ(__UNICODE_STR("abc"), copied.file.c_str());
  EXPECT_FALSE(copied.name.empty());
  ASSERT_EQ(size_t{ 3u }, copied.name.size());
  EXPECT_EQ((uint32_t)'d', copied.name.value()[0]);
  EXPECT_EQ((uint32_t)'e', copied.name.value()[1]);
  EXPECT_EQ((uint32_t)'f', copied.name.value()[2]);
  EXPECT_EQ(0.f, copied.tileColor[0]);
  EXPECT_EQ(1.f, copied.tileColor[1]);
  EXPECT_EQ(2.f, copied.tileColor[2]);

  empty = copied;
  EXPECT_FALSE(empty.file.empty());
  EXPECT_STREQ(__UNICODE_STR("abc"), empty.file.c_str());
  EXPECT_FALSE(empty.name.empty());
  ASSERT_EQ(size_t{ 3u }, empty.name.size());
  EXPECT_EQ((uint32_t)'d', empty.name.value()[0]);
  EXPECT_EQ((uint32_t)'e', empty.name.value()[1]);
  EXPECT_EQ((uint32_t)'f', empty.name.value()[2]);
  EXPECT_EQ(0.f, empty.tileColor[0]);
  EXPECT_EQ(1.f, empty.tileColor[1]);
  EXPECT_EQ(2.f, empty.tileColor[2]);

  ProfileMenuTile moved(std::move(val1));
  EXPECT_FALSE(moved.file.empty());
  EXPECT_STREQ(__UNICODE_STR("abc"), moved.file.c_str());
  EXPECT_FALSE(moved.name.empty());
  ASSERT_EQ(size_t{ 3u }, moved.name.size());
  EXPECT_EQ((uint32_t)'d', moved.name.value()[0]);
  EXPECT_EQ((uint32_t)'e', moved.name.value()[1]);
  EXPECT_EQ((uint32_t)'f', moved.name.value()[2]);
  EXPECT_EQ(0.f, moved.tileColor[0]);
  EXPECT_EQ(1.f, moved.tileColor[1]);
  EXPECT_EQ(2.f, moved.tileColor[2]);

  val1 = std::move(moved);
  EXPECT_FALSE(val1.file.empty());
  EXPECT_STREQ(__UNICODE_STR("abc"), val1.file.c_str());
  EXPECT_FALSE(val1.name.empty());
  ASSERT_EQ(size_t{ 3u }, val1.name.size());
  EXPECT_EQ((uint32_t)'d', val1.name.value()[0]);
  EXPECT_EQ((uint32_t)'e', val1.name.value()[1]);
  EXPECT_EQ((uint32_t)'f', val1.name.value()[2]);
  EXPECT_EQ(0.f, val1.tileColor[0]);
  EXPECT_EQ(1.f, val1.tileColor[1]);
  EXPECT_EQ(2.f, val1.tileColor[2]);
}

TEST_F(ConfigTest, profileLabelToMenuTile) {
  ProfileLabel labelEmpty;
  ProfileMenuTile empty(labelEmpty);
  EXPECT_TRUE(empty.file.empty());
  EXPECT_TRUE(empty.name.empty());
  EXPECT_TRUE(empty.tileColor[0] >= 0.99999f && empty.tileColor[0] <= 1.00001f);
  EXPECT_TRUE(empty.tileColor[1] >= 0.99999f && empty.tileColor[1] <= 1.00001f);
  EXPECT_TRUE(empty.tileColor[2] >= 0.99999f && empty.tileColor[2] <= 1.00001f);
  ProfileMenuTile emptyMoved(std::move(labelEmpty));
  EXPECT_TRUE(emptyMoved.file.empty());
  EXPECT_TRUE(emptyMoved.name.empty());
  EXPECT_TRUE(emptyMoved.tileColor[0] >= 0.99999f && emptyMoved.tileColor[0] <= 1.00001f);
  EXPECT_TRUE(emptyMoved.tileColor[1] >= 0.99999f && emptyMoved.tileColor[1] <= 1.00001f);
  EXPECT_TRUE(emptyMoved.tileColor[2] >= 0.99999f && emptyMoved.tileColor[2] <= 1.00001f);

  ProfileLabel label1 = { 1, __UNICODE_STR("abc"), __UNICODE_STR("def"), MenuTileColor::red };
  ProfileMenuTile tile1(label1);
  EXPECT_FALSE(tile1.file.empty());
  EXPECT_STREQ(__UNICODE_STR("abc"), tile1.file.c_str());
  EXPECT_FALSE(tile1.name.empty());
  ASSERT_EQ(size_t{ 3u }, tile1.name.size());
  EXPECT_EQ((uint32_t)'d', tile1.name.value()[0]);
  EXPECT_EQ((uint32_t)'e', tile1.name.value()[1]);
  EXPECT_EQ((uint32_t)'f', tile1.name.value()[2]);
  EXPECT_TRUE(tile1.tileColor[0] >= 0.9803f && tile1.tileColor[0] <= 0.9804f);
  EXPECT_TRUE(tile1.tileColor[1] >= 0.19999f && tile1.tileColor[1] <= 0.20001f);
  EXPECT_TRUE(tile1.tileColor[2] >= 0.19999f && tile1.tileColor[2] <= 0.20001f);
  ProfileMenuTile tile1Moved(std::move(label1));
  EXPECT_FALSE(tile1Moved.file.empty());
  EXPECT_STREQ(__UNICODE_STR("abc"), tile1Moved.file.c_str());
  EXPECT_FALSE(tile1Moved.name.empty());
  ASSERT_EQ(size_t{ 3u }, tile1Moved.name.size());
  EXPECT_EQ((uint32_t)'d', tile1Moved.name.value()[0]);
  EXPECT_EQ((uint32_t)'e', tile1Moved.name.value()[1]);
  EXPECT_EQ((uint32_t)'f', tile1Moved.name.value()[2]);
  EXPECT_TRUE(tile1Moved.tileColor[0] >= 0.9803f && tile1Moved.tileColor[0] <= 0.9804f);
  EXPECT_TRUE(tile1Moved.tileColor[1] >= 0.19999f && tile1Moved.tileColor[1] <= 0.20001f);
  EXPECT_TRUE(tile1Moved.tileColor[2] >= 0.19999f && tile1Moved.tileColor[2] <= 0.20001f);

  ProfileLabel label2 = { 2, __UNICODE_STR("\x00E9#\x0080*\x00E0"), __UNICODE_STR("_\x00B0(\x00AB\x00F9\x00BB)}"), MenuTileColor::green };
  ProfileMenuTile tile2(label2);
  EXPECT_FALSE(tile2.file.empty());
  EXPECT_STREQ(__UNICODE_STR("\x00E9#\x0080*\x00E0"), tile2.file.c_str());
  EXPECT_FALSE(tile2.name.empty());
  ASSERT_EQ(size_t{ 8u }, tile2.name.size());
  EXPECT_EQ((uint32_t)'_', tile2.name.value()[0]);
  EXPECT_EQ((uint32_t)0xB0, tile2.name.value()[1]);
  EXPECT_EQ((uint32_t)'(', tile2.name.value()[2]);
  EXPECT_EQ((uint32_t)0xAB, tile2.name.value()[3]);
  EXPECT_EQ((uint32_t)0xF9, tile2.name.value()[4]);
  EXPECT_EQ((uint32_t)0xBB, tile2.name.value()[5]);
  EXPECT_EQ((uint32_t)')', tile2.name.value()[6]);
  EXPECT_EQ((uint32_t)'}', tile2.name.value()[7]);
  EXPECT_TRUE(tile2.tileColor[0] >= 0.19999f && tile2.tileColor[0] <= 0.20001f);
  EXPECT_TRUE(tile2.tileColor[1] >= 0.7333f && tile2.tileColor[1] <= 0.7334f);
  EXPECT_TRUE(tile2.tileColor[2] >= 0.19999f && tile2.tileColor[2] <= 0.20001f);
  ProfileMenuTile tile2Moved(std::move(label2));
  EXPECT_FALSE(tile2Moved.file.empty());
  EXPECT_STREQ(__UNICODE_STR("\x00E9#\x0080*\x00E0"), tile2Moved.file.c_str());
  EXPECT_FALSE(tile2Moved.name.empty());
  ASSERT_EQ(size_t{ 8u }, tile2Moved.name.size());
  EXPECT_EQ((uint32_t)'_', tile2Moved.name.value()[0]);
  EXPECT_EQ((uint32_t)0xB0, tile2Moved.name.value()[1]);
  EXPECT_EQ((uint32_t)'(', tile2Moved.name.value()[2]);
  EXPECT_EQ((uint32_t)0xAB, tile2Moved.name.value()[3]);
  EXPECT_EQ((uint32_t)0xF9, tile2Moved.name.value()[4]);
  EXPECT_EQ((uint32_t)0xBB, tile2Moved.name.value()[5]);
  EXPECT_EQ((uint32_t)')', tile2Moved.name.value()[6]);
  EXPECT_EQ((uint32_t)'}', tile2Moved.name.value()[7]);
  EXPECT_TRUE(tile2Moved.tileColor[0] >= 0.19999f && tile2Moved.tileColor[0] <= 0.20001f);
  EXPECT_TRUE(tile2Moved.tileColor[1] >= 0.7333f && tile2Moved.tileColor[1] <= 0.7334f);
  EXPECT_TRUE(tile2Moved.tileColor[2] >= 0.19999f && tile2Moved.tileColor[2] <= 0.20001f);
}
