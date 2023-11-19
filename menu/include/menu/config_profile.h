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
#include <cstring>
#include <memory>
#include <display/controls/text_mesh.h>
#include "menu/tile_colors.h"

namespace menu {
  struct ConfigProfile final {
    ConfigProfile(uint32_t id, const char16_t* nameValue)
      : id(id) {
      size_t length = display::controls::TextMesh::getStringLength(nameValue);
      name.reset(new char16_t[length + 1]);
      memcpy(name.get(), nameValue, (length+1)*sizeof(char16_t));
    }
    ConfigProfile(uint32_t id, std::unique_ptr<char16_t[]>&& name)
      : id(id), name(std::move(name)) {}
    ConfigProfile(ConfigProfile&&) noexcept = default;
    ConfigProfile& operator=(ConfigProfile&&) noexcept = default;
    ~ConfigProfile() noexcept = default;

    uint32_t id;
    std::unique_ptr<char16_t[]> name;
    TileColors color = TileColors::themeColor;
  };
}
