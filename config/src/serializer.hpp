/*******************************************************************************
Pandora GS - PSEmu-compatible GPU driver
Copyright (C) 2021  Romain Vinders

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation, version 2
of the License.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details (LICENSE file).
*******************************************************************************/
// this implementation is included in config.cpp -> single object file (less overhead + smaller lib size)
#include <cstdlib>
#include <thread>
#include <stdexcept>
#include <io/encoder.h>
#include <io/json_serializer.h>
#include <io/file_system_io.h>
#include <io/file_system_locations.h>
#include "config/_private/_serializer_keys.h"
#include "config/serializer.h"

using namespace config;
using pandora::hardware::DisplayMonitor;
using pandora::io::SerializableValue;


// -----------------------------------------------------------------------------
// DIRECTORY & PROFILE BINDING
// -----------------------------------------------------------------------------

// -- directory/file utils -- --------------------------------------------------

// Create config directory
bool Serializer::createConfigDir(bool usePortableLocation) {
  if (usePortableLocation) {
    return (pandora::io::createDirectory(pandora::io::FileSystemLocationFinder::currentLocation()
                                         + __UNICODE_STR(__ABS_PATH_SEP "plugins" __ABS_PATH_SEP ".gpuPandoraGS")) == 0);
  }
  else {
    auto appDataDirs = pandora::io::FileSystemLocationFinder::standardLocation(pandora::io::FileSystemLocation::appData,
                                                                               __UNICODE_STR("Games"));
    auto& targetDir = appDataDirs.front();
    if (!pandora::io::verifyFileSystemAccessMode(targetDir.c_str(), pandora::io::FileSystemAccessMode::read))
      pandora::io::createDirectory(targetDir);

    targetDir += __UNICODE_STR(__ABS_PATH_SEP "gpuPandoraGS");
    return (pandora::io::createDirectory(targetDir) == 0);
  }
}
// Find config directory
UnicodeString Serializer::findConfigDir() {
  auto currentLocation = pandora::io::FileSystemLocationFinder::currentLocation();

  auto configDir = currentLocation + __UNICODE_STR(__ABS_PATH_SEP "plugins" __ABS_PATH_SEP ".gpuPandoraGS" __ABS_PATH_SEP);
  if (!pandora::io::verifyFileSystemAccessMode(configDir.c_str(), pandora::io::FileSystemAccessMode::readWrite)) {
    auto appDataDirs = pandora::io::FileSystemLocationFinder::standardLocation(pandora::io::FileSystemLocation::appData,
                                                                __UNICODE_STR("Games" __ABS_PATH_SEP "gpuPandoraGS" __ABS_PATH_SEP));
    configDir = appDataDirs.front();
    if (!pandora::io::verifyFileSystemAccessMode(configDir.c_str(), pandora::io::FileSystemAccessMode::readWrite))
      return UnicodeString{};
  }
  return UnicodeString(configDir.c_str());
}

/// @brief Verify if portable directory ('plugins') has write access
bool Serializer::isPortableLocationAvailable() {
  auto portableDir = pandora::io::FileSystemLocationFinder::currentLocation() + __UNICODE_STR(__ABS_PATH_SEP "plugins" __ABS_PATH_SEP);
  return pandora::io::verifyFileSystemAccessMode(portableDir.c_str(), pandora::io::FileSystemAccessMode::readWrite);
}


// -- game/profile bindings -- -------------------------------------------------

#define __GAME_ID_BUFFER_SIZE 32

static UnicodeString __getGameProfileBindingPath(const UnicodeString& configDir, const char* gameId) {
  __UNICODE_CHAR buffer[__GAME_ID_BUFFER_SIZE];
  __UNICODE_CHAR* lastIndex = &buffer[__GAME_ID_BUFFER_SIZE - 1];
  __UNICODE_CHAR* cur;
  for (cur = &buffer[0]; *gameId && cur < lastIndex; ++cur, ++gameId) {
    *cur = ((*gameId >= '@' && *gameId != '\\' && *gameId != '|')
         || (*gameId <= '9' && *gameId >= '#' && *gameId != '*' && *gameId != '/'))
         ? static_cast<__UNICODE_CHAR>(*gameId)
         : L'_';
  }
  *cur = (__UNICODE_CHAR)0;

  return configDir + Serializer::gameBindingDirectory() + __UNICODE_STR(__ABS_PATH_SEP) + buffer + __UNICODE_STR(".bind");
}

