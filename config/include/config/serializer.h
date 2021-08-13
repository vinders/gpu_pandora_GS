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

#include <memory>
#include <vector>
#include "config/config.h"

namespace config {
  /// @brief Config/profile JSON file reader/writer
  class Serializer final {
  public:
    Serializer() = delete;

    // -- GAME/PROFILE BINDINGS -- ---------------------------------------------

    /// @brief Find profile associated with current game (saved at the end of last execution)
    /// @returns ID of associated profile (if binding found) or last used profile ID (empty if no profile has ever been loaded).
    static ProfileId readGameProfileBinding(const UnicodeString& configDir, const char* gameId) noexcept;
    /// @brief Associate profile with current game (for the next time) + save profile as "last used profile"
    static bool saveGameProfileBinding(const UnicodeString& configDir, const char* gameId, ProfileId profileId) noexcept;


    // -- JSON SERIALIZATION -- ------------------------------------------------

    // -- config serialization --

    /// @brief Serialize common config to JSON file
    /// @param configDir  Path to config directory (must include a directory separator at the end)
    /// @throws - runtime_error if the file was could not be written
    ///         - bad_alloc on allocation failure
    static void writeMainConfigFile(const UnicodeString& configDir, const VideoConfig& videoCfg,
                                    const WindowConfig& windowCfg, const ActionsConfig& actionsCfg);
    /// @brief Serialize list of profile labels to JSON file
    /// @param configDir  Path to config directory (must include a directory separator at the end)
    /// @throws - runtime_error if the file was could not be written
    ///         - bad_alloc on allocation failure
    static void writeProfileListFile(const UnicodeString& configDir, const std::vector<ProfileLabel>& profiles);
    /// @brief Serialize config profile to JSON file
    /// @throws - runtime_error if the file was could not be written
    ///         - bad_alloc on allocation failure
    static void writeProfileConfigFile(const UnicodeString& outputFilePath, const RendererProfile& rendererCfg);

    // -- config deserialization --

    /// @brief Deserialize common config from JSON file
    /// @param configDir  Path to config directory (must include a directory separator at the end)
    /// @throws - runtime_error if the file was not found or not readable
    ///         - invalid_argument if the JSON syntax is invalid
    ///         - bad_alloc on allocation failure
    static void readMainConfigFile(const UnicodeString& configDir, VideoConfig& outVideoCfg,
                                   WindowConfig& outWindowCfg, ActionsConfig& outActionsCfg);
    /// @brief Deserialize list of profile labels from JSON file - config dialog
    /// @param configDir  Path to config directory (must include a directory separator at the end)
    /// @throws - runtime_error if the file was not found or not readable
    ///         - invalid_argument if the JSON syntax is invalid
    ///         - bad_alloc on allocation failure
    static void readProfileListFile(const UnicodeString& configDir, std::vector<ProfileLabel>& outProfiles);
    /// @brief Deserialize list of profile menu tiles from JSON file - ingame menu
    /// @param configDir  Path to config directory (must include a directory separator at the end)
    /// @throws - runtime_error if the file was not found or not readable
    ///         - invalid_argument if the JSON syntax is invalid
    ///         - bad_alloc on allocation failure
    static void readProfileListFile(const UnicodeString& configDir, std::vector<ProfileMenuTile>& outProfiles);
    /// @brief Deserialize config profile from JSON file
    /// @throws - runtime_error if the file was not found or not readable
    ///         - invalid_argument if the JSON syntax is invalid
    ///         - bad_alloc on allocation failure
    static void readProfileConfigFile(const UnicodeString& sourceFilePath, RendererProfile& outRendererCfg);
  };
}
