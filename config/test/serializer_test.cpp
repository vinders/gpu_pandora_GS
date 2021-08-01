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
      pandora::io::removeDirectory(pluginDir + __UNICODE_STR(__ABS_PATH_SEP ".gpuPandoraGS"));
      pandora::io::removeFileEntry(pluginDir + __UNICODE_STR(__ABS_PATH_SEP) + Serializer::gameBindingDirectory() + __UNICODE_STR(__ABS_PATH_SEP "MY_GAME.032.bind"));
      pandora::io::removeFileEntry(pluginDir + __UNICODE_STR(__ABS_PATH_SEP) + Serializer::gameBindingDirectory() + __UNICODE_STR(__ABS_PATH_SEP "_Other-24.bind"));
      pandora::io::removeFileEntry(pluginDir + __UNICODE_STR(__ABS_PATH_SEP ".last.bind"));
      pandora::io::removeFileEntry(pluginDir + __UNICODE_STR(__ABS_PATH_SEP) + Serializer::mainConfigFileName());
      pandora::io::removeDirectory(pluginDir + __UNICODE_STR(__ABS_PATH_SEP) + Serializer::gameBindingDirectory());
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    else
      ASSERT_TRUE(pandora::io::createDirectory(pluginDir) == 0);
    pluginDir += __UNICODE_STR(__ABS_PATH_SEP);
  }
  static void TearDownTestCase() {
    pluginDir.pop_back();
    pandora::io::removeDirectory(pluginDir);
  }

  void SetUp() override {}
  void TearDown() override {}

  static pandora::io::SystemPath pluginDir;
};
pandora::io::SystemPath SerializerTest::pluginDir;


// -- directory/file utils --

TEST_F(SerializerTest, directoryCreateFind) {
  EXPECT_TRUE(Serializer::isPortableLocationAvailable());
  EXPECT_TRUE(Serializer::createConfigDir(true));
  auto configDir = pluginDir + __UNICODE_STR(".gpuPandoraGS" __ABS_PATH_SEP);
  EXPECT_TRUE(pandora::io::verifyFileSystemAccessMode(configDir.c_str(), pandora::io::FileSystemAccessMode::readWrite));

  auto foundDir = Serializer::findConfigDir();
  EXPECT_STREQ(configDir.c_str(), foundDir.c_str());

  pandora::io::removeDirectory(configDir);
}


// -- game/profile bindings --

TEST_F(SerializerTest, gameBindingSaveFind) {
  UnicodeString configDir(pluginDir.c_str());

  EXPECT_EQ((ProfileId)0, Serializer::findGameProfileBinding(configDir, "MY_GAME.032")); // default
  EXPECT_EQ((ProfileId)0, Serializer::findGameProfileBinding(configDir, "_Other-24")); // default

  EXPECT_TRUE(Serializer::saveGameProfileBinding(configDir, "MY_GAME.032", (ProfileId)32));
  UnicodeString gameFile1 = configDir + Serializer::gameBindingDirectory() + __UNICODE_STR(__ABS_PATH_SEP "MY_GAME.032.bind");
  UnicodeString lastUsedFile = configDir + Serializer::lastBindingFileName();
  EXPECT_TRUE(pandora::io::verifyFileSystemAccessMode(gameFile1.c_str(), pandora::io::FileSystemAccessMode::read));
  EXPECT_TRUE(pandora::io::verifyFileSystemAccessMode(lastUsedFile.c_str(), pandora::io::FileSystemAccessMode::read));
  EXPECT_EQ((ProfileId)32, Serializer::findGameProfileBinding(configDir, "MY_GAME.032"));
  EXPECT_EQ((ProfileId)32, Serializer::findGameProfileBinding(configDir, "_Other-24")); // uses .last.bind
  EXPECT_EQ((ProfileId)32, Serializer::findGameProfileBinding(configDir, "anything"));  // uses .last.bind

  EXPECT_TRUE(Serializer::saveGameProfileBinding(configDir, "_Other-24", (ProfileId)24));
  UnicodeString gameFile2 = configDir + Serializer::gameBindingDirectory() + __UNICODE_STR(__ABS_PATH_SEP "_Other-24.bind");
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
  pandora::io::removeDirectory(pluginDir + __UNICODE_STR(__ABS_PATH_SEP) + Serializer::gameBindingDirectory());
}


