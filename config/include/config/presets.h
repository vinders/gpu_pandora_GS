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

#include "config/config.h"

namespace config {
  /// @brief Config profile preset ID
  /// @remarks Can be used as ProfileId -> set MSB to 1, to avoid conflicts
  enum class PresetId : uint32_t {
    defaultConfig = 0x80000000,
    psxAccurate   = 0x80000001,
    realistic2D   = 0x80000002,
    realistic3D   = 0x80000003,
    cartoon       = 0x80000004
  };
# define __CONFIG_PRESET_FLAG   0x80000000
# define __CONFIG_MAX_PRESET_ID PresetId::cartoon

  /// @brief Get menu label for a preset
  constexpr inline const __UNICODE_CHAR* toLabel(PresetId id) noexcept {
    switch (id) {
      case PresetId::defaultConfig: return __UNICODE_STR("Default");
      case PresetId::psxAccurate:   return __UNICODE_STR("Accurate (PSX)");
      case PresetId::realistic2D:   return __UNICODE_STR("Realistic 2D");
      case PresetId::realistic3D:   return __UNICODE_STR("Realistic 3D");
      case PresetId::cartoon:       return __UNICODE_STR("Cartoon");
      default: return __UNICODE_STR("");
    }
  }

  // ---

  /// @brief Verify if a "profile ID" (ex: stored in game association) is actually a preset ID
  constexpr inline bool isPresetId(ProfileId targetId) noexcept { return (targetId & __CONFIG_PRESET_FLAG); }

  /// @brief Load config preset values (associated with a preset ID)
  void loadPreset(PresetId id, RendererProfile& outRendererCfg) noexcept;
}
