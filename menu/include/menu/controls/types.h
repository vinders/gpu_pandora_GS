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
#include <array>
#include <display/video_api.h>
#include <display/geometry.h>
#include <display/image_loader.h>

namespace menu {
  namespace controls {
    enum class FontType : uint32_t {
      titles = 0,
      labels,
      inputText,
      COUNT
    };
    /// @brief Control rendering context
    struct RendererContext final {
      RendererContext() = default;
      RendererContext(const RendererContext&) = default;
      RendererContext(RendererContext&&) noexcept = default;
      RendererContext& operator=(const RendererContext&) = default;
      RendererContext& operator=(RendererContext&&) noexcept = default;
      ~RendererContext() noexcept = default;

      inline display::Font& getFont(FontType fontType) noexcept { return *fonts[(size_t)fontType]; }

      std::shared_ptr<video_api::Renderer> renderer = nullptr;
      std::array<std::unique_ptr<display::Font>, (size_t)FontType::COUNT> fonts{};
      display::ImageLoader imageLoader;
      float pixelSizeX = 1.f;
      float pixelSizeY = 1.f;
    };

    // ---

    enum class ControlStatus : uint32_t {
      none = 0, ///< Regular control
      selected, ///< Hover/selected
      pressed,  ///< Active/pressed
      disabled, ///< Unusable control
      COUNT
    };

    // ---

    /// @brief Visual style properties for a complex control
    struct ControlStyle final {
      ControlStyle(FontType fontType, display::ControlIconType icon, uint32_t paddingX = 0, uint32_t paddingY = 0)
        : fontType(fontType), icon(icon), paddingX(paddingX), paddingY(paddingY) {}
      ControlStyle(const float color_[4], FontType fontType, display::ControlIconType icon, uint32_t paddingX = 0, uint32_t paddingY = 0)
        : fontType(fontType), icon(icon), paddingX(paddingX), paddingY(paddingY) {
        this->color[0] = color_[0];
        this->color[1] = color_[1];
        this->color[2] = color_[2];
        this->color[3] = color_[3];
      }
      ControlStyle() = default;
      ControlStyle(const ControlStyle&) = default;
      ControlStyle(ControlStyle&&) noexcept = default;
      ControlStyle& operator=(const ControlStyle&) = default;
      ControlStyle& operator=(ControlStyle&&) noexcept = default;
      ~ControlStyle() noexcept = default;

      float color[4]{ 0.f,0.f,0.f,1.f };    ///< Background color type
      FontType fontType = FontType::titles; ///< Font type to use
      display::ControlIconType icon = display::ControlIconType::none; ///< Icon to display (if available)
      uint32_t paddingX = 0; ///< Left/right padding (between border and inner text/icon)
      uint32_t paddingY = 0; ///< Top/bottom padding (between border and inner text/icon)
    };
  }
}
