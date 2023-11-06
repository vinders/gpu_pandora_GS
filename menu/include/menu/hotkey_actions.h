/*******************************************************************************
Pandora GS - PSEmu-compatible GPU driver
Copyright (C) 2023  Romain Vinders

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation, version 2 of the License.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details (LICENSE file).
*******************************************************************************/
#pragma once

#include <cstdint>

namespace menu {
  enum class HotkeyActions : uint32_t { ///< Actions that can be bound to a keyboard/controller key
    showOptions = 0,
    previousProfile,
    nextProfile,

    nextSaveSlot,
    quickSave,
    quickLoad,

    pauseResume,
    slowMotion,
    fastForward,
    resetEmulator,
    exitToEmulator,

    screenshot,
    toggleWindowMode,
    toggleOsd,
    COUNT
  };
}
