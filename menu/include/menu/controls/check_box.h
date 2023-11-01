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
#include <display/controls/icon_mesh.h>
#include <display/controls/text_mesh.h>
#include "menu/renderer_state_buffers.h"
#include "menu/controls/control.h"

namespace menu {
  namespace controls {
    /// @brief UI check-box control
    class CheckBox final : public Control {
    public:
      /// @brief Create check-box control
      /// @param onChange    Event handler to call (with 'operationId' and value) when the check-box value changes
      /// @param boundValue  Data/config value to bind to the check-box value (get/set)
      /// @param enabler     Optional data/config value to which the button state should be bound
      CheckBox(RendererContext& context, const char32_t* label, int32_t x, int32_t labelY,
               uint32_t minLabelWidth, uint32_t operationId, std::function<void(uint32_t,uint32_t)> onChange,
               bool& boundValue, const bool* enabler = nullptr)
        : boundValue(&boundValue),
          enabler(enabler),
          onChange(std::move(onChange)),
          operationId(operationId),
          minLabelWidth(minLabelWidth) {
        init(context, label, x, labelY);
      }

      CheckBox() = default;
      CheckBox(const CheckBox&) = delete;
      CheckBox(CheckBox&&) noexcept = default;
      CheckBox& operator=(const CheckBox&) = delete;
      CheckBox& operator=(CheckBox&&) noexcept = default;
      ~CheckBox() noexcept { release(); }

      inline void release() noexcept {
        checkedMesh.release();
        uncheckedMesh.release();
        labelMesh.release();
      }
      ControlType Type() const noexcept override;

      // -- accessors --

      inline int32_t x() const noexcept { return labelMesh.x(); }
      inline int32_t y() const noexcept { return checkedMesh.y() + 1; }
      inline uint32_t width() const noexcept {
        const uint32_t labelWidth = ((labelMesh.width() >= minLabelWidth) ? labelMesh.width() : minLabelWidth);
        return labelWidth ? (checkedMesh.width() + labelWidth + labelMargin()) : checkedMesh.width();
      }
      inline uint32_t height() const noexcept { return checkedMesh.height(); }

      inline bool isEnabled() const noexcept { return (enabler == nullptr || *enabler); } ///< Verify if control is enabled
      inline bool isHover(int32_t mouseX, int32_t mouseY) const noexcept { ///< Verify mouse hover
        const int32_t coordX = x();
        return (mouseY >= y() && mouseX >= coordX && mouseY < y() + (int32_t)height() && mouseX < coordX + (int32_t)width());
      }
      /// @brief Get control status, based on mouse location (hover, disabled...)
      ControlStatus getStatus(int32_t mouseX, int32_t mouseY) const noexcept override;

      inline bool isChecked() const noexcept { return *boundValue; } ///< Get checkbox value

      // -- operations --

      /// @brief Report click to the control (on mouse click with hover -or- on keyboard/pad action)
      /// @returns True if the control is now open (always false)
      bool click(RendererContext& context, int32_t mouseX) override;
      inline void click() const {
        if (isEnabled()) {
          *boundValue ^= true;
          if (onChange != nullptr)
            onChange(operationId, (uint32_t)*boundValue);
        }
      }
      void move(RendererContext& context, int32_t x, int32_t labelY); ///< Change control location (on window resize)
      void updateLabel(RendererContext& context, const char32_t* label); ///< Change control label

      // -- rendering --

      /// @brief Draw check-box icon
      /// @remarks - Use 'bindGraphicsPipeline' (for flat-shaded images) before call.
      ///          - It's recommended to draw all labels using the same pipeline/uniform before using the other draw calls.
      void drawIcon(RendererContext& context, RendererStateBuffers& buffers, bool isActive);
      /// @brief Draw label next to check-box
      /// @remarks - Use 'bindGraphicsPipeline' (for control labels) before call.
      ///          - It's recommended to draw all labels using the same pipeline/uniform before using the other draw calls.
      void drawLabel(RendererContext& context, RendererStateBuffers& buffers, bool isActive);

    private:
      void init(RendererContext& context, const char32_t* label, int32_t x, int32_t labelY);
      
    private:
      display::controls::IconMesh checkedMesh;
      display::controls::IconMesh uncheckedMesh;
      display::controls::TextMesh labelMesh;
      bool* boundValue = nullptr;
      const bool* enabler = nullptr;

      std::function<void(uint32_t, bool)> onChange;
      uint32_t operationId = 0;
      uint32_t minLabelWidth = 0;
    };
  }
}
