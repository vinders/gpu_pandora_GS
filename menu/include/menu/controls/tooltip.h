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
#include <functional>
#include <display/controls/control_mesh.h>
#include <display/controls/icon_mesh.h>
#include <display/controls/text_mesh.h>
#include "menu/renderer_state_buffers.h"
#include "menu/controls/control.h"

namespace menu {
  namespace controls {
    /// @brief UI tooltip control with optional icon
    class Tooltip final {
    public:
      /// @brief Create tooltip control
      Tooltip(RendererContext& context, const char32_t* label, FontType fontType, LabelBufferType textColor,
              int32_t x, int32_t y, uint32_t width, uint32_t height, uint32_t paddingX,
              const float backgroundColor[4], display::ControlIconType icon = display::ControlIconType::none)
        : fontType(fontType),
          textColor(textColor),
          paddingX(paddingX) {
        init(context, label, x, y, width, height, backgroundColor, icon);
      }

      Tooltip() = default;
      Tooltip(const Tooltip&) = delete;
      Tooltip(Tooltip&&) noexcept = default;
      Tooltip& operator=(const Tooltip&) = delete;
      Tooltip& operator=(Tooltip&&) noexcept = default;
      ~Tooltip() noexcept { release(); }

      inline void release() noexcept {
        controlMesh.release();
        iconMesh.release();
        labelMesh.release();
      }

      // -- accessors --

      inline int32_t x() const noexcept { return controlMesh.x(); }
      inline int32_t y() const noexcept { return controlMesh.y(); }
      inline uint32_t width() const noexcept { return controlMesh.width(); }
      inline uint32_t height() const noexcept { return controlMesh.height(); }

      // -- operations --

      void move(RendererContext& context, int32_t x, int32_t y, uint32_t width); ///< Change control location (on window resize)
      
      void updateIcon(RendererContext& context, display::ControlIconType icon); ///< Replace tooltip icon
      void updateLabel(RendererContext& context, const char32_t* label, LabelBufferType textColor); ///< Replace tooltip text

      // -- rendering --

      /// @brief Draw tooltip background
      /// @remarks - Use 'bindGraphicsPipeline' (for control backgrounds) before call.
      ///          - It's recommended to draw all controls using the same pipeline/uniform before using the other draw calls.
      void drawBackground(RendererContext& context, RendererStateBuffers& buffers);
      /// @brief Draw tooltip icon (if any)
      /// @remarks - Use 'bindGraphicsPipeline' (for flat-shaded images) before call.
      ///          - It's recommended to draw all labels using the same pipeline/uniform before using the other draw calls.
      void drawIcon(RendererContext& context, RendererStateBuffers& buffers);
      /// @brief Draw tooltip label (if any)
      /// @remarks - Use 'bindGraphicsPipeline' (for control labels) before call.
      ///          - It's recommended to draw all labels using the same pipeline/uniform before using the other draw calls.
      void drawLabel(RendererContext& context, RendererStateBuffers& buffers);

    private:
      void init(RendererContext& context, const char32_t* label, int32_t x, int32_t y, uint32_t width, uint32_t height,
                const float backgroundColor[4], display::ControlIconType icon);
      static constexpr inline uint32_t iconMarginRight() noexcept { return 4u; }

    private:
      display::controls::ControlMesh controlMesh;
      display::controls::IconMesh iconMesh;
      display::controls::TextMesh labelMesh;
      FontType fontType = FontType::inputText;
      LabelBufferType textColor = LabelBufferType::regular;
      
      uint32_t paddingX = 0;
    };
  }
}
