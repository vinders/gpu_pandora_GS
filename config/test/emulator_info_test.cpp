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
#include <config/emulator_info.h>

using namespace config;

class EmulatorInfoTest : public testing::Test {
public:
protected:
  //static void SetUpTestCase() {}
  //static void TearDownTestCase() {}

  void SetUp() override {}
  void TearDown() override {}
};


TEST_F(EmulatorInfoTest, emulatorDetection) {
  size_t length = 0;
  __PATH_CHAR buffer[1024];
  __PATH_CHAR* fileName = config::getProcessName(buffer, 1024, length);
  ASSERT_TRUE(fileName != nullptr);
  EXPECT_TRUE(fileName >= buffer);
  
  const __PATH_CHAR* appNamePrefix = __PATH_STR("gpu_pandora_GS.config.test");
  size_t prefixLength = 26;
  EXPECT_EQ(0, memcmp(fileName, appNamePrefix, prefixLength*sizeof(__PATH_CHAR)));
  EXPECT_TRUE(length >= prefixLength);
  
  EXPECT_EQ(config::EmulatorType::unknown, config::getEmulatorType());
}