// -- serializer/deserializer --

static void __compareCommonConfig(const RendererConfig& r1, const RendererConfig& r2,
                                  const WindowConfig& w1, const WindowConfig& w2,
                                  const ActionsConfig& a1, const ActionsConfig& a2) {
  EXPECT_EQ(r1.api, r2.api);
  EXPECT_EQ(r1.enableFramerateLimit, r2.enableFramerateLimit);
  EXPECT_EQ(r1.enableVsync, r2.enableVsync);
  EXPECT_EQ(r1.framerateLimit, r2.framerateLimit);
  EXPECT_EQ(r1.frameSkip, r2.frameSkip);
  EXPECT_EQ(r1.precision, r2.precision);
  EXPECT_EQ(r1.osd, r2.osd);

  EXPECT_EQ(w1.monitorId, w2.monitorId);
  EXPECT_EQ(w1.windowMode, w2.windowMode);
  EXPECT_EQ(w1.windowHeight, w2.windowHeight);
  EXPECT_EQ(w1.fullscreen.width, w2.fullscreen.width);
  EXPECT_EQ(w1.fullscreen.height, w2.fullscreen.height);
  EXPECT_EQ(w1.fullscreen.refreshRate, w2.fullscreen.refreshRate);
  EXPECT_EQ(w1.isWideSource, w2.isWideSource);

  for (size_t i = 0; i < keyboardMap::length(); ++i) {
    EXPECT_EQ(a1.keyboardMapping[i], a2.keyboardMapping[i]);
  }
  for (size_t i = 0; i < controllerMap::length(); ++i) {
    EXPECT_EQ(a1.controllerMapping[i], a2.controllerMapping[i]);
  }
  EXPECT_EQ(a1.controllerHotkey, a2.controllerHotkey);
}
static void __compareProfileList(const std::vector<ProfileLabel>& p1, const std::vector<ProfileLabel>& p2_label,
                                 const std::unordered_map<ProfileId,ProfileMenuTile>& p2_menu) {
  ASSERT_EQ(p1.size(), p2_label.size());
  ASSERT_EQ(p1.size(), p2_menu.size());
  for (uint32_t i = 0; i < p1.size(); ++i) {
    auto& src = p1[i];
    auto& dest = p2_label[i];
    auto& menu = p2_menu.at(src.id);

    float color[3];
    toColorComponents(src.tileColor, color);
    CharCodeArray srcCharCodes(src.name.c_str(), src.name.size());

    EXPECT_EQ(src.id, dest.id);
    EXPECT_EQ(src.name, dest.name);
    EXPECT_EQ(srcCharCodes, menu.name);
    EXPECT_EQ(src.file, dest.file);
    EXPECT_EQ(src.file, menu.file);
    EXPECT_EQ(src.tileColor, dest.tileColor);
    for (uint32_t c = 0; c < 3; ++c)
      EXPECT_EQ(color[c], menu.tileColor[c]);
  }
}
static void __compareProfileConfig(const RendererProfile& r1, const RendererProfile& r2,
                                   const WindowProfile& w1, const WindowProfile& w2,
                                   const EffectsProfile& e1, const EffectsProfile& e2) {
  EXPECT_EQ(r1.internalResFactorX, r2.internalResFactorX);
  EXPECT_EQ(r1.internalResFactorY, r2.internalResFactorY);
  EXPECT_EQ(r1.colorMode, r2.colorMode);
  EXPECT_EQ(r1.fillMode, r2.fillMode);
  EXPECT_EQ(r1.antiAliasing, r2.antiAliasing);
  EXPECT_EQ(r1.textureUpscaling, r2.textureUpscaling);
  EXPECT_EQ(r1.textureUpscalingFactor, r2.textureUpscalingFactor);
  EXPECT_EQ(r1.useTextureBilinear, r2.useTextureBilinear);
  EXPECT_EQ(r1.spriteUpscaling, r2.spriteUpscaling);
  EXPECT_EQ(r1.spriteUpscalingFactor, r2.spriteUpscalingFactor);
  EXPECT_EQ(r1.useSpriteBilinear, r2.useSpriteBilinear);
  EXPECT_EQ(r1.screenUpscaling, r2.screenUpscaling);
  EXPECT_EQ(r1.screenUpscalingFactor, r2.screenUpscalingFactor);
  EXPECT_EQ(r1.mdecUpscaling, r2.mdecUpscaling);

  EXPECT_EQ(w1.screenStretching, w2.screenStretching);
  EXPECT_EQ(w1.screenCropping, w2.screenCropping);
  EXPECT_EQ(w1.screenCurvature, w2.screenCurvature);
  EXPECT_EQ(w1.isMirrored, w2.isMirrored);
  EXPECT_EQ(w1.isPalRecentered, w2.isPalRecentered);
  EXPECT_EQ(w1.isOverscanVisible, w2.isOverscanVisible);
  for (size_t i = 0; i < 4; ++i) {
    EXPECT_EQ(w1.blackBorderSizes[i], w2.blackBorderSizes[i]);
  }

  EXPECT_EQ(e1.screenGrain, e2.screenGrain);
  EXPECT_EQ(e1.textureGrain, e2.textureGrain);
  EXPECT_EQ(e1.dithering, e2.dithering);
  EXPECT_EQ(e1.useTextureDithering, e2.useTextureDithering);
  EXPECT_EQ(e1.useSpriteDithering, e2.useSpriteDithering);
}