// ---

// Find profile associated with current game (saved at the end of last execution)
ProfileId Serializer::findGameProfileBinding(const UnicodeString& configDir, const char* gameId) noexcept {
  ProfileId profileId;
  if (gameId != nullptr) {
    auto filePath = __getGameProfileBindingPath(configDir, gameId);
    auto input = pandora::io::openFileEntry(filePath.c_str(), __UNICODE_STR("rb"));
    if (input.isOpen() && fread(&profileId, sizeof(ProfileId), 1, input.handle()) > 0)
      return profileId;
  }

  auto filePath = configDir + lastBindingFileName();
  auto input = pandora::io::openFileEntry(filePath.c_str(), __UNICODE_STR("rb"));
  if (input.isOpen() && fread(&profileId, sizeof(ProfileId), 1, input.handle()) > 0)
    return profileId;
  return 0;
}

// Associate profile with current game (for the next time) + save profile as "last used profile"
bool Serializer::saveGameProfileBinding(const UnicodeString& configDir, const char* gameId, ProfileId profileId) noexcept {
  if (gameId != nullptr) {
    // create directory (if not existing)
    auto dirPath = pandora::io::SystemPath(configDir.c_str()) + gameBindingDirectory();
    if (!pandora::io::verifyFileSystemAccessMode(dirPath.c_str(), pandora::io::FileSystemAccessMode::readWrite))
      pandora::io::createDirectory(dirPath);

    // save game/profile association
    auto filePath = __getGameProfileBindingPath(configDir, gameId);
    auto output = pandora::io::openFileEntry(filePath.c_str(), __UNICODE_STR("wb"));
    if (output.isOpen())
      fwrite(&profileId, sizeof(ProfileId), 1, output.handle());
  }

  // save "last used profile"
  bool isSuccess = false;
  auto filePath = configDir + lastBindingFileName();
  auto output = pandora::io::openFileEntry(filePath.c_str(), __UNICODE_STR("wb"));
  if (output.isOpen())
    isSuccess = (fwrite(&profileId, sizeof(ProfileId), 1, output.handle()) > 0);
  return isSuccess;
}


// -----------------------------------------------------------------------------
// JSON SERIALIZATION
// -----------------------------------------------------------------------------

#define __MAX_RETRIES  5

// -- serialization/deserialization helpers -- ---------------------------------

