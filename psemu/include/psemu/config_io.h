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
#pragma once

#include <vector>
#include "config/config.h"

namespace psemu {
  // -- config directory management -- -------------------------------------------

  /// @brief Show message-box to choose config directory + create it
  /// @throws - runtime_error on directory creation failure;
  ///         - bad_alloc on memory allocation failure.
  config::UnicodeString createConfigDirectory(const config::UnicodeString& pluginDir);


  // -- config profile management -- ---------------------------------------------

  /// @brief Load global/common config (or create file if missing)
  /// @warning On failure, keeps current config values.
  void loadGlobalConfig(const config::UnicodeString& configDir, config::VideoConfig& outVideoConfig,
                        config::WindowConfig& outWindowConfig, config::ActionsConfig& outInputConfig) noexcept;

  /// @brief Read list of profiles (or create file if missing)
  std::vector<config::ProfileMenuTile> readListOfProfiles(const config::UnicodeString& configDir) noexcept;

  /// @brief Load config profile associated with current game ID (if available)
  /// @warning On failure, keeps current config values.
  void loadGameConfigProfile(const config::UnicodeString& configDir, const pandora::memory::LightString& gameId,
                             const std::vector<config::ProfileMenuTile>& profiles,
                             config::RendererProfile& outConfigProfile) noexcept;
}
