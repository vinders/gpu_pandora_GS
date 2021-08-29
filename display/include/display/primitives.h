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
#pragma once

#include <cstdint>

namespace display {
  class StatusRegister;
  class Renderer;

  /// @brief Clear pending command data buffer
  void clearCommandBuffer() noexcept;

  /// @brief Run GP0 rendering command (drawing & rendering attributes)
  /// @returns Size used by current command
  int runGp0Command(StatusRegister& status, Renderer& renderer,
                    uint32_t* mem, int size, bool isFrameSkipped) noexcept;
}