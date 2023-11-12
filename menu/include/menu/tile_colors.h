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
  enum class TileColors : uint32_t { ///< Menu profile tile colors
    themeColor = 0,
    red,
    orange,
    yellow,
    apple,
    green,
    teal,
    cyan,
    blue,
    purple,
    violet,
    pink,
    gray,
    COUNT
  };
}
