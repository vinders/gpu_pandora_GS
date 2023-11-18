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
#include <display/controls/text_mesh.h>
#include "menu/renderer_state_buffers.h"
#include "menu/controls/control.h"

namespace menu {
  namespace controls {
    /// @brief UI sliding ruler control
    class Ruler final : public Control {
    public:
      /// @brief Create sliding ruler control
      /// @param colors      [0]: ruler background / [1]: ruler border / [2]: thumb / [3]: left-side ruler fill
      /// @param operationId Unique ruler operation identifier (should be cast from an enum or constant)
      /// @param onChange    Event handler to call (with 'operationId') when the ruler value changes
      /// @param enabler     Optional data/config value to which the ruler state should be bound
      Ruler(RendererContext& context, const char16_t* label, const char16_t* suffix, menu::FontType labelFontType,
            display::controls::TextAlignment labelAlign, int32_t x, int32_t labelY, uint32_t minLabelWidth,
            uint32_t fixedRulerWidth, const RulerColors& colors, uint32_t operationId,
            std::function<void(uint32_t,uint32_t)> onChange, uint32_t minValue, uint32_t maxValue, uint32_t step,
            uint32_t& boundValue_, const bool* enabler = nullptr)
        : boundValue(&boundValue_),
          enabler(enabler),
          lastValue(boundValue_),
          minValue(minValue),
          maxValue(maxValue),
          step(step),
          minLabelWidth(minLabelWidth),
          onChange(std::move(onChange)),
          operationId(operationId) {
        init(context, label, suffix, labelFontType, labelAlign, x, labelY, fixedRulerWidth, colors);
      }

      Ruler() = default;
      Ruler(const Ruler&) = delete;
      Ruler(Ruler&&) noexcept = default;
      Ruler& operator=(const Ruler&) = delete;
      Ruler& operator=(Ruler&&) noexcept = default;
      ~Ruler() noexcept { release(); }

      inline void release() noexcept {
        controlMesh.release();
        fillerMesh.release();
        thumbMesh.release();
        labelMesh.release();
        suffixMesh.release();
      }
      ControlType type() const noexcept override;

      // -- accessors --

      inline int32_t x() const noexcept { return labelMesh.x(); }
      inline int32_t y() const noexcept { return thumbMesh.y() + 1; }
      inline int32_t controlX() const noexcept { return controlMesh.x(); }
      inline int32_t rightX() const noexcept {
        return suffixMesh.width() ? (suffixMesh.x() + (int32_t)suffixMesh.width())
                                  : (controlMesh.x() + (int32_t)controlMesh.width());
      }

      inline uint32_t width() const noexcept { return static_cast<uint32_t>(rightX() - x()); }
      inline uint32_t height() const noexcept { return thumbMesh.height(); }

      inline bool isEnabled() const noexcept { return (enabler == nullptr || *enabler); } ///< Verify if control is enabled
      inline bool isDragged() const noexcept { return isDragging; } ///< Verify if thumb is currently being dragged
      inline bool isHover(int32_t mouseX, int32_t mouseY) const noexcept { ///< Verify mouse hover
        return (mouseX >= x() && mouseY >= y() && mouseX < x() + (int32_t)width() && mouseY < y() + (int32_t)height());
      }
      /// @brief Get control status, based on mouse location (hover, disabled...)
      ControlStatus getStatus(int32_t mouseX, int32_t mouseY) const noexcept override;

      // -- operations --

      /// @brief Report click to the control (on mouse click with hover)
      /// @returns True if the control is dragged (always true if control is enabled)
      bool click(RendererContext& context, int32_t mouseX, int32_t) override;
      inline void click(RendererContext& context, int32_t mouseX, bool isMouseDown) { ///< Report click to control (on mouse click with hover)
        if (isEnabled()) {
          isDragging = isMouseDown;
          mouseMove(context, mouseX, 0);
        }
      }
      /// @brief Report mouse move to control (on mouse move with mouse down during drag)
      void mouseMove(RendererContext& context, int32_t mouseX, int32_t mouseY) override;
      bool mouseUp(RendererContext& context, int32_t mouseX) override; ///< Report end of mouse click (after drag)
      void selectPrevious(RendererContext& context);       ///< Select previous entry if available (on keyboard/pad action)
      void selectNext(RendererContext& context);           ///< Select next entry if available (on keyboard/pad action)
      void close() override;

      void setSelectedIndex(RendererContext& context, uint32_t value, bool notify = true); ///< Force selection of specific value if available

      /// @brief Change control location (on window resize)
      void move(RendererContext& context, int32_t x, int32_t labelY, display::controls::TextAlignment labelAlign);

      // -- rendering --

      /// @brief Draw ruler background + thumb
      /// @remarks - Use 'bindGraphicsPipeline' (for control backgrounds) before call.
      ///          - It's recommended to draw all controls using the same pipeline/uniform before using the other draw calls.
      void drawBackground(RendererContext& context, RendererStateBuffers& buffers);
      /// @brief Draw ruler label + suffix
      /// @remarks - Use 'bindGraphicsPipeline' (for control labels) before call.
      ///          - It's recommended to draw all labels using the same pipeline/uniform before using the other draw calls.
      void drawLabels(RendererContext& context, RendererStateBuffers& buffers, bool isActive);

    private:
      void init(RendererContext& context, const char16_t* label, const char16_t* suffix, menu::FontType labelFontType,
                display::controls::TextAlignment labelAlign, int32_t x, int32_t labelY,
                uint32_t fixedRulerWidth, const RulerColors& colors);
      void updateThumbPosition(RendererContext& context, uint32_t value, bool notify = true);

    private:
      display::controls::ControlMesh controlMesh;
      display::controls::ControlMesh fillerMesh;
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
      uint32_t firstStepOffset = 0;
      uint32_t stepWidth = 1;

      std::function<void(uint32_t,uint32_t)> onChange;
      uint32_t operationId = 0;
      bool isDragging = false;
    };
  }
}
