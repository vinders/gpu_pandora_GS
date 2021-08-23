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
#include <thread>
#include <io/file_system_io.h>
#include <config/file_path_utils.h>

using namespace config;

class FilePathUtilsTest : public testing::Test {
public:
protected:
  //static void SetUpTestCase() {}
  //static void TearDownTestCase() {}

  void SetUp() override {}
  void TearDown() override {}
};

#ifdef _WINDOWS
# define PRINT std::wcout
  static const __UNICODE_CHAR* g_appName = __UNICODE_STR("gpu_pandora_GS.config.test.exe");
  static size_t g_nameLength = 30;
#else
# define PRINT std::cout
  static const __UNICODE_CHAR* g_appName = __UNICODE_STR("gpu_pandora_GS.config.test");
  static size_t g_nameLength = 26;
#endif


// -- file / path utilities --

TEST_F(FilePathUtilsTest, fileCreateRemove) {
  auto writer = config::openFile(__UNICODE_STR("ABC"), __UNICODE_STR("wt"));
  EXPECT_TRUE(writer.isOpen());
  writer.close();
  EXPECT_TRUE(config::isPathReadable(__UNICODE_STR("ABC")));
  EXPECT_TRUE(config::isPathWritable(__UNICODE_STR("ABC")));

  std::this_thread::sleep_for(std::chrono::milliseconds(1));
  EXPECT_TRUE(config::removeFile(__UNICODE_STR("ABC")));
}

TEST_F(FilePathUtilsTest, directoryCreate) {
  EXPECT_TRUE(config::createDirectory(__UNICODE_STR("DEF")));
  EXPECT_TRUE(config::isPathReadable(__UNICODE_STR("DEF/")));
  EXPECT_TRUE(config::isPathWritable(__UNICODE_STR("DEF/")));

  std::this_thread::sleep_for(std::chrono::milliseconds(1));
  pandora::io::removeDirectory(__UNICODE_STR("DEF"));
}

TEST_F(FilePathUtilsTest, currentProcessPaths) {
  auto processPath = config::getProcessPath();
  EXPECT_FALSE(processPath.empty());
  auto curDirPath = config::getCurrentDir();
  EXPECT_FALSE(curDirPath.empty());
  PRINT << __UNICODE_STR("Current: ") << curDirPath.c_str() << __UNICODE_STR("\nProcess: ") << processPath.c_str() << std::endl;

  ASSERT_TRUE(curDirPath.size() < processPath.size());
  ASSERT_TRUE(g_nameLength <= processPath.size());
  EXPECT_EQ(0, memcmp(curDirPath.c_str(), processPath.c_str(), curDirPath.size() * sizeof(__UNICODE_CHAR)));
  EXPECT_EQ(0, memcmp(&(processPath.c_str()[processPath.size() - g_nameLength]), g_appName, g_nameLength*sizeof(__UNICODE_CHAR)));
}

// -- emulator detection --

TEST_F(FilePathUtilsTest, emulatorDetection) {
  EmulatorInfo info;
  config::readEmulatorInfo(info);
  EXPECT_EQ(config::EmulatorType::unknown, info.type);
  EXPECT_FALSE(info.pluginDir.empty());
  PRINT << __UNICODE_STR("Plugins: ") << info.pluginDir.c_str() << std::endl;

  auto processPath = config::getProcessPath();
  ASSERT_TRUE(processPath.size() >= g_nameLength);
  size_t dirLength = processPath.size() - g_nameLength;
  __UNICODE_CHAR processDir[1024];
  memcpy(processDir, processPath.c_str(), dirLength*sizeof(__UNICODE_CHAR));
  processDir[dirLength] = (__UNICODE_CHAR)0;

  ASSERT_TRUE(info.pluginDir.size() >= dirLength);
  EXPECT_EQ(0, memcmp(processDir, info.pluginDir.c_str(), dirLength*sizeof(__UNICODE_CHAR)));

  config::readEmulatorOptions(info);
  EXPECT_FALSE(info.isCursorHidden);
  EXPECT_FALSE(info.widescreenHack);
}

// -- config file paths --

TEST_F(FilePathUtilsTest, createFindConfigDir) {
  auto pluginDir = config::getCurrentDir() + __UNICODE_STR(__ABS_PATH_SEP "plugins" __ABS_PATH_SEP);
  ASSERT_TRUE(config::createDirectory(pluginDir.c_str()));
  EXPECT_TRUE(config::isPathWritable(pluginDir.c_str()));

  auto configDir = config::findConfigDir(pluginDir);
  EXPECT_TRUE(configDir.empty());

  auto portableDir = config::getPortableConfigDir(pluginDir);
  EXPECT_FALSE(portableDir.empty());
  auto userDir = config::getLocalUserConfigDir();
  EXPECT_FALSE(userDir.empty());
  PRINT << __UNICODE_STR("Portable: ") << portableDir.c_str() << __UNICODE_STR("\nLocal: ") << userDir.c_str() << std::endl;

  auto parentDir = config::getLocalUserParentDir();
  ASSERT_TRUE(!parentDir.empty() && parentDir.size() < userDir.size());
  EXPECT_EQ(0, memcmp(parentDir.c_str(), userDir.c_str(), parentDir.size()*sizeof(__UNICODE_CHAR)));
  EXPECT_EQ(toLocalUserConfigDir(parentDir), userDir);

  ASSERT_TRUE(config::createDirectory(portableDir.c_str()));
  EXPECT_TRUE(config::isPathWritable(portableDir.c_str()));

  configDir = config::findConfigDir(pluginDir);
  EXPECT_FALSE(configDir.empty());
  EXPECT_EQ(portableDir, configDir);

  EXPECT_EQ(config::getGlobalConfigPath(configDir), configDir + config::globalConfigFileName());
  EXPECT_EQ(config::getProfileListPath(configDir), configDir + config::profileListFileName());

  pandora::io::removeDirectory(portableDir.c_str());
  pandora::io::removeDirectory(pluginDir.c_str());
}

// -- game/profile binding paths --

TEST_F(FilePathUtilsTest, gameBindingsPaths) {
  UnicodeString configDir(__UNICODE_STR("ABC"));
  EXPECT_EQ(config::getGameBindingsDir(configDir), configDir + config::gameBindingsDirectory());

  EXPECT_EQ(configDir + config::gameBindingsDirectory() + __UNICODE_STR(__ABS_PATH_SEP "DEF.bind"),
            getGameBindingPath(configDir, "DEF"));
  EXPECT_EQ(configDir + config::gameBindingsDirectory() + __UNICODE_STR(__ABS_PATH_SEP "a__speci@l__path_....bind"),
            getGameBindingPath(configDir, "a| speci@l? path*..."));
  EXPECT_EQ(configDir + config::gameBindingsDirectory() + __UNICODE_STR(__ABS_PATH_SEP "_name-with_seps.1_23_45_6_.bind"),
            getGameBindingPath(configDir, ":name-with_seps.1\\23<45/6>"));
}
