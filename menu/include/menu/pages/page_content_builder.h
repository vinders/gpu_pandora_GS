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
#include <cstddef>
#include <functional>
#include <display/controls/control_mesh.h>
#include <display/controls/text_mesh.h>
#include "menu/color_theme.h"
#include "menu/renderer_context.h"
#include "menu/pages/page.h"

namespace menu {
  namespace controls {
    class Button;
    class CheckBox;
    class ComboBox;
    class ComboBoxOption;
    class Fieldset;
    class KeyBinding;
    class Ruler;
    class Slider;
    class TextBox;
  }
  namespace pages {
    /// @brief Helper to organize and build page content
    class PageContentBuilder final {
    public:
      PageContentBuilder(RendererContext& context, const ColorTheme& theme,
                         int32_t pageX, int32_t pageY, uint32_t pageWidth,
                         size_t reservedRegisteredControlCount = 8,
                         std::function<void(uint32_t)> keyChangeHandler = nullptr,
                         std::function<void(uint32_t,uint32_t)> keyValueChangeHandler = nullptr);
      PageContentBuilder(const PageContentBuilder&) = default;
      PageContentBuilder(PageContentBuilder&&) noexcept = default;
      PageContentBuilder& operator=(const PageContentBuilder&) = default;
      PageContentBuilder& operator=(PageContentBuilder&&) noexcept = default;
      ~PageContentBuilder() noexcept = default;
      
      /// @brief Get list of registered controls -- read and store after building all page controls
      inline std::vector<ControlRegistration>& controlRegistry() noexcept { return registry; }
      /// @brief Get horizontal line position for next control
      inline int32_t linePositionX() const noexcept { return controlX; }
      /// @brief Get vertical line position for next control
      inline int32_t linePositionY() const noexcept { return currentLineY; }
      
      /// @brief Set conditional enabler for future controls added
      inline void setEnabler(bool& boundValue) noexcept { enabler = &boundValue; }
      /// @brief Remove conditional enabler for future controls added
      inline void resetEnabler() noexcept { enabler = nullptr; }
      /// @brief Add/remove custom vertical offset
      inline void addLineOffset(int32_t offsetY) noexcept { currentLineY += offsetY; }
      
      // -- page decoration --
      
      /// @brief Create main page title mesh
      void addTitle(const char16_t* label, display::controls::TextMesh& outTitle);
      /// @brief Create fieldset (group of controls)
      void addFieldset(const char16_t* label, uint32_t controlLineCount, uint32_t additionalPaddingY,
                       controls::Fieldset& outFieldset);
      
      // -- page controls --
      
      /// @brief Create check-box control
      /// @param controlId  Control identifier used by change handler (or 0 to disable handler)
      void addCheckBox(uint32_t controlId, const char16_t* label, const char16_t* tooltip,
                       bool& boundValue, controls::CheckBox& outCheckBox);
      
      // ---
      
      /// @brief Create combo-box selector control with label
      /// @param controlId  Control identifier used by change handler (or 0 to disable handler)
      void addComboBox(uint32_t controlId, const char16_t* label, const char16_t* tooltip,
                       uint32_t minControlWidth, controls::ComboBoxOption* options, size_t optionCount,
                       int32_t selectedIndex, controls::ComboBox& outComboBox);
      /// @brief Create free-position combo-box selector control
      void addComboBox(uint32_t controlId, const char16_t* tooltip, int32_t x, uint32_t minControlWidth,
                       controls::ComboBoxOption* options, size_t optionCount,
                       int32_t selectedIndex, controls::ComboBox& outComboBox);
      /// @brief Create two combo-box selectors on the same line
      void addDoubleComboBox(uint32_t leftControlId, const char16_t* label, const char16_t* tooltip,
                             controls::ComboBoxOption* leftOptions, size_t leftOptionCount, int32_t selectedLeftIndex,
                             controls::ComboBox& outLeftComboBox,
                             uint32_t rightControlId, uint32_t rightControlWidth,
                             controls::ComboBoxOption* rightOptions, size_t rightOptionCount, int32_t selectedRightIndex,
                             controls::ComboBox& outRightComboBox);
      /// @brief Create combo-box selector control with a button on the same line
      void addComboBoxWithButton(uint32_t controlId, const char16_t* label, const char16_t* tooltip,
                                 controls::ComboBoxOption* options, size_t optionCount, controls::ComboBox& outComboBox,
                                 uint32_t buttonId, const char16_t* buttonLabel, controls::Button& outButton);
      
