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
    /// @brief UI sliding ruler control
    class Ruler final {
    public:
      /// @brief Create sliding ruler control
      /// @param operationId Unique ruler operation identifier (should be cast from an enum or constant)
      /// @param onClick     Event handler to call (with 'operationId') when the ruler is clicked
      /// @param enabler     Optional data/config value to which the ruler state should be bound
      Ruler(RendererContext& context, const char32_t* label, const char* suffix,
            display::controls::TextAlignment labelAlign, int32_t x, int32_t labelY, const ControlStyle& style,
            uint32_t fixedRulerWidth, float borderColor[4], float thumbColor[4],
            uint32_t minValue, uint32_t maxValue, uint32_t step, uint32_t& boundValue, const bool* enabler = nullptr)
        : boundValue(&boundValue),
          enabler(enabler),
          lastValue(boundValue),
          minValue(minValue),
          maxValue(maxValue),
          step(step),
          minLabelWidth(style.minLabelWidth),
          paddingX(style.paddingX),
          paddingY(style.paddingY) {
        init(context, label, suffix, labelAlign, x, labelY, style, fixedRulerWidth, borderColor, thumbColor);
      }

      Ruler() = default;
      Ruler(const Ruler&) = delete;
      Ruler(Ruler&&) noexcept = default;
      Ruler& operator=(const Ruler&) = delete;
      Ruler& operator=(Ruler&&) noexcept = default;
      ~Ruler() noexcept { release(); }

      inline void release() noexcept {
        controlMesh.release();
        thumbMesh.release();
        labelMesh.release();
        suffixMesh.release();
      }

      // -- accessors --

      inline int32_t x() const noexcept { return controlMesh.x(); }
      inline int32_t y() const noexcept { return thumbMesh.y(); }
      inline int32_t middleY() const noexcept { return labelMesh.y() + (int32_t)(labelMesh.height() >> 1); }
      inline uint32_t width() const noexcept { return controlMesh.width(); }
      inline uint32_t height() const noexcept { return thumbMesh.height(); }

      inline bool isEnabled() const noexcept { return (enabler == nullptr || *enabler); } ///< Verify if control is enabled
      inline bool isDragged() const noexcept { return isDragging; } ///< Verify if thumb is currently being dragged
      inline bool isHover(int32_t mouseX, int32_t mouseY) const noexcept { ///< Verify mouse hover
        return (mouseX >= x() && mouseY >= y() && mouseX < x() + (int32_t)width() && mouseY < y() + (int32_t)height());
      }

      // -- operations --

      /// @brief Report click to control (on mouse click with hover / on keyboard/pad action)
      inline void click(RendererContext& context, int32_t mouseX, bool isMouseDown) { ///< Report click to control (on mouse click with hover)
        if (isEnabled()) {
          isDragging = isMouseDown;
          mouseMove(context, mouseX);
        }
      }
      void mouseMove(RendererContext& context, int32_t mouseX);       ///< Report mouse move to control (on mouse move with mouse down during drag)
      inline void mouseUp(RendererContext& context, int32_t mouseX) { ///< Report end of mouse click (after drag)
        mouseMove(context, mouseX);
        isDragging = false;
      }
      void selectPrevious(RendererContext& context);       ///< Select previous entry if available (on keyboard/pad action)
      void selectNext(RendererContext& context);           ///< Select next entry if available (on keyboard/pad action)

      void move(RendererContext& context, int32_t x, int32_t labelY, display::controls::TextAlignment labelAlign); ///< Change control location (on window resize)

      /// @brief Draw ruler background + thumb
      /// @remarks - Use 'bindGraphicsPipeline' (for control backgrounds) and 'bindVertexUniforms' (with color modifier) before call.
      ///          - It's recommended to draw all controls using the same pipeline/uniform before using the other draw calls.
      void drawBackground(RendererContext& context);
      /// @brief Draw ruler label + suffix
      /// @remarks - Use 'bindGraphicsPipeline' (for control labels) and 'bindFragmentUniforms' (with label colors) before call.
      ///          - It's recommended to draw all labels using the same pipeline/uniform before using the other draw calls.
      inline void drawLabels(RendererContext& context) {
        labelMesh.draw(*context.renderer);
        suffixMesh.draw(*context.renderer);
      }

    private:
      void init(RendererContext& context, const char32_t* label, const char* suffix,
                display::controls::TextAlignment labelAlign, int32_t x, int32_t labelY, const ControlStyle& style,
                uint32_t fixedRulerWidth, const float borderColor[4], const float thumbColor[4]);
      static constexpr inline uint32_t labelMargin() noexcept { return 6u; }

      void updateThumbPosition(RendererContext& context, uint32_t value);

    private:
      display::controls::ControlMesh controlMesh;
      display::controls::ControlMesh thumbMesh;
      display::controls::TextMesh labelMesh;
      display::controls::TextMesh suffixMesh;
      uint32_t* boundValue = nullptr;
      const bool* enabler = nullptr;
      uint32_t lastValue = 0;

      uint32_t minValue = 0;
      uint32_t maxValue = 0;
      uint32_t step = 1;
      uint32_t minLabelWidth = 0;
      uint32_t paddingX = 0;
      uint32_t paddingY = 0;
      uint32_t firstStepOffset = 0;
      uint32_t stepWidth = 1;
      bool isDragging = false;
    };
  }
}
