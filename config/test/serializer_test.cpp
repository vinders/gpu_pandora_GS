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
#include <thread>
#include <io/file_system_io.h>
#include <io/file_system_locations.h>
#include <config/serializer.h>

using namespace config;

class SerializerTest : public testing::Test {
public:
protected:
  static void SetUpTestCase() {
    pluginDir = pandora::io::FileSystemLocationFinder::currentLocation() + __UNICODE_STR(__ABS_PATH_SEP "plugins");
    if (pandora::io::verifyFileSystemAccessMode(pluginDir.c_str(), pandora::io::FileSystemAccessMode::readWrite)) {
      pandora::io::removeDirectory(pluginDir + __UNICODE_STR(".gpuPandoraGS"));
      pandora::io::removeFileEntry(pluginDir + __UNICODE_STR(".MY_GAME.032.bind"));
      pandora::io::removeFileEntry(pluginDir + __UNICODE_STR("._Other-24.bind"));
      pandora::io::removeFileEntry(pluginDir + __UNICODE_STR(".last.bind"));
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    else
      ASSERT_TRUE(pandora::io::createDirectory(pluginDir) == 0);
  }
  static void TearDownTestCase() {
    pandora::io::removeDirectory(pluginDir);
  }

  void SetUp() override {}
  void TearDown() override {}

  static pandora::io::SystemPath pluginDir;
};
pandora::io::SystemPath SerializerTest::pluginDir;


TEST_F(SerializerTest, directoryCreateFind) {
  EXPECT_TRUE(Serializer::isPortableLocationAvailable());
  EXPECT_TRUE(Serializer::createConfigDir(true));
  auto configDir = pluginDir + __UNICODE_STR(__ABS_PATH_SEP ".gpuPandoraGS" __ABS_PATH_SEP);
  EXPECT_TRUE(pandora::io::verifyFileSystemAccessMode(configDir.c_str(), pandora::io::FileSystemAccessMode::readWrite));

  auto foundDir = Serializer::findConfigDir();
  EXPECT_STREQ(configDir.c_str(), foundDir.c_str());

  pandora::io::removeDirectory(configDir);
}

TEST_F(SerializerTest, gameBindingSaveFind) {
  UnicodeString configDir(pluginDir.c_str());
  configDir += __UNICODE_STR(__ABS_PATH_SEP);

  EXPECT_EQ((ProfileId)0, Serializer::findGameProfileBinding(configDir, "MY_GAME.032"));
  EXPECT_EQ((ProfileId)0, Serializer::findGameProfileBinding(configDir, "_Other-24"));

  EXPECT_TRUE(Serializer::saveGameProfileBinding(configDir, "MY_GAME.032", (ProfileId)32));
  UnicodeString gameFile1 = configDir + __UNICODE_STR(".MY_GAME.032.bind");
  UnicodeString lastUsedFile = configDir + __UNICODE_STR(".last.bind");
  EXPECT_TRUE(pandora::io::verifyFileSystemAccessMode(gameFile1.c_str(), pandora::io::FileSystemAccessMode::read));
  EXPECT_TRUE(pandora::io::verifyFileSystemAccessMode(lastUsedFile.c_str(), pandora::io::FileSystemAccessMode::read));
  EXPECT_EQ((ProfileId)32, Serializer::findGameProfileBinding(configDir, "MY_GAME.032"));
  EXPECT_EQ((ProfileId)32, Serializer::findGameProfileBinding(configDir, "_Other-24")); // uses .last.bind
  EXPECT_EQ((ProfileId)32, Serializer::findGameProfileBinding(configDir, "anything"));  // uses .last.bind

  EXPECT_TRUE(Serializer::saveGameProfileBinding(configDir, "_Other-24", (ProfileId)24));
  UnicodeString gameFile2 = configDir + __UNICODE_STR("._Other-24.bind");
  EXPECT_TRUE(pandora::io::verifyFileSystemAccessMode(gameFile1.c_str(), pandora::io::FileSystemAccessMode::read));
  EXPECT_TRUE(pandora::io::verifyFileSystemAccessMode(gameFile2.c_str(), pandora::io::FileSystemAccessMode::read));
  EXPECT_TRUE(pandora::io::verifyFileSystemAccessMode(lastUsedFile.c_str(), pandora::io::FileSystemAccessMode::read));
  EXPECT_EQ((ProfileId)32, Serializer::findGameProfileBinding(configDir, "MY_GAME.032"));
  EXPECT_EQ((ProfileId)24, Serializer::findGameProfileBinding(configDir, "_Other-24"));

  EXPECT_TRUE(Serializer::saveGameProfileBinding(configDir, "MY_GAME.032", (ProfileId)64));
  EXPECT_TRUE(pandora::io::verifyFileSystemAccessMode(gameFile1.c_str(), pandora::io::FileSystemAccessMode::read));
  EXPECT_TRUE(pandora::io::verifyFileSystemAccessMode(gameFile2.c_str(), pandora::io::FileSystemAccessMode::read));
  EXPECT_TRUE(pandora::io::verifyFileSystemAccessMode(lastUsedFile.c_str(), pandora::io::FileSystemAccessMode::read));
  EXPECT_EQ((ProfileId)64, Serializer::findGameProfileBinding(configDir, "MY_GAME.032"));
  EXPECT_EQ((ProfileId)24, Serializer::findGameProfileBinding(configDir, "_Other-24"));

  std::this_thread::sleep_for(std::chrono::milliseconds(1));
  pandora::io::removeFileEntry(gameFile1.c_str());
  pandora::io::removeFileEntry(gameFile2.c_str());
  pandora::io::removeFileEntry(lastUsedFile.c_str());
}