      // ---
      
      /// @brief Create sliding ruler control with label
      /// @param controlId  Control identifier used by change handler (or 0 to disable handler)
      void addRuler(uint32_t controlId, const char16_t* label, const char16_t* suffix, const char16_t* tooltip,
                    uint32_t controlWidth, uint32_t maxValue, uint32_t& boundValue, controls::Ruler& outRuler);
      /// @brief Create sliding ruler control with prefix/suffix
      void addRuler(uint32_t controlId, const char16_t* prefix, const char16_t* suffix, const char16_t* tooltip,
                    int32_t x, uint32_t controlWidth, uint32_t maxValue, uint32_t& boundValue, controls::Ruler& outRuler);
      
      // ---
      
      /// @brief Create left/right slider control with label
      /// @param controlId  Control identifier used by change handler (or 0 to disable handler)
      void addSlider(uint32_t controlId, const char16_t* label, const char16_t* tooltip,
                     uint32_t controlWidth, controls::ComboBoxOption* options, size_t optionCount,
                     int32_t selectedIndex, controls::Slider& outSlider);
      /// @brief Create free-position left/right slider control
      void addSlider(uint32_t controlId, const char16_t* tooltip, int32_t x, uint32_t controlWidth,
                     controls::ComboBoxOption* options, size_t optionCount,
                     int32_t selectedIndex, controls::Slider& outSlider);
      
      // ---
      
      /// @brief Create string-mode text-box control
      /// @param controlId  Control identifier used by change handler (or 0 to disable handler)
      void addStringTextBox(uint32_t controlId, const char16_t* label, const char16_t* tooltip,
                            const char16_t* value, uint32_t maxValueLength, controls::TextBox& outTextBox);
      /// @brief Create resolution integer text-box control
      /// @param controlId  Control identifier used by change handler (or 0 to disable handler)
      void addResolutionTextBox(uint32_t controlId, const char16_t* label, const char16_t* tooltip,
                                uint32_t controlWidth, uint32_t controlOffsetX,
                                uint32_t integerValue, controls::TextBox& outTextBox);
      /// @brief Create floating-point number text-box control
      /// @param controlId  Control identifier used by change handler (or 0 to disable handler)
      void addNumberTextBox(uint32_t controlId, const char16_t* label, const char16_t* suffix, const char16_t* tooltip,
                            uint32_t controlWidth, double numberValue, controls::TextBox& outTextBox);
      /// @brief Create two integer text-box controls on the same line
      void addDoubleTextBox(uint32_t leftControlId, uint32_t rightControlId,
                            const char16_t* label, const char16_t* suffix, const char16_t* tooltip,
                            uint32_t leftIntegerValue, uint32_t rightIntegerValue, uint32_t maxValueLength,
                            controls::TextBox& outLeftTextBox, controls::TextBox& outRightTextBox);
      
      // ---
      
      /// @brief Create single key binding (keyboard/controller)
      void addControllerKeyBinding(const char16_t* label, const char16_t* tooltip, uint32_t keyCode, controls::KeyBinding& outBinding);
      /// @brief Create keyboard and controller key binding
      void addDualKeyBinding(const char16_t* label, const char16_t* tooltip,
                             uint32_t keyboardKeyCode, uint32_t controllerKeyCode, controls::KeyBinding& outBinding);

      // ---

