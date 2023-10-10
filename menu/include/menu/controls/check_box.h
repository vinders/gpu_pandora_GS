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
#include "menu/controls/types.h"

namespace menu {
  namespace controls {
    /// @brief UI check-box control
    class CheckBox final {
    public:
      /// @brief Create check-box control
      /// @param boundValue  Data/config value to bind to the check-box value (get/set)
      /// @param enabler     Optional data/config value to which the button state should be bound
      CheckBox(RendererContext& context, const char32_t* label, int32_t x, int32_t labelY,
               bool isLabelBeforeBox, uint32_t minLabelWidth, bool& boundValue, const bool* enabler = nullptr)
        : boundValue(&boundValue),
          enabler(enabler),
          minLabelWidth(minLabelWidth),
          isLabelBeforeBox(isLabelBeforeBox) {
        init(context, label, x, labelY);
      }

      CheckBox() = default;
      CheckBox(const CheckBox&) = delete;
      CheckBox(CheckBox&&) noexcept = default;
      CheckBox& operator=(const CheckBox&) = delete;
      CheckBox& operator=(CheckBox&&) noexcept = default;
      ~CheckBox() noexcept = default;

      // -- accessors --

      inline int32_t x() const noexcept { return isLabelBeforeBox ? labelMesh.x() : checkedMesh.x(); }
      inline int32_t y() const noexcept { return checkedMesh.y(); }
      inline uint32_t width() const noexcept { return checkedMesh.width() + labelMesh.width() + labelMargin(); }
      inline uint32_t height() const noexcept { return checkedMesh.height(); }

      inline bool isEnabled() const noexcept { return (enabler == nullptr || *enabler); } ///< Verify if control is enabled
      inline bool isHover(int32_t mouseX, int32_t mouseY) const noexcept { ///< Verify mouse hover
        const int32_t coordX = x();
        return (mouseY >= y() && mouseX >= coordX && mouseY < y() + (int32_t)height() && mouseX < coordX + (int32_t)width());
      }

      // -- operations --

      /// @brief Report click to control (on mouse click with hover / on keyboard/pad action)
      inline void click() const {
        if (isEnabled())
          *boundValue ^= true;
      }
      void move(RendererContext& context, int32_t x, int32_t labelY); ///< Change control location (on window resize)

      /// @brief Draw check-box icon
      /// @remarks - Use 'bindGraphicsPipeline' (for flat-shaded images) and 'bindFragmentUniforms' (with on/off info) before call.
      ///          - It's recommended to draw all labels using the same pipeline/uniform before using the other draw calls.
      inline void drawIcon(RendererContext& context) {
        if (*boundValue)
          checkedMesh.draw(*context.renderer);
        else
          uncheckedMesh.draw(*context.renderer);
      }
      /// @brief Draw label next to check-box
      /// @remarks - Use 'bindGraphicsPipeline' (for control labels) and 'bindFragmentUniforms' (with label colors) before call.
      ///          - It's recommended to draw all labels using the same pipeline/uniform before using the other draw calls.
      inline void drawLabel(RendererContext& context) { labelMesh.draw(*context.renderer); }

    private:
      void init(RendererContext& context, const char32_t* label, int32_t x, int32_t labelY);
      static constexpr inline uint32_t labelMargin() noexcept { return 6u; }
      inline int32_t getBoxX(int32_t x, uint32_t labelWidth) const noexcept {
        if (isLabelBeforeBox) {
          if (minLabelWidth >= labelWidth)
            labelWidth = minLabelWidth;
          if (labelWidth)
            return (x + (int32_t)labelWidth + (int32_t)labelMargin());
        }
        return x;
      }
      
    private:
      display::controls::IconMesh checkedMesh;
      display::controls::IconMesh uncheckedMesh;
      display::controls::TextMesh labelMesh;
      bool* boundValue = nullptr;
      const bool* enabler = nullptr;

      uint32_t minLabelWidth = 0;
      bool isLabelBeforeBox = false;
    };
  }
}
