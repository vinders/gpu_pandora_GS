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

#ifdef _WINDOWS
# define __PATH_CHAR     wchar_t
# define __PATH_STR(str) L"" str
#else
# define __PATH_CHAR     char
# define __PATH_STR(str) str
#endif

namespace config {
  /// @brief PlayStation emulator type
  enum class EmulatorType : int {
    unknown    = 0,
    epsxe      = 1, ///< Standard emulator (ePSXe)
    pcsxr      = 2, ///< Standard emulator (PCSX-R / PCSX-PGXP)
    zinc       = 3, ///< Arcade machine emulator (ZiNc) -> adapt VRAM access + system status
    pluginTest = 4  ///< psxtest_gpu -> use accurate config (no effects or upscaling)
  };
  
  // ---
  
  /// @brief Read emulator process name
  /// @remarks bufferLength should be the max path length of the system
  /// @returns Pointer to file name in buffer (buffer contains whole path) on success, or NULL on failure
  __PATH_CHAR* getProcessName(__PATH_CHAR* buffer, size_t bufferLength, size_t& outUsedBufferLength) noexcept;
  
  /// @brief Detect emulator type, based on process name
  /// @remarks Used for special tweaks + call appropriate F-keys on menu action events
  EmulatorType getEmulatorType() noexcept;
}