      /// @brief Create color-picker (combo-box and preview mesh)
      void addColorPicker(uint32_t controlId, const char16_t* label, const float previewColorRgba[4],
                          controls::ComboBoxOption* options, size_t optionCount, int32_t selectedIndex,
                          controls::ComboBox& outComboBox, display::controls::ControlMesh& outColorPreview);
      /// @brief Change preview of a color-picker (preview mesh)
      static void changeColorPickerColors(RendererContext& context, const float previewColorRgba[4],
                                          display::controls::ControlMesh& colorPreview);
    private:
      RendererContext* context = nullptr;
      const ColorTheme* theme = nullptr;
      int32_t controlX;
      int32_t currentLineY;
      int32_t fieldsetX;
      uint32_t fieldsetWidth;
      
      std::function<void(uint32_t)> keyChangeHandler = nullptr;
      std::function<void(uint32_t,uint32_t)> keyValueChangeHandler = nullptr;
      std::vector<ControlRegistration> registry;
      bool* enabler = nullptr;
    };
    
    // -------------------------------------------------------------------------
    
    /// @brief Helper to organize and move page content
    class PageContentMover final {
    public:
      PageContentMover(RendererContext& context, int32_t pageX, int32_t pageY, uint32_t pageWidth);
      PageContentMover(const PageContentMover&) = default;
      PageContentMover(PageContentMover&&) noexcept = default;
      PageContentMover& operator=(const PageContentMover&) = default;
      PageContentMover& operator=(PageContentMover&&) noexcept = default;
      ~PageContentMover() noexcept = default;

      /// @brief Get horizontal line position for next control
      inline int32_t linePositionX() const noexcept { return controlX; }
      /// @brief Get vertical line position for next control
      inline int32_t linePositionY() const noexcept { return currentLineY; }
      /// @brief Add/remove custom vertical offset
      inline void addLineOffset(int32_t offsetY) noexcept { currentLineY += offsetY; }

      // -- page decoration --

      /// @brief Move main page title mesh
      void moveTitle(display::controls::TextMesh& title);
      /// @brief Move fielset (group of controls)
      void moveFieldset(uint32_t controlLineCount, uint32_t additionalPaddingY, controls::Fieldset& fieldset);

      // -- page controls --

      /// @brief Move check-box control
      void moveCheckBox(controls::CheckBox& checkBox);

      // ---

      /// @brief Move combo-box selector control with label
      void moveComboBox(controls::ComboBox& comboBox);
      /// @brief Move free-position combo-box selector control
      void moveComboBox(int32_t x, controls::ComboBox& comboBox);
      /// @brief Move two combo-box selectors on the same line
      void moveDoubleComboBox(controls::ComboBox& leftComboBox, controls::ComboBox& rightComboBox);
      /// @brief Move combo-box selector control with a button on the same line
      void moveComboBoxWithButton(controls::ComboBox& comboBox, controls::Button& button);

      // ---

      /// @brief Move sliding ruler control with label
      void moveRuler(controls::Ruler& ruler);
      /// @brief Move sliding ruler control with prefix/suffix
      void moveRuler(int32_t x, controls::Ruler& ruler);

      // ---

      /// @brief Move left/right slider control with label
      void moveSlider(controls::Slider& slider);
      /// @brief Move free-position left/right slider control
      void moveSlider(int32_t x, controls::Slider& slider);

      // ---

      /// @brief Move string-mode/integer/floating-point text-box control
      void moveTextBox(controls::TextBox& textBox);
      /// @brief Move two integer text-box controls on the same line
      void moveDoubleTextBox(controls::TextBox& leftTextBox, controls::TextBox& rightTextBox);

      // ---

      /// @brief Move single/dual key binding (keyboard/controller)
      void moveKeyBinding(controls::KeyBinding& binding);

      // ---

      /// @brief Move color-picker (combo-box and preview mesh)
      void moveColorPicker(controls::ComboBox& comboBox, display::controls::ControlMesh& colorPreview);

    private:
      RendererContext* context = nullptr;
      int32_t controlX;
      int32_t currentLineY;
      int32_t fieldsetX;
      uint32_t fieldsetWidth;
    };
  }
}
