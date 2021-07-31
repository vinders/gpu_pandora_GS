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
#include <unordered_map>
#include "config/config.h"

namespace config {
  /// @brief Config/profile JSON file reader/writer
  class Serializer final {
  public:
    Serializer() = delete;

    // -- DIRECTORY & BINDING -- -----------------------------------------------

    // -- directory/file utils --

    static constexpr inline auto mainConfigFileName() noexcept { return __UNICODE_STR("common.cfg"); }    ///< common config file
    static constexpr inline auto profileListFileName() noexcept { return __UNICODE_STR("profiles.cfg"); } ///< list of profile labels

    /// @brief Create config directory
    /// @param usePortableLocation  Use portable config ('plugins' directory) or local user config ('AppData/Roaming')
    /// @returns Success
    static bool createConfigDir(bool usePortableLocation);
    /// @brief Find config directory (ending with directory separator)
    /// @returns Config directory found (priority given to portable location) or empty string (if no directory or inaccessible)
    static UnicodeString findConfigDir();
    /// @brief Verify if portable directory ('plugins') has write access
    static bool isPortableLocationAvailable();

    
    // -- game/profile bindings --

    static constexpr inline auto lastBindingFileName() noexcept { return __UNICODE_STR(".last.bind"); } ///< last used profile file name
    static constexpr inline auto gameBindingDirectory() noexcept { return __UNICODE_STR(".bind"); } ///< directory for game/profile bindings

    /// @brief Find profile associated with current game (saved at the end of last execution)
    /// @returns ID of associated profile (if binding found) or last used profile ID (empty if no profile has ever been loaded).
    static ProfileId findGameProfileBinding(const UnicodeString& configDir, const char* gameId) noexcept;
    /// @brief Associate profile with current game (for the next time) + save profile as "last used profile"
    static bool saveGameProfileBinding(const UnicodeString& configDir, const char* gameId, ProfileId profileId) noexcept;


    // -- JSON SERIALIZATION -- ------------------------------------------------

    // -- config serialization --

    /// @brief Serialize common config to JSON file
    /// @param configDir  Path to config directory (must include a directory separator at the end)
    /// @throws - runtime_error if the file was could not be written
    ///         - bad_alloc on allocation failure
    static void writeMainConfigFile(const UnicodeString& configDir, const RendererConfig& rendererCfg,
                                    const WindowConfig& windowCfg, const ActionsConfig& actionsCfg);
    /// @brief Serialize list of profile labels to JSON file
    /// @param configDir  Path to config directory (must include a directory separator at the end)
    /// @throws - runtime_error if the file was could not be written
    ///         - bad_alloc on allocation failure
    static void writeProfileListFile(const UnicodeString& configDir, const std::vector<ProfileLabel>& profiles);
    /// @brief Serialize config profile to JSON file
    /// @throws - runtime_error if the file was could not be written
    ///         - bad_alloc on allocation failure
    static void writeProfileConfigFile(const UnicodeString& outputFilePath, const RendererProfile& rendererCfg,
                                       const WindowProfile& windowCfg, const EffectsProfile& effectsCfg);

    // -- config deserialization --

    /// @brief Deserialize common config from JSON file
    /// @param configDir  Path to config directory (must include a directory separator at the end)
    /// @throws - runtime_error if the file was not found or not readable
    ///         - invalid_argument if the JSON syntax is invalid
    ///         - bad_alloc on allocation failure
    static void readMainConfigFile(const UnicodeString& configDir, RendererConfig& outRendererCfg,
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
    static void readProfileListFile(const UnicodeString& configDir, std::unordered_map<ProfileId,ProfileMenuTile>& outProfiles);
    /// @brief Deserialize config profile from JSON file
    /// @throws - runtime_error if the file was not found or not readable
    ///         - invalid_argument if the JSON syntax is invalid
    ///         - bad_alloc on allocation failure
    static void readProfileConfigFile(const UnicodeString& sourceFilePath, RendererProfile& outRendererCfg,
                                      WindowProfile& outWindowCfg, EffectsProfile& outEffectsCfg);
  };
}
