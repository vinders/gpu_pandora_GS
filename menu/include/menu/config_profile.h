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

#define DEFAULT_NAME_BUFFER_SIZE 20

namespace menu {
  static inline void setDefaultProfileName(uint32_t profileId, char16_t buffer[DEFAULT_NAME_BUFFER_SIZE]) {
    const char16_t defaultPrefix[] = u"Profile #";
    memcpy(buffer, defaultPrefix, sizeof(defaultPrefix));

    char16_t* it = buffer + (static_cast<intptr_t>(sizeof(defaultPrefix)/sizeof(char16_t)) - 1u); // append profile ID
    if (profileId >= 10u) {
      it += (profileId < 10000u)
          ? ((profileId < 100u) ? 2 : ((profileId < 1000u) ? 3 : 4) )
          : ((profileId < 1000000u)
             ? ((profileId < 100000u) ? 5 : 6)
             : ((profileId < 100000000u)
                ? ((profileId < 10000000u) ? 7 : 8)
                : ((profileId < 1000000000u) ? 9 : 10)));
      *it = u'\0';
      --it;
      while (profileId) {
        *it = u'0' + static_cast<char16_t>(profileId % 10u);
        profileId /= 10u;
        --it;
      }
    }
    else {
      *it = u'0' + static_cast<char16_t>(profileId);
      *(++it) = u'\0';
    }
  }

  // ---

  struct ConfigProfile final {
    ConfigProfile(uint32_t id, const char16_t* nameValue, TileColors color = TileColors::themeColor)
      : id(id), color(color) {
      if (nameValue != nullptr) {
        size_t length = display::controls::TextMesh::getStringLength(nameValue);
        name.reset(new char16_t[length + 1]);
        memcpy(name.get(), nameValue, (length+1)*sizeof(char16_t));
      }
      else {
        name.reset(new char16_t[DEFAULT_NAME_BUFFER_SIZE]);
        setDefaultProfileName(id, name.get());
      }
    }
    ConfigProfile(uint32_t id, std::unique_ptr<char16_t[]>&& name, TileColors color = TileColors::themeColor)
      : id(id), name(std::move(name)), color(color) {}
    ConfigProfile(ConfigProfile&&) noexcept = default;
    ConfigProfile& operator=(ConfigProfile&&) noexcept = default;
    ~ConfigProfile() noexcept = default;

    uint32_t id;
    std::unique_ptr<char16_t[]> name;
    TileColors color = TileColors::themeColor;
  };
}