// ---

TEST_F(SerializerTest, writeReadCommonConfig) {
  UnicodeString configDir(pluginDir.c_str());
  std::unique_ptr<char[]> buffer = nullptr;

  RendererConfig inRendererCfg, outRendererCfg;
  WindowConfig inWindowCfg, outWindowCfg;
  ActionsConfig inActionsCfg, outActionsCfg;

  inRendererCfg.enableFramerateLimit = false;
  inRendererCfg.enableVsync = false;
  memset(&inActionsCfg, 0, sizeof(ActionsConfig));
  Serializer::writeMainConfigFile(configDir, inRendererCfg, inWindowCfg, inActionsCfg);
  Serializer::readMainConfigFile(configDir, outRendererCfg, outWindowCfg, outActionsCfg);
  __compareCommonConfig(inRendererCfg, outRendererCfg, inWindowCfg, outWindowCfg, inActionsCfg, outActionsCfg);

  inRendererCfg.api = RenderingApi::openGL4;
  inRendererCfg.enableFramerateLimit = true;
  inRendererCfg.enableVsync = true;
  inRendererCfg.framerateLimit = 59.94f;
  inRendererCfg.frameSkip = FrameSkipping::adaptative;
  inRendererCfg.precision = PrecisionMode::subprecision;
  inRendererCfg.osd = OnScreenDisplay::framerate;
  inWindowCfg.monitorId = __UNICODE_STR("\\Display_1 - Generic PnP");
  inWindowCfg.windowMode = WindowMode::window;
  inWindowCfg.windowHeight = 800;
  inWindowCfg.fullscreen.width = 1280;
  inWindowCfg.fullscreen.height = 800;
  inWindowCfg.isWideSource = true;
  for (size_t i = 0; i < keyboardMap::length(); ++i)
    inActionsCfg.keyboardMapping[i] = i * 2;
  for (size_t i = 0; i < controllerMap::length(); ++i)
    inActionsCfg.controllerMapping[i] = i + 3;
  inActionsCfg.controllerHotkey = 42;
  Serializer::writeMainConfigFile(configDir, inRendererCfg, inWindowCfg, inActionsCfg);
  Serializer::readMainConfigFile(configDir, outRendererCfg, outWindowCfg, outActionsCfg);
  __compareCommonConfig(inRendererCfg, outRendererCfg, inWindowCfg, outWindowCfg, inActionsCfg, outActionsCfg);

  std::this_thread::sleep_for(std::chrono::milliseconds(1));
  auto filePath = configDir + Serializer::mainConfigFileName();
  pandora::io::removeFileEntry(filePath.c_str());
}

