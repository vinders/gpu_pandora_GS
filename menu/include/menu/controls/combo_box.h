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

#include <cassert>
#include <cstdint>
#include <vector>
#include <memory>
#include <functional>
#include <display/controls/control_mesh.h>
#include <display/controls/text_mesh.h>
#include "menu/renderer_state_buffers.h"
#include "menu/controls/control.h"
#include "menu/controls/combo_box_option.h"

namespace menu {
  namespace controls {
    /// @brief UI button control
    class ComboBox final : public Control {
    public:
      /// @brief Create combo-box control
      /// @param colors       [0]: primary color / [1]: gradient top color / [2]: dropdown color
      /// @param operationId  Unique combo-box identifier (should be cast from an enum or constant)
      /// @param onChange     Event handler to call (with 'operationId' and value) when the combo-box value changes
      /// @param enabler      Optional data/config value to which the combo-box state should be bound
      ComboBox(RendererContext& context, const char32_t* label, int32_t x, int32_t labelY,
               uint32_t minLabelWidth, uint32_t minBoxWidth, ComboBoxStyle style, const ComboBoxColors& colors,
               uint32_t operationId, std::function<void(uint32_t,uint32_t)> onChange, ComboBoxOption* values,
               size_t valueCount, int32_t selectedIndex = -1, const bool* enabler = nullptr)
        : selectedIndex((selectedIndex < (int32_t)valueCount) ? selectedIndex : -1),
          enabler(enabler),
          onChange(std::move(onChange)),
          operationId(operationId),
          minLabelWidth(minLabelWidth),
          minBoxWidth(minBoxWidth) {
        assert(valueCount != 0);
        init(context, label, x, labelY, style, colors, values, valueCount);
      }

      ComboBox() = default;
      ComboBox(const ComboBox&) = delete;
      ComboBox(ComboBox&&) noexcept = default;
      ComboBox& operator=(const ComboBox&) = delete;
      ComboBox& operator=(ComboBox&&) noexcept = default;
      ~ComboBox() noexcept { release(); }

      inline void release() noexcept {
        controlMesh.release();
        dropdownMesh.release();
        dropdownHoverMesh.release();
        labelMesh.release();
        selectedNameMesh.release();
        selectableValues.clear();
      }
      ControlType type() const noexcept override;

      // -- accessors --

      inline int32_t x() const noexcept { return labelMesh.x(); }
      inline int32_t y() const noexcept { return controlMesh.y(); }
      inline int32_t controlX() const noexcept { return controlMesh.x(); }
      inline uint32_t width() const noexcept { return static_cast<uint32_t>(controlMesh.x() + (int32_t)controlMesh.width() - x()); }
      inline uint32_t height() const noexcept { return isListOpen ? controlMesh.height() + dropdownMesh.height() : controlMesh.height(); }

      inline bool isEnabled() const noexcept { return (enabler == nullptr || *enabler); } ///< Verify if control is enabled
      inline bool isOpen() const noexcept { return isListOpen; }   ///< Verify if the dropdown list is open
      bool isHover(int32_t mouseX, int32_t mouseY) const noexcept; ///< Verify mouse hover
      /// @brief Get control status, based on mouse location (hover, disabled...)
      ControlStatus getStatus(int32_t mouseX, int32_t mouseY) const noexcept override;

      inline int32_t getSelectedIndex() const noexcept { return selectedIndex; } ///< Currently selected index (or -1)
      inline const ComboValue* getSelectedValue() const noexcept { ///< Get value at selected index (if any)
        return (selectedIndex != -1) ? &(selectableValues[selectedIndex].value) : nullptr;
      }

      // -- operations --

