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
#include <display/image_loader.h>
#include <display/controls/text_mesh.h>

namespace menu {
  namespace controls {
    /// @brief Option for vertical tabs creation
    class VerticalTabOption final {
    public:
      static constexpr const size_t maxLength{ (size_t)31 }; ///< Max option name length

      VerticalTabOption(const char16_t* label, display::TabIconType icon) noexcept
        : icon_(icon) {
        updateName(label);
      }
      VerticalTabOption() noexcept { *name_ = u'\0'; }
      VerticalTabOption(const VerticalTabOption&) = default;
      VerticalTabOption(VerticalTabOption&&) noexcept = default;
      VerticalTabOption& operator=(const VerticalTabOption&) = default;
      VerticalTabOption& operator=(VerticalTabOption&&) noexcept = default;
      ~VerticalTabOption() noexcept = default;

      inline const char16_t* name() const noexcept { return name_; } ///< Option label to display
      inline display::TabIconType icon() const noexcept { return icon_; } ///< Icon associated with this tab

      inline void updateName(const char16_t* label) noexcept { ///< Set option label
        size_t length = display::controls::TextMesh::getStringLength(label);
        if (length) {
          if (length > maxLength)
            length = maxLength;
          memcpy(name_, label, length*sizeof(char16_t));
        }
        name_[length] = u'\0';
      }

    private:
      char16_t name_[maxLength+1];
      display::TabIconType icon_ = display::TabIconType::none;
    };
  }
}
