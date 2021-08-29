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
#include <iostream>
#include <fstream>
#include <thread>
#include <utils/syslog.h>

using namespace utils;

class SysLogTest : public testing::Test {
public:
protected:
  //static void SetUpTestCase() {}
  //static void TearDownTestCase() {}

  void SetUp() override {}
  void TearDown() override {}
};


TEST_F(SysLogTest, testLogger) {
# ifdef _WINDOWS
    SysLog::init(L"./", "--- title ---", SysLog::Level::debug);
# else
    SysLog::init("./", "--- title ---", SysLog::Level::debug);
# endif
  SysLog::logDebug("abc", 1, "test example: %d", 42);
  SysLog::logInfo("//def//", 2, "other example");
  SysLog::logInfo("//def//", 3, "last example: %s", "42");
  SysLog::logWarning("--warned--", 0, "no params here!!!");
  SysLog::logError("--error!!--", 0xFF, "this is not a real error...");
  SysLog::close();

# ifdef _WINDOWS
  SysLog::init(L"./", "--- title2 ---", SysLog::Level::error);
# else
  SysLog::init("./", "--- title2 ---", SysLog::Level::error);
# endif
  SysLog::logDebug("abc", 1, "test example: %d", 42);
  SysLog::logInfo("//def//", 2, "other example");
  SysLog::logInfo("//def//", 3, "last example: %s", "42");
  SysLog::logWarning("--warned--", 0, "no params here!!!");
  SysLog::logError("--error!!--", 0xFF, "this is not a real error...");
  SysLog::close();

  std::ifstream reader("./gpuPandoraGS.log", std::ios::in);
  EXPECT_TRUE(reader.is_open());

  if ((reader.is_open())) {
    char buffer[128];
    EXPECT_TRUE(!reader.eof() && reader.getline(buffer, 128));
    EXPECT_STREQ("--- title ---", buffer);
#   if defined(_DEBUG) || !defined(NDEBUG)
      EXPECT_TRUE(!reader.eof() && reader.getline(buffer, 128));
      EXPECT_STREQ("(lv.2) [abc:1]: test example: 42", &buffer[24]);
#   endif
    EXPECT_TRUE(!reader.eof() && reader.getline(buffer, 128));
    EXPECT_STREQ("INFO(lv.3) [//def//:2]: other example", &buffer[24]);
    EXPECT_TRUE(!reader.eof() && reader.getline(buffer, 128));
    EXPECT_STREQ("INFO(lv.3) [//def//:3]: last example: 42", &buffer[24]);
    EXPECT_TRUE(!reader.eof() && reader.getline(buffer, 128));
    EXPECT_STREQ("WARNING(lv.4) [--warned--:0]: no params here!!!", &buffer[24]);
    EXPECT_TRUE(!reader.eof() && reader.getline(buffer, 128));
    EXPECT_STREQ("ERROR(lv.5) [--error!!--:255]: this is not a real error...", &buffer[24]);
    EXPECT_TRUE(!reader.eof() && reader.getline(buffer, 128));
    EXPECT_STREQ("--- title2 ---", buffer);
    EXPECT_TRUE(!reader.eof() && reader.getline(buffer, 128));
    EXPECT_STREQ("ERROR(lv.5) [--error!!--:255]: this is not a real error...", &buffer[24]);

    EXPECT_TRUE(reader.eof() || !reader.getline(buffer, 128) || *buffer == '\0');
  }
  reader.close();
  std::this_thread::sleep_for(std::chrono::milliseconds(1));
  remove("./gpuPandoraGS.log");
}
