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
#include <video/message_box.h>
#include "config/file_path_utils.h"
#include "config/serializer.h"
#include "config/presets.h"
#include "utils/syslog.h"
#include "psemu/config_io.h"
// this implementation is included in psemu_gpu.cpp -> single object file (less overhead + smaller lib size)

using pandora::video::MessageBox;
using utils::SysLog;


// -- config directory management -- -------------------------------------------

// Show message-box to choose config directory + create it
config::UnicodeString psemu::createConfigDirectory(const config::UnicodeString& pluginDir) {
  bool isUserDir = true;
  if (config::isPathWritable(pluginDir.c_str())) {
    auto choice = MessageBox::show(__UNICODE_STR("First config initialization"),
                      __UNICODE_STR("Please choose where to create config files:\n\n"
                                    "* Local: shared with other emulators (per user)\n"
                                    "* Portable: in emulator's directory"),
                      MessageBox::IconType::question,
                      __UNICODE_STR("Local"), __UNICODE_STR("Portable"));
    isUserDir = (choice != MessageBox::Result::action2);
  }

  config::UnicodeString configDir;
  if (isUserDir) {
    auto parentDir = config::getLocalUserParentDir();
    if (!config::isPathReadable(parentDir.c_str()))
      config::createDirectory(parentDir.c_str());
    configDir = config::toLocalUserConfigDir(parentDir);
  }
  else
    configDir = config::getPortableConfigDir(pluginDir);

  if (!config::createDirectory(configDir.c_str())) {
    MessageBox::show(__UNICODE_STR("Config creation failure"), __UNICODE_STR("Failed to create config directory..."),
                     MessageBox::ActionType::ok, MessageBox::IconType::error);
    throw std::runtime_error("GPUinit: config directory creation failed");
  }
  return configDir;
}


// -- config profile management -- ---------------------------------------------

// Load global/common config (or create file if missing)
void psemu::loadGlobalConfig(const config::UnicodeString& configDir, config::VideoConfig& outVideoConfig,
                             config::WindowConfig& outWindowConfig, config::ActionsConfig& outInputConfig) noexcept {
  try {
    config::Serializer::readGlobalConfigFile(configDir, outVideoConfig, outWindowConfig, outInputConfig);
  }
  catch (const std::exception& exc) {
    if (!configDir.empty() && !config::isPathReadable(configDir + config::globalConfigFileName())) { // not found -> create it
      outInputConfig.initDefaultMapping();
      config::Serializer::writeGlobalConfigFile(configDir, outVideoConfig, outWindowConfig, outInputConfig);
    }
    else // file corrupted or alloc failure
      SysLog::logError(__FILE_NAME__, __LINE__, exc.what());
  }
}

// Read list of profiles (or create file if missing)
std::vector<config::ProfileMenuTile> psemu::readListOfProfiles(const config::UnicodeString& configDir) noexcept {
  std::vector<config::ProfileMenuTile> profiles;
  try {
    config::Serializer::readProfileListFile(configDir, profiles);
  }
  catch (const std::exception& exc) {
    if (!configDir.empty() && !config::isPathReadable(configDir + config::profileListFileName())) // not found -> create it
      config::Serializer::writeProfileListFile(configDir, std::vector<config::ProfileLabel>{});
    else // file corrupted
      SysLog::logError(__FILE_NAME__, __LINE__, exc.what());
  }
  return profiles;
}

// ---

// Load config profile associated with current game ID (if available)
void psemu::loadGameConfigProfile(const config::UnicodeString& configDir, const pandora::memory::LightString& gameId,
                                  const std::vector<config::ProfileMenuTile>& profiles,
                                  config::RendererProfile& outConfigProfile) noexcept {
  auto targetId = config::Serializer::readGameProfileBinding(configDir, gameId.c_str());

  if (config::isPresetId(targetId)) { // target is a preset
    config::loadPreset((config::PresetId)targetId, outConfigProfile);
  }
  else if (!profiles.empty()) { // target is a profile
    try {
      const config::ProfileMenuTile* targetProfile = &profiles[0]; // default to first profile (if target not found)
      for (auto& it : profiles) {
        if (it.id == targetId) {
          targetProfile = &it;
          break;
        }
      }
      config::Serializer::readProfileConfigFile(targetProfile->file, outConfigProfile);
    }
    catch (const std::exception& exc) { // corrupted profile or alloc failure
      SysLog::logError(__FILE_NAME__, __LINE__, exc.what());
    }
  }// no profile (or error): keep current values
}
