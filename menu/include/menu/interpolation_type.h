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
  enum class InterpolationType : uint32_t { ///< Minification/magnification interpolation types
    nearest = 0, ///< Nearest pixel, no filtering
    bilinear,    ///< Smooth, diamond artifacts, fast
    gaussian,    ///< Smoothest, blurry, slower
    bessel,      ///< Smooth/rounder, slower
    mitchell,    ///< Smooth/rounder, less blurry, slower
    lanczos,     ///< Sharper/rounder, keeps color vibrance, ring artifacts, slower
    COUNT
  };
}