TEST_F(SerializerTest, writeReadProfileList) {
  UnicodeString configDir(pluginDir.c_str());
  std::unique_ptr<char[]> buffer = nullptr;

  std::vector<ProfileLabel> inList, outList;
  std::unordered_map<ProfileId, ProfileMenuTile> outMap;
  Serializer::writeProfileListFile(configDir, inList);
  Serializer::readProfileListFile(configDir, outList);
  Serializer::readProfileListFile(configDir, outMap);
  __compareProfileList(inList, outList, outMap);

  inList.push_back(ProfileLabel{ 42, UnicodeString(__UNICODE_STR("my_profile.cfg")), UnicodeString(__UNICODE_STR("My Profile")), MenuTileColor::violet });
  inList.push_back(ProfileLabel{ 1, UnicodeString(__UNICODE_STR("-the_accurate_prf-.cfg")), UnicodeString(__UNICODE_STR("-the_accurate_PRF-")), MenuTileColor::red });
  inList.push_back(ProfileLabel{ 0x7FFFFFFFu, UnicodeString(__UNICODE_STR("12345.cfg")), UnicodeString(__UNICODE_STR("12345")), MenuTileColor::teal });
  Serializer::writeProfileListFile(configDir, inList);
  Serializer::readProfileListFile(configDir, outList);
  Serializer::readProfileListFile(configDir, outMap);
  __compareProfileList(inList, outList, outMap);

  std::this_thread::sleep_for(std::chrono::milliseconds(1));
  auto filePath = configDir + Serializer::profileListFileName();
  pandora::io::removeFileEntry(filePath.c_str());
}

TEST_F(SerializerTest, writeReadProfileConfig) {
  UnicodeString configDir(pluginDir.c_str());
  auto filePath1 = configDir + __UNICODE_STR("profile_test_1");
  auto filePath2 = configDir + __UNICODE_STR("profile-02");
  std::unique_ptr<char[]> buffer = nullptr;

  RendererProfile inRendererCfg, outRendererCfg;
  WindowProfile inWindowCfg, outWindowCfg;
  EffectsProfile inEffectsCfg, outEffectsCfg;

  inRendererCfg.internalResFactorX = inRendererCfg.internalResFactorY = 1;
  inRendererCfg.useTextureBilinear = false;
  inWindowCfg.isPalRecentered = false;
  Serializer::writeProfileConfigFile(filePath1, inRendererCfg, inWindowCfg, inEffectsCfg);
  Serializer::readProfileConfigFile(filePath1, outRendererCfg, outWindowCfg, outEffectsCfg);
  __compareProfileConfig(inRendererCfg, outRendererCfg, inWindowCfg, outWindowCfg, inEffectsCfg, outEffectsCfg);

  inRendererCfg.internalResFactorX = 4;
  inRendererCfg.internalResFactorY = 2;
  inRendererCfg.colorMode = ColorOutput::rgb16;
  inRendererCfg.fillMode = FillMode::wireframeOverlay;
  inRendererCfg.antiAliasing = AntiAliasing::smaa4;
  inRendererCfg.textureUpscaling = UpscalingFilter::lanczos;
  inRendererCfg.useTextureBilinear = true;
  inRendererCfg.spriteUpscaling = UpscalingFilter::xBR;
  inRendererCfg.useSpriteBilinear = true;
  inRendererCfg.screenUpscaling = UpscalingFilter::SABR;
  inRendererCfg.mdecUpscaling = MdecFilter::bilinear;
  inWindowCfg.screenStretching = 4;
  inWindowCfg.screenCropping = 6;
  inWindowCfg.screenCurvature = 2;
  inWindowCfg.isMirrored = true;
  inWindowCfg.isOverscanVisible = true;
  inWindowCfg.isPalRecentered = true;
  for (uint32_t i = 0; i < 4; ++i)
    inWindowCfg.blackBorderSizes[i] = (uint8_t)i;
  inEffectsCfg.textureGrain = NoiseFilter::grain;
  inEffectsCfg.screenGrain = NoiseFilter::gauss;
  inEffectsCfg.dithering = ColorDithering::ditherOutput;
  inEffectsCfg.useTextureDithering = true;
  inEffectsCfg.useSpriteDithering = true;
  Serializer::writeProfileConfigFile(filePath2, inRendererCfg, inWindowCfg, inEffectsCfg);
  Serializer::readProfileConfigFile(filePath2, outRendererCfg, outWindowCfg, outEffectsCfg);
  __compareProfileConfig(inRendererCfg, outRendererCfg, inWindowCfg, outWindowCfg, inEffectsCfg, outEffectsCfg);

  std::this_thread::sleep_for(std::chrono::milliseconds(1));
  pandora::io::removeFileEntry(filePath1.c_str());
  pandora::io::removeFileEntry(filePath2.c_str());
}
