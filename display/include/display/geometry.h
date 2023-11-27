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
#include <vector>

namespace display {
  // -- geometry helpers --
  
  static constexpr inline float ToPixelSize(uint32_t frameSize) noexcept {
    return 2.0f/(float)frameSize;
  }
  static constexpr inline float ToVertexPositionX(int32_t x, const float pixelSizeX) noexcept {
    return (float)x*pixelSizeX - 1.0f;
  }
  static constexpr inline float ToVertexPositionY(int32_t y, const float pixelSizeY) noexcept {
    return 1.0f - (float)y*pixelSizeY;
  }
  static constexpr inline float ToTextureCoord(uint32_t px, uint32_t textureSize) noexcept {
    return (float)px/(float)textureSize;
  }
#   ifdef _WINDOWS
    static constexpr inline float DepthUnit() noexcept { return 1.0f; } // D3D11 -> left-handed (positive depth)
#   else
    static constexpr inline float DepthUnit() noexcept { return -1.0f; } // Vulkan -> right-handed (negative depth)
#   endif
}
