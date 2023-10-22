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
#include <vector>
#include <memory>
#include <functional>
#include <display/controls/control_mesh.h>
#include <display/controls/text_mesh.h>
#include "menu/controls/types.h"
#include "menu/controls/combo_box_option.h"

namespace menu {
  namespace controls {
    /// @brief UI slider-box control
    class Slider final {
    public:
      /// @brief Create slider-box control
      /// @param operationId  Unique slider-box identifier (should be cast from an enum or constant)
      /// @param onChange     Event handler to call (with 'operationId' and value) when the slider-box value changes
      /// @param enabler      Optional data/config value to which the combo-box state should be bound
      Slider(RendererContext& context, const char32_t* label, int32_t x, int32_t labelY, const ControlStyle& style,
             uint32_t fixedSliderWidth, uint32_t operationId, std::function<void(uint32_t,ComboValue)> onChange,
             ComboBoxOption* values, size_t valueCount, int32_t selectedIndex = -1, const bool* enabler = nullptr)
        : selectedIndex((selectedIndex < (int32_t)valueCount) ? selectedIndex : -1),
          enabler(enabler),
          onChange(std::move(onChange)),
          operationId(operationId),
          minLabelWidth(style.minLabelWidth),
          fixedSliderWidth(fixedSliderWidth),
          paddingY(style.paddingY) {
        init(context, label, x, labelY, style.color, values, valueCount);
      }

      Slider() = default;
      Slider(const Slider&) = delete;
      Slider(Slider&&) noexcept = default;
      Slider& operator=(const Slider&) = delete;
      Slider& operator=(Slider&&) noexcept = default;
      ~Slider() noexcept { release(); }

      inline void release() noexcept {
        arrowLeftMesh.release();
        arrowRightMesh.release();
        labelMesh.release();
        selectableValues.clear();
      }

      // -- accessors --

      inline int32_t x() const noexcept { return arrowLeftMesh.x(); }
      inline int32_t y() const noexcept { return arrowLeftMesh.y(); }
      inline int32_t middleY() const noexcept { return arrowLeftMesh.y() + (int32_t)(arrowLeftMesh.height() >> 1); }
      inline uint32_t width() const noexcept { return fixedSliderWidth + (arrowLeftMesh.width() << 1); }
      inline uint32_t height() const noexcept { return arrowLeftMesh.height(); }

      inline bool isEnabled() const noexcept { return (enabler == nullptr || *enabler); } ///< Verify if control is enabled
      inline bool isHover(int32_t mouseX, int32_t mouseY) const noexcept { ///< Verify mouse hover
        return (mouseY >= y() && mouseX >= x() && mouseY < y() + (int32_t)height() && mouseX < x() + (int32_t)width());
      }

      inline const ComboValue* getSelectedValue() const noexcept { ///< Get value at selected index (if any)
        return (selectedIndex != -1) ? &(selectableValues[selectedIndex].value) : nullptr;
      }

      // -- operations --

      void click(int32_t mouseX); ///< Report click to control (on mouse click with hover)
      void selectPrevious();      ///< Select previous entry if available (on keyboard/pad action)
      void selectNext();          ///< Select next entry if available (on keyboard/pad action)

      void move(RendererContext& context, int32_t x, int32_t labelY); ///< Change control location (on window resize)

      /// @brief Draw slider background
      /// @remarks - Use 'bindGraphicsPipeline' (for control backgrounds) and 'bindVertexUniforms' (with color modifier) before call.
      ///          - It's recommended to draw all controls using the same pipeline/uniform before using the other draw calls.
      /// @returns True if 'hoverPressedVertexUniform' has been bound (if mouse hover on special part)
      bool drawBackground(RendererContext& context, int32_t mouseX, int32_t mouseY,
                          video_api::Buffer<video_api::ResourceUsage::staticGpu>& regularVertexUniform,
                          video_api::Buffer<video_api::ResourceUsage::staticGpu>& hoverPressedVertexUniform,
                          video_api::Buffer<video_api::ResourceUsage::staticGpu>& disabledVertexUniform);
      /// @brief Draw slider label + selected option name
      /// @remarks - Use 'bindGraphicsPipeline' (for control labels) and 'bindFragmentUniforms' (with label colors) before call.
      ///          - It's recommended to draw all labels using the same pipeline/uniform before using the other draw calls.
      inline void drawLabels(RendererContext& context) {
        labelMesh.draw(*context.renderer);
        if (selectedIndex >= 0)
          selectableValues[selectedIndex].nameMesh.draw(*context.renderer);
      }

    private:
      void init(RendererContext& context, const char32_t* label, int32_t x, int32_t labelY,
                const float arrowColor[4], ComboBoxOption* values, size_t valueCount);
      static constexpr inline uint32_t labelMargin() noexcept { return 6u; }

      struct OptionMesh final { // selectable value stored
        OptionMesh(RendererContext& context, display::Font& font, const char32_t* text, int32_t x, int32_t y, ComboValue value)
          : nameMesh(*context.renderer, font, text, context.pixelSizeX, context.pixelSizeY,
                     x, y, display::controls::TextAlignment::center),
            value(value) {}
        OptionMesh() = default;
        OptionMesh(const OptionMesh&) = default;
        OptionMesh(OptionMesh&&) noexcept = default;
        OptionMesh& operator=(const OptionMesh&) = default;
        OptionMesh& operator=(OptionMesh&&) noexcept = default;
        ~OptionMesh() noexcept { nameMesh.release(); }

        display::controls::TextMesh nameMesh;
        ComboValue value = 0;
      };
    private:
      display::controls::ControlMesh arrowLeftMesh;
      display::controls::ControlMesh arrowRightMesh;
      display::controls::TextMesh labelMesh;
      std::vector<OptionMesh> selectableValues;
      int32_t selectedIndex = -1;
      const bool* enabler = nullptr;

      std::function<void(uint32_t,ComboValue)> onChange;
      uint32_t operationId = 0;
      uint32_t minLabelWidth = 0;
      uint32_t fixedSliderWidth = 0;
      uint32_t paddingY = 0;
    };
  }
}
