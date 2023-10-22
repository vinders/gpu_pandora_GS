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
#include <display/controls/control_mesh.h>
#include <display/controls/text_mesh.h>
#include "menu/controls/types.h"

namespace menu {
  namespace controls {
    enum class FieldsetStyle : uint32_t { ///< Fieldset visual style
      classic = 0, ///< Contour border with a title bar
      title        ///< Title with underline decoration and vertical line
    };
    
    /// @brief UI fieldset grouping control
    class Fieldset final {
    public:
      /// @brief Create fieldset control
      Fieldset(RendererContext& context, const char32_t* label, FieldsetStyle style,
               const float color[4], int32_t x, int32_t labelY, uint32_t paddingX,
               uint32_t paddingY, uint32_t width, uint32_t contentHeight);

      Fieldset() = default;
      Fieldset(const Fieldset&) = delete;
      Fieldset(Fieldset&&) noexcept = default;
      Fieldset& operator=(const Fieldset&) = delete;
      Fieldset& operator=(Fieldset&&) noexcept = default;
      ~Fieldset() noexcept { release(); }

      inline void release() noexcept {
        controlMesh.release();
        labelMesh.release();
      }

      // -- accessors --

      inline int32_t x() const noexcept { return controlMesh.x(); }
      inline int32_t y() const noexcept { return controlMesh.y(); }
      inline uint32_t width() const noexcept { return controlMesh.width(); }
      inline uint32_t height() const noexcept { return controlMesh.height(); }

      // -- operations --

      void move(RendererContext& context, int32_t x, int32_t labelY, ///< Change control location (on window resize)
                uint32_t width, uint32_t contentHeight);

      /// @brief Draw fieldset decoration control
      /// @remarks - Use 'bindGraphicsPipeline' (for flat-shaded images) and 'bindVertexUniforms' (with color modifier) before call.
      ///          - It's recommended to draw all controls using the same pipeline/uniform before using the other draw calls.
      inline void drawBackground(RendererContext& context) { controlMesh.draw(*context.renderer); }
      /// @brief Draw fieldset text
      /// @remarks - Use 'bindGraphicsPipeline' (for control labels) and 'bindFragmentUniforms' (with label colors) before call.
      ///          - It's recommended to draw all labels using the same pipeline/uniform before using the other draw calls.
      inline void drawLabel(RendererContext& context) { labelMesh.draw(*context.renderer); }

    private:
      display::controls::ControlMesh controlMesh;
      display::controls::TextMesh labelMesh;
      FieldsetStyle style = FieldsetStyle::classic;
    };
  }
}