// Write JSON to config file
static void __writeJsonFile(const UnicodeString& outputFile, const SerializableValue::Object& data) {
  pandora::io::JsonSerializer serializer(0);
  auto serialized = serializer.toString(data);
  size_t length = serialized.size();

  bool prevFileExists = pandora::io::verifyFileSystemAccessMode(outputFile.c_str(), pandora::io::FileSystemAccessMode::read);
  UnicodeString tmpFile = prevFileExists ? outputFile + __UNICODE_STR("_tmp") : outputFile;
  auto output = pandora::io::openFileEntry(tmpFile.c_str(), __UNICODE_STR("wt"));
  if (!output.isOpen())
    throw std::runtime_error("Config file could not be created");

  size_t bytesWritten = fwrite(serialized.c_str(), 1, length, output.handle());
  output.close();

  if (bytesWritten < length) {
    pandora::io::removeFileEntry(tmpFile.c_str());
    throw std::runtime_error("Failed to write config file data");
  }
  if (prevFileExists) {
    if (pandora::io::removeFileEntry(outputFile.c_str()) != 0)
      throw std::runtime_error("Failed to replace previous data file");

    uint32_t retryCount = 0;
#   ifdef _WINDOWS
    while (_wrename(tmpFile.c_str(), outputFile.c_str()) < 0)
#   else
    while (rename(tmpFile.c_str(), outputFile.c_str()) < 0)
#   endif
    {
      if (++retryCount > __MAX_RETRIES)
        throw std::runtime_error("Failed to rename temporary file");
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
  }
}

// Read JSON from config file
static SerializableValue::Object __readJsonFile(const UnicodeString& sourceFilePath) {
  auto input = pandora::io::openFileEntry(sourceFilePath.c_str(), __UNICODE_STR("rt"));
  if (!input.isOpen())
    throw std::runtime_error("Config file not found or not readable");

  fseek(input.handle(), 0, SEEK_END);
  long fileSize = ftell(input.handle());
  fseek(input.handle(), 0, SEEK_SET);
  if (fileSize <= 0)
    throw std::runtime_error("Config file empty or not readable");

  auto fileData = std::unique_ptr<char[]>(new char[fileSize + 1]);
  fread(&fileData[0], 1, fileSize, input.handle());
  fileData[fileSize] = 0;
  input.close();

  pandora::io::JsonSerializer deserializer;
  return deserializer.fromString(&fileData[0]); // throws if invalid syntax
}

// ---

static void __writeSystemString(SerializableValue::Object& parent, const char* key, const UnicodeString& value) {
  if (!value.empty()) {
#   ifdef _WINDOWS
      auto utf8Val = pandora::io::Encoder::Utf8::fromUtf16((const char16_t*)value.c_str());
      parent.emplace(key, SerializableValue(utf8Val.c_str()));
#   else
      parent.emplace(key, SerializableValue(value.c_str()));
#   endif
  }
}

template <typename _IntegerType>
static void __writeIntegerArray(SerializableValue::Object& parent, const char* key,
                                const _IntegerType* arrayVal, size_t length) {
  SerializableValue::Array values;
  while (length) {
    values.emplace_back((int32_t)*arrayVal);
    ++arrayVal;
    --length;
  }
  parent.emplace(key, std::move(values));
}

// ---

template <typename _OutType>
static inline _OutType __readInteger(const SerializableValue::Object& parent, const char* key, _OutType defaultVal) {
  auto itProp = parent.find(key);
  return (itProp != parent.end()) ? (_OutType)itProp->second.getInteger() : defaultVal;
}

static inline float __readFloat(const SerializableValue::Object& parent, const char* key, float defaultVal) {
  auto itProp = parent.find(key);
  return (itProp != parent.end()) ? (float)itProp->second.getNumber() : defaultVal;
}

static void __readSystemString(const SerializableValue::Object& parent, const char* key, UnicodeString& outValue) {
  auto itProp = parent.find(key);
  const char* utf8Value = (itProp != parent.end()) ? itProp->second.getText() : nullptr;

# ifdef _WINDOWS // to UTF-16 on Windows
  if (utf8Value) {
    auto utf16Value = pandora::io::Encoder::Utf16::fromUtf8(utf8Value);
    outValue.assign((const wchar_t*)utf16Value.c_str());
  }
# else
  if (utf8Value) {
    outValue.assign(utf8Value);
  }
# endif
  else
    outValue.clear();
}

template <typename _IntegerType>
static void __readIntegerArray(const SerializableValue::Object& parent, const char* key,
                               _IntegerType* outArray, size_t maxArraySize) {
  auto nodeId = parent.find(key);
  if (nodeId != parent.end()) {
    auto* arrayItems = nodeId->second.getArray();
    size_t length = nodeId->second.size();
    if (arrayItems != nullptr && length <= maxArraySize) { // not empty, not too long
      for (auto& it : *arrayItems) {
        *outArray = (_IntegerType)it.getInteger();
        ++outArray;
      }
    }
  }
}

static void __getDefaultProfileName(uint32_t index, UnicodeString& outValue) {
  outValue = __UNICODE_STR("Profile ");
# ifdef _WINDOWS
    wchar_t buffer[4];
    _itow_s(index, buffer, 4, 10);
    outValue.append(buffer);
# else
    char buffer[4];
    outValue.append(itoa(index, buffer, 10));
# endif
}


// -- serialization -- ---------------------------------------------------------

// Serialize common config to JSON file
void Serializer::writeMainConfigFile(const UnicodeString& configDir, const VideoConfig& rendererCfg,
                                     const WindowConfig& windowCfg, const ActionsConfig& actionsCfg) {
  SerializableValue::Object jsonObject;

  // video params
  if (rendererCfg.api != defaultRenderingApi()) // no need to store empty values (reader sets same empty values if not in file)
    jsonObject.emplace(video::api(), SerializableValue((int32_t)rendererCfg.api));
  if (rendererCfg.enableVsync)
    jsonObject.emplace(video::enableVsync(), SerializableValue((int32_t)rendererCfg.enableVsync)); // store bool as int (faster)
  if (rendererCfg.enableFramerateLimit)
    jsonObject.emplace(video::enableFramerateLimit(), SerializableValue((int32_t)rendererCfg.enableFramerateLimit));
  if (rendererCfg.framerateLimit != autodetectFramerate())
    jsonObject.emplace(video::framerateLimit(), SerializableValue(rendererCfg.framerateLimit));
  if (rendererCfg.frameSkip != FrameSkipping::none)
    jsonObject.emplace(video::frameSkip(), SerializableValue((int32_t)rendererCfg.frameSkip));
  if (rendererCfg.precision != PrecisionMode::standard)
    jsonObject.emplace(video::precision(), SerializableValue((int32_t)rendererCfg.precision));
  if (rendererCfg.osd != OnScreenDisplay::none)
    jsonObject.emplace(video::osd(), SerializableValue((int32_t)rendererCfg.osd));

  // window params
  __writeSystemString(jsonObject, window::monitorId(), windowCfg.monitorId);
  jsonObject.emplace(window::windowMode(), SerializableValue((int32_t)windowCfg.windowMode));
  jsonObject.emplace(window::windowHeight(), SerializableValue((int32_t)windowCfg.windowHeight));
  if (windowCfg.fullscreen.width != desktopResolution())
    jsonObject.emplace(window::fullscreenResX(), SerializableValue((int32_t)windowCfg.fullscreen.width));
  if (windowCfg.fullscreen.height != desktopResolution())
    jsonObject.emplace(window::fullscreenResY(), SerializableValue((int32_t)windowCfg.fullscreen.height));
  if (windowCfg.fullscreen.refreshRate != defaultRefreshRateMilli())
    jsonObject.emplace(window::fullscreenRate(), SerializableValue((int32_t)windowCfg.fullscreen.refreshRate));
  if (windowCfg.isWideSource)
    jsonObject.emplace(window::isWideSource(), SerializableValue((int32_t)windowCfg.isWideSource));

  // actions params
  __writeIntegerArray(jsonObject, actions::keyboardMapping(), &actionsCfg.keyboardMapping[0], keyboardMap::length());
  __writeIntegerArray(jsonObject, actions::controllerMapping(), &actionsCfg.controllerMapping[0], controllerMap::length());
  if (actionsCfg.controllerHotkey)
    jsonObject.emplace(actions::controllerHotkey(), SerializableValue((int32_t)actionsCfg.controllerHotkey));

  uint32_t retryCount = 0;
  bool isSuccess = false;
  do {
    try {
      __writeJsonFile(configDir + mainConfigFileName(), jsonObject);
      isSuccess = true;
    }
    catch (...) {
      if (++retryCount > __MAX_RETRIES)
        throw;
    }
  } while (!isSuccess);
}

// Serialize list of profile labels to JSON file
void Serializer::writeProfileListFile(const UnicodeString& configDir, const std::vector<ProfileLabel>& profiles) {
  SerializableValue::Object jsonObject;
  if (!profiles.empty()) {
    SerializableValue::Array values;

    for (auto& prf : profiles) {
      SerializableValue::Object profileLabel;
      profileLabel.emplace(profile::id(), SerializableValue((int32_t)prf.id));
      __writeSystemString(profileLabel, profile::name(), prf.name);
      __writeSystemString(profileLabel, profile::file(), prf.file);
      profileLabel.emplace(profile::tileColor(), SerializableValue((int32_t)prf.tileColor));

      values.emplace_back(std::move(profileLabel));
    }
    jsonObject.emplace(profile::_array(), std::move(values));
  }

  uint32_t retryCount = 0;
  bool isSuccess = false;
  do {
    try {
      __writeJsonFile(configDir + profileListFileName(), jsonObject);
      isSuccess = true;
    }
    catch (...) {
      if (++retryCount > __MAX_RETRIES)
        throw;
    }
  } while (!isSuccess);
}

// Serialize config profile to JSON file
void Serializer::writeProfileConfigFile(const UnicodeString& outputFilePath, const RendererProfile& rendererCfg) {
  SerializableValue::Object jsonObject;

  // viewport
  if (rendererCfg.screenStretching)
    jsonObject.emplace(profile::renderer::screenStretching(), SerializableValue((int32_t)rendererCfg.screenStretching));
  if (rendererCfg.screenCropping)
    jsonObject.emplace(profile::renderer::screenCropping(), SerializableValue((int32_t)rendererCfg.screenCropping));
  if (rendererCfg.isPalRecentered)
    jsonObject.emplace(profile::renderer::isPalRecentered(), SerializableValue((int32_t)rendererCfg.isPalRecentered));
  if (rendererCfg.isOverscanVisible)
    jsonObject.emplace(profile::renderer::isOverscanVisible(), SerializableValue((int32_t)rendererCfg.isOverscanVisible));
  if (rendererCfg.isMirrored)
    jsonObject.emplace(profile::renderer::isMirrored(), SerializableValue((int32_t)rendererCfg.isMirrored));
  if (rendererCfg.screenCurvature)
    jsonObject.emplace(profile::renderer::screenCurvature(), SerializableValue((int32_t)rendererCfg.screenCurvature));
  __writeIntegerArray<uint8_t>(jsonObject, profile::renderer::blackBorderSizes(), &rendererCfg.blackBorderSizes[0], 4);
  
  // rendering
  if (rendererCfg.internalResFactorX > 1u)
    jsonObject.emplace(profile::renderer::internalResFactorX(), SerializableValue((int32_t)rendererCfg.internalResFactorX));
  if (rendererCfg.internalResFactorY > 1u)
    jsonObject.emplace(profile::renderer::internalResFactorY(), SerializableValue((int32_t)rendererCfg.internalResFactorY));
  if (rendererCfg.colorMode != ColorOutput::rgb32)
    jsonObject.emplace(profile::renderer::colorMode(), SerializableValue((int32_t)rendererCfg.colorMode));
  if (rendererCfg.fillMode != FillMode::normal)
    jsonObject.emplace(profile::renderer::fillMode(), SerializableValue((int32_t)rendererCfg.fillMode));
  if (rendererCfg.antiAliasing != AntiAliasing::none)
    jsonObject.emplace(profile::renderer::antiAliasing(), SerializableValue((int32_t)rendererCfg.antiAliasing));

  // upscaling
  if (rendererCfg.textureUpscaling != UpscalingFilter::none)
    jsonObject.emplace(profile::renderer::textureUpscaling(), SerializableValue((int32_t)rendererCfg.textureUpscaling));
  if (rendererCfg.textureUpscalingFactor > 1)
    jsonObject.emplace(profile::renderer::textureUpscalingFactor(), SerializableValue((int32_t)rendererCfg.textureUpscalingFactor));
  if (rendererCfg.useTextureBilinear)
    jsonObject.emplace(profile::renderer::useTextureBilinear(), SerializableValue((int32_t)rendererCfg.useTextureBilinear));
  if (rendererCfg.spriteUpscaling != UpscalingFilter::none)
    jsonObject.emplace(profile::renderer::spriteUpscaling(), SerializableValue((int32_t)rendererCfg.spriteUpscaling));
  if (rendererCfg.spriteUpscalingFactor > 1)
    jsonObject.emplace(profile::renderer::spriteUpscalingFactor(), SerializableValue((int32_t)rendererCfg.spriteUpscalingFactor));
  if (rendererCfg.useSpriteBilinear)
    jsonObject.emplace(profile::renderer::useSpriteBilinear(), SerializableValue((int32_t)rendererCfg.useSpriteBilinear));
  if (rendererCfg.screenUpscaling != UpscalingFilter::none)
    jsonObject.emplace(profile::renderer::screenUpscaling(), SerializableValue((int32_t)rendererCfg.screenUpscaling));
  if (rendererCfg.screenUpscalingFactor > 1)
    jsonObject.emplace(profile::renderer::screenUpscalingFactor(), SerializableValue((int32_t)rendererCfg.screenUpscalingFactor));
  if (rendererCfg.mdecUpscaling != MdecFilter::none)
    jsonObject.emplace(profile::renderer::mdecUpscaling(), SerializableValue((int32_t)rendererCfg.mdecUpscaling));

  // noise effects
  if (rendererCfg.textureGrain != NoiseFilter::none)
    jsonObject.emplace(profile::renderer::textureGrain(), SerializableValue((int32_t)rendererCfg.textureGrain));
  if (rendererCfg.screenGrain != NoiseFilter::none)
    jsonObject.emplace(profile::renderer::screenGrain(), SerializableValue((int32_t)rendererCfg.screenGrain));
  if (rendererCfg.dithering != ColorDithering::none)
    jsonObject.emplace(profile::renderer::dithering(), SerializableValue((int32_t)rendererCfg.dithering));
  if (rendererCfg.useTextureDithering)
    jsonObject.emplace(profile::renderer::useTextureDithering(), SerializableValue((int32_t)rendererCfg.useTextureDithering));
  if (rendererCfg.useSpriteDithering)
    jsonObject.emplace(profile::renderer::useSpriteDithering(), SerializableValue((int32_t)rendererCfg.useSpriteDithering));
  
  //...

  uint32_t retryCount = 0;
  bool isSuccess = false;
  do {
    try {
      __writeJsonFile(outputFilePath, jsonObject);
      isSuccess = true;
    }
    catch (...) {
      if (++retryCount > __MAX_RETRIES)
        throw;
    }
  } while (!isSuccess);
}


// -- deserialization -- -------------------------------------------------------

// Deserialize common config from JSON file
void Serializer::readMainConfigFile(const UnicodeString& configDir, VideoConfig& outRendererCfg,
                                    WindowConfig& outWindowCfg, ActionsConfig& outActionsCfg) {
  auto jsonObject = __readJsonFile(configDir + mainConfigFileName()); // throws

  // video params
  outRendererCfg.api = __readInteger(jsonObject, video::api(), defaultRenderingApi());
  outRendererCfg.enableVsync = __readInteger<bool>(jsonObject, video::enableVsync(), false);
  outRendererCfg.enableFramerateLimit = __readInteger<bool>(jsonObject, video::enableFramerateLimit(), false);
  outRendererCfg.framerateLimit = __readFloat(jsonObject, video::framerateLimit(), autodetectFramerate());
  outRendererCfg.frameSkip = __readInteger(jsonObject, video::frameSkip(), FrameSkipping::none);
  outRendererCfg.precision = __readInteger(jsonObject, video::precision(), PrecisionMode::standard);
  outRendererCfg.osd = __readInteger(jsonObject, video::osd(), OnScreenDisplay::none);

  // window params
  __readSystemString(jsonObject, window::monitorId(), outWindowCfg.monitorId);
  outWindowCfg.windowMode = __readInteger(jsonObject, window::windowMode(), WindowMode::fullscreenWindow);
  outWindowCfg.windowHeight = __readInteger(jsonObject, window::windowHeight(), defaultWindowHeight());
  outWindowCfg.fullscreen.width = __readInteger(jsonObject, window::fullscreenResX(), desktopResolution());
  outWindowCfg.fullscreen.height = __readInteger(jsonObject, window::fullscreenResY(), desktopResolution());
  outWindowCfg.fullscreen.refreshRate = __readInteger(jsonObject, window::fullscreenRate(), defaultRefreshRateMilli());
  outWindowCfg.isWideSource = __readInteger<bool>(jsonObject, window::isWideSource(), false);

  // actions params
  memset(outActionsCfg.keyboardMapping, disabledKey(), keyboardMap::length()*sizeof(*outActionsCfg.keyboardMapping));
  __readIntegerArray(jsonObject, actions::keyboardMapping(), &outActionsCfg.keyboardMapping[0], keyboardMap::length());
  memset(outActionsCfg.controllerMapping, disabledKey(), controllerMap::length()*sizeof(*outActionsCfg.controllerMapping));
  __readIntegerArray(jsonObject, actions::controllerMapping(), &outActionsCfg.controllerMapping[0], controllerMap::length());
  outActionsCfg.controllerHotkey = __readInteger(jsonObject, actions::controllerHotkey(), disabledKey());
}

// Deserialize list of profile labels from JSON file
void Serializer::readProfileListFile(const UnicodeString& configDir, std::vector<ProfileLabel>& outProfiles) {
  auto jsonObject = __readJsonFile(configDir + profileListFileName()); // throws

  outProfiles.clear();
  auto profileListIt = jsonObject.find(profile::_array());
  if (profileListIt != jsonObject.end()) {
    auto* profiles = profileListIt->second.getArray();
    if (profiles != nullptr) { // not empty

      uint32_t index = 0;
      for (auto& p : *profiles) {
        auto* profile = p.getObject();
        if (profile != nullptr) {
#         if !defined(_CPP_REVISION) || _CPP_REVISION != 14
            ProfileLabel& label = outProfiles.emplace_back();
#         else
            outProfiles.emplace_back();
            ProfileLabel& label = outProfiles.back();
#         endif

          label.id = __readInteger<uint32_t>((*profile), profile::id(), 0);
          __readSystemString((*profile), profile::file(), label.file);
          if (label.file.empty()) {
            outProfiles.pop_back(); // skip non-existing profiles
            continue;
          }

          __readSystemString((*profile), profile::name(), label.name);
          if (label.name.empty())
            __getDefaultProfileName(index, label.name);
          label.tileColor = __readInteger((*profile), profile::tileColor(), MenuTileColor::white);
        }
      }
    }
  }
}
// Deserialize list of profile menu tiles from JSON file
void Serializer::readProfileListFile(const UnicodeString& configDir, std::unordered_map<ProfileId,ProfileMenuTile>& outProfiles) {
  std::vector<ProfileLabel> labels;
  readProfileListFile(configDir, labels);
  outProfiles.clear();

  if (!labels.empty()) {
    for (auto& it : labels)
      outProfiles.emplace(it.id, std::move(it));
  }
}

// Deserialize config profile from JSON file
void Serializer::readProfileConfigFile(const UnicodeString& sourceFilePath, RendererProfile& outRendererCfg) {
  auto jsonObject = __readJsonFile(sourceFilePath); // throws

  // viewport
  outRendererCfg.screenStretching = __readInteger(jsonObject, profile::renderer::screenStretching(), 0);
  if (outRendererCfg.screenStretching > maxScreenFraming())
    outRendererCfg.screenStretching = maxScreenFraming();
  outRendererCfg.screenCropping = __readInteger(jsonObject, profile::renderer::screenCropping(), 0);
  if (outRendererCfg.screenCropping > maxScreenFraming())
    outRendererCfg.screenCropping = maxScreenFraming();
  outRendererCfg.isPalRecentered = __readInteger<bool>(jsonObject, profile::renderer::isPalRecentered(), false);
  outRendererCfg.isOverscanVisible = __readInteger<bool>(jsonObject, profile::renderer::isOverscanVisible(), false);
  outRendererCfg.isMirrored = __readInteger<bool>(jsonObject, profile::renderer::isMirrored(), false);
  outRendererCfg.screenCurvature = __readInteger(jsonObject, profile::renderer::screenCurvature(), 0);
  if (outRendererCfg.screenCurvature > maxScreenFraming())
    outRendererCfg.screenCurvature = maxScreenFraming();
  memset(outRendererCfg.blackBorderSizes, 0, 4*sizeof(*outRendererCfg.blackBorderSizes));
  __readIntegerArray(jsonObject, profile::renderer::blackBorderSizes(), &outRendererCfg.blackBorderSizes[0], 4);
  
  // rendering
  outRendererCfg.internalResFactorX = __readInteger<uint32_t>(jsonObject, profile::renderer::internalResFactorX(), 1u);
  if (outRendererCfg.internalResFactorX > maxInternalResFactor())
    outRendererCfg.internalResFactorX = maxInternalResFactor();
  outRendererCfg.internalResFactorY = __readInteger<uint32_t>(jsonObject, profile::renderer::internalResFactorY(), 1u);
  if (outRendererCfg.internalResFactorY > maxInternalResFactor())
    outRendererCfg.internalResFactorY = maxInternalResFactor();
  outRendererCfg.colorMode = __readInteger(jsonObject, profile::renderer::colorMode(), ColorOutput::rgb32);
  outRendererCfg.fillMode = __readInteger(jsonObject, profile::renderer::fillMode(), FillMode::normal);
  outRendererCfg.antiAliasing = __readInteger(jsonObject, profile::renderer::antiAliasing(), AntiAliasing::none);

  // upscaling
  outRendererCfg.textureUpscaling = __readInteger(jsonObject, profile::renderer::textureUpscaling(), UpscalingFilter::none);
  outRendererCfg.textureUpscalingFactor = __readInteger(jsonObject, profile::renderer::textureUpscalingFactor(), 1);
  outRendererCfg.useTextureBilinear = __readInteger<bool>(jsonObject, profile::renderer::useTextureBilinear(), false);
  outRendererCfg.spriteUpscaling = __readInteger(jsonObject, profile::renderer::spriteUpscaling(), UpscalingFilter::none);
  outRendererCfg.spriteUpscalingFactor = __readInteger(jsonObject, profile::renderer::spriteUpscalingFactor(), 1);
  outRendererCfg.useSpriteBilinear = __readInteger<bool>(jsonObject, profile::renderer::useSpriteBilinear(), false);
  outRendererCfg.screenUpscaling = __readInteger(jsonObject, profile::renderer::screenUpscaling(), UpscalingFilter::none);
  outRendererCfg.screenUpscalingFactor = __readInteger(jsonObject, profile::renderer::screenUpscalingFactor(), 1);
  outRendererCfg.mdecUpscaling = __readInteger(jsonObject, profile::renderer::mdecUpscaling(), MdecFilter::none);

  // noise effects
  outRendererCfg.textureGrain = __readInteger(jsonObject, profile::renderer::textureGrain(), NoiseFilter::none);
  outRendererCfg.screenGrain = __readInteger(jsonObject, profile::renderer::screenGrain(), NoiseFilter::none);
  outRendererCfg.dithering = __readInteger(jsonObject, profile::renderer::dithering(), ColorDithering::none);
  outRendererCfg.useTextureDithering = __readInteger<bool>(jsonObject, profile::renderer::useTextureDithering(), false);
  outRendererCfg.useSpriteDithering = __readInteger<bool>(jsonObject, profile::renderer::useSpriteDithering(), false);
  
  //...
}
