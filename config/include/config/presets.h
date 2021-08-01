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
  enum class PresetId : int {
    defaultConfig = 0,
    psxAccurate = 1,
    realistic2D = 2,
    realistic3D = 3,
    cartoon = 4
  };
# define __CONFIG_LAST_PRESET_ID PresetId::cartoon

  /// @brief Get menu label for a preset
  constexpr inline const __UNICODE_CHAR* toLabel(PresetId id) {
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

  void loadPreset(PresetId id, RendererProfile& outRendererCfg,
                  WindowProfile& outWindowCfg, EffectsProfile& outEffectsCfg) noexcept;
}
