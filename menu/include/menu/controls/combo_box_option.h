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
#include <memory>

namespace menu {
  namespace controls {
    using ComboValue = uint32_t;

    /// @brief Option for combo-box creation
    struct ComboBoxOption final {
      ComboBoxOption(const char32_t* name, ComboValue value)
        : name(display::controls::TextMesh::toString(name)), value(value) {}
      ComboBoxOption() = default;
      ComboBoxOption(const ComboBoxOption&) = default;
      ComboBoxOption(ComboBoxOption&&) noexcept = default;
      ComboBoxOption& operator=(const ComboBoxOption&) = default;
      ComboBoxOption& operator=(ComboBoxOption&&) noexcept = default;
      ~ComboBoxOption() noexcept = default;

      std::unique_ptr<char32_t[]> name = nullptr;
      ComboValue value = 0;
    };
  }
}
