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
#include <display/controls/text_mesh.h>

namespace menu {
  namespace controls {
    using ComboValue = uint32_t;

    /// @brief Option for combo-box creation
    class ComboBoxOption final {
    public:
      static constexpr const size_t maxLength{ (size_t)35 }; ///< Max option name length

      ComboBoxOption(const char16_t* label, ComboValue value) noexcept
        : value_(value) {
        updateName(label);
      }
      explicit ComboBoxOption(const wchar_t* label, ComboValue value) noexcept
        : value_(value) {
        updateName(label);
      }
      ComboBoxOption() noexcept { *name_ = u'\0'; }
      ComboBoxOption(const ComboBoxOption&) = default;
      ComboBoxOption(ComboBoxOption&&) noexcept = default;
      ComboBoxOption& operator=(const ComboBoxOption&) = default;
      ComboBoxOption& operator=(ComboBoxOption&&) noexcept = default;
      ~ComboBoxOption() noexcept = default;

      inline const char16_t* name() const noexcept { return name_; } ///< Option label to display
      inline ComboValue value() const noexcept { return value_; }    ///< Value associated with this option

      inline void updateName(const char16_t* label) noexcept { ///< Set option label
        size_t length = display::controls::TextMesh::getStringLength(label);
        if (length) {
          if (length > maxLength)
            length = maxLength;
          memcpy(name_, label, length*sizeof(char16_t));
        }
        name_[length] = u'\0';
      }
      inline void updateName(const wchar_t* label) noexcept {
        char16_t* destIt = name_;
        const char16_t* endIt = name_ + (intptr_t)maxLength;
        if (label != nullptr) {
          for (const wchar_t* srcIt = label; *srcIt && destIt < endIt; ++srcIt, ++destIt) {
            if (*srcIt <= (wchar_t)0xFFFF)
              *destIt = (char16_t)*srcIt;
          }
        }
        *destIt = u'\0';
      }

    private:
      char16_t name_[maxLength+1];
      ComboValue value_ = 0;
    };
  }
}
