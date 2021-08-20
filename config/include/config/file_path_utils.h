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

#include <io/file_handle.h>
#include "config/types.h"

namespace config {
  // -- file / path utilities --

  /// @brief Create/open file (mode: wt/wb/rt/rb/...)
  /// @return Valid file handle (or empty handle on failure)
  pandora::io::FileHandle openFile(const __UNICODE_CHAR* path, const __UNICODE_CHAR* mode) noexcept;
  bool removeFile(const __UNICODE_CHAR* path) noexcept;      ///< Remove existing file
  bool createDirectory(const __UNICODE_CHAR* path) noexcept; ///< Create new directory (its parent must exist)

  // ---

  bool isPathReadable(const __UNICODE_CHAR* path) noexcept; ///< Verify if a path exists and is readable
  inline bool isPathReadable(const UnicodeString& path) noexcept { return isPathReadable(path.c_str()); }
  bool isPathWritable(const __UNICODE_CHAR* path) noexcept; ///< Verify if a path exists and is writable

  UnicodeString getCurrentDir();  ///< Read current execution directory path (no trailing separator)
  UnicodeString getProcessPath(); ///< Read current process file path and name (if available)


  // -- emulator detection --

  /// @brief Detect emulator type and directories
  /// @remarks Used for config files, for special tweaks and to call appropriate F-keys on menu action events
  /// @throws bad_alloc on memory allocation failure
  void readEmulatorInfo(EmulatorInfo& outInfo);
  /// @brief Detect if emulator uses a widescreen hack
  bool getEmulatorWidescreenState(EmulatorType type) noexcept;


  // -- config file paths --

  /// @brief Get portable config directory path (in plugins directory) -- with trailing separator
  /// @remarks This path may not be writable: verify with 'isPathWritable' and only use if writable
  /// @throws bad_alloc on memory allocation failure
  inline UnicodeString getPortableConfigDir(const UnicodeString& pluginDir) {
    return pluginDir + __UNICODE_STR(".gpuPandoraGS" __ABS_PATH_SEP);
  }
  /// @brief Get local user config directory path (ex: AppData on Windows) -- with trailing separator
  /// @throws bad_alloc on memory allocation failure
  UnicodeString getLocalUserConfigDir();
  /// @brief Get parent directory of local user config -- with trailing separator
  /// @remarks In many cases, the parent directory will not exist -> it must be created first, before creating config subdir.
  /// @throws bad_alloc on memory allocation failure
  UnicodeString getLocalUserParentDir();
  /// @brief Convert local user parent directory to config path (append missing part)
  UnicodeString toLocalUserConfigDir(const UnicodeString& localUserParentDir);

  /// @brief Find existing config directory path -- with trailing separator
  /// @pluginDir Path of emulator directory containing plugins (call 'readEmulatorInfo')
  /// @returns Config directory path (or empty string if not found or not writable)
  /// @remarks If both portable/local directories exist, priority is given to the portable location.
  /// @throws bad_alloc on memory allocation failure
  UnicodeString findConfigDir(const UnicodeString& pluginDir);

  // ---

  constexpr inline const __UNICODE_CHAR* globalConfigFileName() noexcept { return __UNICODE_STR("global.cfg"); }
  constexpr inline const __UNICODE_CHAR* profileListFileName() noexcept { return __UNICODE_STR("profiles.cfg"); }

  /// @brief Build global config file path (from config directory)
  /// @throws bad_alloc on memory allocation failure
  inline UnicodeString getGlobalConfigPath(const UnicodeString& configDir) { return configDir + globalConfigFileName(); }
  /// @brief Build global config file path (from config directory)
  /// @throws bad_alloc on memory allocation failure
  inline UnicodeString getProfileListPath(const UnicodeString& configDir) { return configDir + profileListFileName(); }


  // -- game/profile binding paths --

  constexpr inline const __UNICODE_CHAR* latestBindingFileName() noexcept { return __UNICODE_STR("_last.bind"); }
  constexpr inline const __UNICODE_CHAR* gameBindingsDirectory() noexcept { return __UNICODE_STR("bind"); }

  /// @brief Build global config file path (from config directory) -- no trailing separator
  /// @throws bad_alloc on memory allocation failure
  inline UnicodeString getGameBindingsDir(const UnicodeString& configDir) { return configDir + gameBindingsDirectory(); }
  /// @brief Build global config file path (from config directory)
  /// @throws bad_alloc on memory allocation failure
  UnicodeString getGameBindingPath(const UnicodeString& configDir, const char* gameId);
}
