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
#include <display/controls/icon_mesh.h>
#include <display/controls/text_mesh.h>
#include "menu/renderer_context.h"

namespace menu {
  namespace controls {
    /// @brief UI label control with optional icon
    class Label final {
    public:
      /// @brief Create label control
      Label(RendererContext& context, const char32_t* label, int32_t x, int32_t labelY,
            display::controls::TextAlignment align = display::controls::TextAlignment::left,
            display::ControlIconType icon = display::ControlIconType::none) {
        init(context, label, x, labelY, align, icon);
      }

      Label() = default;
      Label(const Label&) = delete;
      Label(Label&&) noexcept = default;
      Label& operator=(const Label&) = delete;
      Label& operator=(Label&&) noexcept = default;
      ~Label() noexcept { release(); }

      inline void release() noexcept {
        iconMesh.release();
        labelMesh.release();
      }

      // -- accessors --

      inline int32_t x() const noexcept { return iconMesh.width() ? iconMesh.x() : labelMesh.x(); }
      inline int32_t y() const noexcept { return iconMesh.width() ? iconMesh.y() : labelMesh.y(); }
      inline uint32_t width() const noexcept { return iconMesh.width() ? (iconMesh.width() + labelMesh.width() + labelMargin()) : labelMesh.width(); }
      inline uint32_t height() const noexcept { return iconMesh.width() ? iconMesh.height() : labelMesh.height(); }

      // -- operations --

      void move(RendererContext& context, int32_t x, int32_t labelY, ///< Change control location (on window resize)
                display::controls::TextAlignment align = display::controls::TextAlignment::left);

      // -- rendering --

      /// @brief Draw label icon (if any)
      /// @remarks - Use 'bindGraphicsPipeline' (for flat-shaded images) and 'bindFragmentUniforms' (with on/off info) before call.
      ///          - It's recommended to draw all icons using the same pipeline/uniform before using the other draw calls.
      inline void drawIcon(RendererContext& context) { iconMesh.draw(context.renderer()); }
      /// @brief Draw label text
      /// @remarks - Use 'bindGraphicsPipeline' (for control labels) and 'bindFragmentUniforms' (with label colors) before call.
      ///          - It's recommended to draw all labels using the same pipeline/uniform before using the other draw calls.
      inline void drawLabel(RendererContext& context) { labelMesh.draw(context.renderer()); }

    private:
      void init(RendererContext& context, const char32_t* label, int32_t x, int32_t labelY,
                display::controls::TextAlignment align, display::ControlIconType icon);
      static constexpr inline uint32_t labelMargin() noexcept { return 6u; }
      
    private:
      display::controls::IconMesh iconMesh;
      display::controls::TextMesh labelMesh;
    };
  }
}