      /// @brief Report click to the control (on mouse click with hover -or- on keyboard/pad action)
      /// @returns True if the control is now open (dropdown)
      bool click(RendererContext& context, int32_t mouseX) override;
      void mouseMove(RendererContext& context, int32_t mouseX, int32_t mouseY) override; ///< Report mouse move to control (on mouse move with hover)
      void selectPrevious(RendererContext& context);       ///< Select previous entry if available (on keyboard/pad action)
      void selectNext(RendererContext& context);           ///< Select next entry if available (on keyboard/pad action)
      void close() override;                               ///< Force-close the dropdown list without changing (if open)

      void move(RendererContext& context, int32_t x, int32_t labelY); ///< Change control location (on window resize)
      void replaceValues(RendererContext& context, ComboBoxOption* values, size_t valueCount, int32_t selectedIndex = -1); ///< Replace selectable values
      
      inline void setSelectedIndex(int32_t index) noexcept { ///< Force selection of a specific entry
        if (index >= 0 && index < (int32_t)selectableValues.size()) {
          if (selectedIndex != index) {
            selectedIndex = index;
            if (onChange != nullptr)
              onChange(operationId, (uint32_t)selectableValues[selectedIndex].value);
          }
        }
        else selectedIndex = -1;
      }

      // -- rendering --

      /// @brief Draw combo-box background/arrow
      /// @remarks - Use 'bindGraphicsPipeline' (for control backgrounds) before call.
      ///          - It's recommended to draw all controls using the same pipeline/uniform before using the other draw calls.
      inline void drawBackground(RendererContext& context, RendererStateBuffers& buffers, bool isActive) {
        buffers.bindControlBuffer(context.renderer(), isEnabled() ? (isActive ? ControlBufferType::active : ControlBufferType::regular)
                                                                  : ControlBufferType::disabled);
        controlMesh.draw(context.renderer());
      }
      /// @brief Draw combo-box drop-down background/hover (if open)
      /// @remarks - Use 'bindGraphicsPipeline' (for control backgrounds) before call.
      ///          - It's recommended to draw all controls using the same pipeline/uniform before using the other draw calls.
      void drawDropdown(RendererContext& context, RendererStateBuffers& buffers);
      /// @brief Draw combo-box label + selected option name
      /// @remarks - Use 'bindGraphicsPipeline' (for control labels) before call.
      ///          - It's recommended to draw all labels using the same pipeline/uniform before using the other draw calls.
      void drawLabels(RendererContext& context, RendererStateBuffers& buffers, bool isActive);
      /// @brief Draw drop-down option names (if open)
      /// @remarks - Use 'bindGraphicsPipeline' (for control labels) before call.
      ///          - It's recommended to draw all labels using the same pipeline/uniform before using the other draw calls.
      void drawOptions(RendererContext& context, RendererStateBuffers& buffers);

    private:
      void init(RendererContext& context, const char32_t* label, int32_t x, int32_t labelY, ComboBoxStyle style,
                const ComboBoxColors& colors, ComboBoxOption* values, size_t valueCount);
      void moveDropdownHover(RendererContext& context, int32_t hoverIndex);

      struct OptionMesh final { // selectable value stored
        OptionMesh(RendererContext& context, display::Font& font, const char32_t* text, int32_t x, int32_t y, ComboValue value)
          : nameMesh(context.renderer(), font, text, context.pixelSizeX(), context.pixelSizeY(), x, y), value(value) {}
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
      display::controls::ControlMesh controlMesh;
      display::controls::ControlMesh dropdownMesh;
      display::controls::ControlMesh dropdownHoverMesh;
      display::controls::TextMesh labelMesh;
      display::controls::TextMesh selectedNameMesh;
      std::vector<OptionMesh> selectableValues;
      int32_t selectedIndex = -1;
      int32_t hoverIndex = -1;
      const bool* enabler = nullptr;
      bool isListOpen = false;

      std::function<void(uint32_t,ComboValue)> onChange;
      uint32_t operationId = 0;
      uint32_t minLabelWidth = 0;
      uint32_t minBoxWidth = 0;
    };
  }
}
