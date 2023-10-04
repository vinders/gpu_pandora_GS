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

namespace menu {
  namespace controls {
    using ComboValue = uint32_t;

    /// @brief Entry for combo-box creation
    struct ComboBoxEntry final {
      ComboBoxEntry(const char32_t* name, ComboValue value)
        : name(display::controls::TextMesh::toString(name)), value(value) {}
      ComboBoxEntry() = default;
      ComboBoxEntry(const ComboBoxEntry&) = default;
      ComboBoxEntry(ComboBoxEntry&&) noexcept = default;
      ComboBoxEntry& operator=(const ComboBoxEntry&) = default;
      ComboBoxEntry& operator=(ComboBoxEntry&&) noexcept = default;
      ~ComboBoxEntry() noexcept = default;

      std::unique_ptr<char32_t[]> name = nullptr;
      ComboValue value = 0;
    };

    // ---

    /// @brief UI button control
    class ComboBox final {
    public:
      /// @brief Create combo-box control
      /// @param operationId   Unique combo-box identifier (should be cast from an enum or constant)
      /// @param onClick       Event handler to call (with 'operationId' and value) when the combo-box is clicked
      /// @param enabler       Optional data/config value to which the combo-box state should be bound
      ComboBox(RendererContext& context, const char32_t* label, int32_t x, int32_t y, uint32_t paddingX,
               uint32_t paddingY, uint32_t minLabelWidth, uint32_t minBoxWidth, const float color[4],
               const float dropdownColor[4], uint32_t operationId, std::function<void(uint32_t,ComboValue)> onChange,
               ComboBoxEntry* values, size_t valueCount, int32_t selectedIndex = -1, const bool* enabler = nullptr)
        : selectedIndex((selectedIndex < (int32_t)valueCount) ? selectedIndex : -1),
          enabler(enabler),
          onChange(std::move(onChange)),
          operationId(operationId),
          minLabelWidth(minLabelWidth),
          minBoxWidth(minBoxWidth),
          paddingX(paddingX),
          paddingY(paddingY) {
        init(context, label, x, y, color, dropdownColor, values, valueCount);
      }

      ComboBox() = default;
      ComboBox(const ComboBox&) = delete;
      ComboBox(ComboBox&&) noexcept = default;
      ComboBox& operator=(const ComboBox&) = delete;
      ComboBox& operator=(ComboBox&&) noexcept = default;
      ~ComboBox() noexcept = default;

      // -- accessors --

      inline int32_t x() const noexcept { return (labelMesh.width() == 0 || isListOpen) ? controlMesh.x() : labelMesh.x(); }
      inline int32_t y() const noexcept { return controlMesh.y(); }
      inline int32_t width() const noexcept { return controlMesh.width(); }
      inline int32_t height() const noexcept { return isListOpen ? controlMesh.height() : controlMesh.height() + dropdownMesh.height(); }

      inline bool isEnabled() const noexcept { return (enabler == nullptr || *enabler); } ///< Verify if control is enabled
      inline bool isOpen() const noexcept { return isListOpen; }           ///< Verify if the dropdown list is open
      inline bool isHover(int32_t mouseX, int32_t mouseY) const noexcept { ///< Verify mouse hover
        return (mouseY >= y() && mouseY < y() + (int32_t)height() && mouseX >= x() && mouseX < x() + (int32_t)width());
      }

      // -- operations --

      void click(RendererContext& context);                     ///< Report click to control (on mouse click with hover / on keyboard/pad action)
      void mouseMove(RendererContext& context, int32_t mouseY); ///< Report click to control (on mouse move with hover)
      void selectPrevious(RendererContext& context);       ///< Select previous entry if available (on keyboard/pad action)
      void selectNext(RendererContext& context);           ///< Select next entry if available (on keyboard/pad action)
      inline void close() noexcept { isListOpen = false; } ///< Force-close the dropdown list without changing (if open)

      void move(RendererContext& context, int32_t x, int32_t y); ///< Change control location (on window resize)
      
      inline void setSelectedIndex(int32_t index) noexcept { ///< Force selection of a specific entry
        if (index >= 0 && index < (int32_t)selectableValues.size()) {
          if (selectedIndex != index) {
            selectedIndex = index;
            onChange(operationId, selectableValues[selectedIndex].value);
          }
        }
        else selectedIndex = -1;
      }

      /// @brief Draw combo-box background/arrow + drop-down background/hover (if open)
      /// @remarks - Use 'bindGraphicsPipeline' (for control backgrounds) and 'bindVertexUniforms' (with color modifier) before call.
      ///          - It's recommended to draw all controls using the same pipeline/uniform before using the other draw calls.
      void drawBackground(RendererContext& context);
      /// @brief Draw combo-box label
      /// @remarks - Use 'bindGraphicsPipeline' (for control labels) and 'bindFragmentUniforms' (with label colors) before call.
      ///          - It's recommended to draw all labels using the same pipeline/uniform before using the other draw calls.
      inline void drawLabel(RendererContext& context) { labelMesh.draw(*context.renderer); }
      /// @brief Draw combo-box selected option name + drop-down option names (if open)
      /// @remarks - Use 'bindGraphicsPipeline' (for control input text) and 'bindFragmentUniforms' (with input text colors) before call.
      ///          - It's recommended to draw all labels using the same pipeline/uniform before using the other draw calls.
      inline void drawOptionNames(RendererContext& context) {
        selectedNameMesh.draw(*context.renderer);
        if (isListOpen) {
          const auto* endEntries = &selectableValues[0] + (intptr_t)selectableValues.size();
          for (auto* entry = &selectableValues[0]; entry < endEntries; ++entry)
            entry->nameMesh.draw(*context.renderer);
        }
      }

    private:
      void init(RendererContext& context, const char32_t* label, int32_t x, int32_t y,
                const float color[4], const float dropdownColor[4], ComboBoxEntry* values, size_t valueCount);
      void moveDropdownHover(RendererContext& context, int32_t hoverIndex);
      static constexpr inline uint32_t labelMargin() noexcept { return 6u; }

      struct EntryData final { // selectable value stored
        EntryData(RendererContext& context, display::Font& font, const char32_t* text, int32_t x, int32_t y, ComboValue value)
          : nameMesh(*context.renderer, font, text, context.pixelSizeX, context.pixelSizeY, x, y),
            value(value) {}
        EntryData() = default;
        EntryData(const EntryData&) = default;
        EntryData(EntryData&&) noexcept = default;
        EntryData& operator=(const EntryData&) = default;
        EntryData& operator=(EntryData&&) noexcept = default;
        ~EntryData() noexcept = default;

        display::controls::TextMesh nameMesh;
        ComboValue value = 0;
      };
    private:
      display::controls::ControlMesh controlMesh;
      display::controls::ControlMesh dropdownMesh;
      display::controls::ControlMesh dropdownHoverMesh;
      display::controls::TextMesh labelMesh;
      display::controls::TextMesh selectedNameMesh;
      std::vector<EntryData> selectableValues;
      int32_t selectedIndex = -1;
      int32_t hoverIndex = -1;
      const bool* enabler = nullptr;
      bool isListOpen = false;

      std::function<void(uint32_t,ComboValue)> onChange;
      uint32_t operationId = 0;
      uint32_t minLabelWidth = 0;
      uint32_t minBoxWidth = 0;
      uint32_t paddingX = 0;
      uint32_t paddingY = 0;
    };
  }
}
