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
#include <display/controls/control_mesh.h>
#include <display/controls/text_mesh.h>
#include "menu/controls/types.h"

#define MAX_INTEGER_LENGTH 10

namespace menu {
  namespace controls {
    enum class TextBoxType : uint32_t { ///< Text edit control value constraints
      text,    ///< Free text: no restrictions
      integer, ///< Integer: only allow 0-9
      number   ///< Floating-point number: only allow 0-9 and . and ,
    };

    /// @brief UI text edit control
    class TextBox final {
    public:
      /// @brief Create text edit control -- text value
      /// @param boundValue  Data/config value to bind to the text-box value (get/set)
      /// @param enabler     Optional data/config value to which the text-box state should be bound
      TextBox(RendererContext& context, const char32_t* label, const char32_t* suffix,
              int32_t x, int32_t labelY, const ControlStyle& style, uint32_t fixedWidth,
              const char32_t* textValue, uint32_t maxValueLength, const bool* enabler = nullptr)
        : enabler(enabler),
          valueType(TextBoxType::text),
          maxValueLength(maxValueLength),
          minLabelWidth(style.minLabelWidth),
          paddingX(style.paddingX),
          paddingY(style.paddingY) {
        init(context, label, suffix, x, labelY, fixedWidth, style.color, textValue);
      }
      /// @brief Create text edit control -- integer value
      /// @param boundValue  Data/config value to bind to the text-box value (get/set)
      /// @param enabler     Optional data/config value to which the text-box state should be bound
      TextBox(RendererContext& context, const char32_t* label, const char32_t* suffix,
              int32_t x, int32_t labelY, const ControlStyle& style, uint32_t fixedWidth,
              uint32_t integerValue, uint32_t maxValueLength, const bool* enabler = nullptr)
        : enabler(enabler),
          valueType(TextBoxType::integer),
          maxValueLength(maxValueLength),
          minLabelWidth(style.minLabelWidth),
          paddingX(style.paddingX),
          paddingY(style.paddingY) {
        char32_t buffer[MAX_INTEGER_LENGTH+1];
        init(context, label, suffix, x, labelY, fixedWidth, style.color, fromInteger(integerValue, buffer));
      }
      /// @brief Create text edit control -- number value
      /// @param boundValue  Data/config value to bind to the text-box value (get/set)
      /// @param enabler     Optional data/config value to which the text-box state should be bound
      TextBox(RendererContext& context, const char32_t* label, const char32_t* suffix,
              int32_t x, int32_t labelY, const ControlStyle& style, uint32_t fixedWidth,
              double numberValue, uint32_t maxValueLength, const bool* enabler = nullptr)
        : enabler(enabler),
          valueType(TextBoxType::number),
          maxValueLength(maxValueLength),
          minLabelWidth(style.minLabelWidth),
          paddingX(style.paddingX),
          paddingY(style.paddingY) {
        auto buffer = fromNumber(numberValue, maxValueLength);
        init(context, label, suffix, x, labelY, fixedWidth, style.color, buffer.get());
      }

      TextBox() = default;
      TextBox(const TextBox&) = delete;
      TextBox(TextBox&&) noexcept = default;
      TextBox& operator=(const TextBox&) = delete;
      TextBox& operator=(TextBox&&) noexcept = default;
      ~TextBox() noexcept = default;

      // -- accessors --

      inline int32_t x() const noexcept { return controlMesh.x(); }
      inline int32_t y() const noexcept { return controlMesh.y(); }
      inline int32_t width() const noexcept { return controlMesh.width(); }
      inline int32_t height() const noexcept { return controlMesh.height(); }

      inline bool isEnabled() const noexcept { return (enabler == nullptr || *enabler); } ///< Verify if control is enabled
      inline bool isHover(int32_t mouseX, int32_t mouseY) const noexcept { ///< Verify mouse hover
        return (mouseY >= y() && mouseX >= x() && mouseY < y() + (int32_t)height() && mouseX < x() + (int32_t)width());
      }
      inline bool isEditMode() const noexcept { return isEditing; } ///< Verify if control is currently in edit mode
      inline const char32_t* valueText() const noexcept { return inputValue.data(); } ///< Get text value stored in text-box
      uint32_t valueInteger() const noexcept; ///< Get integer value stored in text-box (only with TextBoxType::integer)
      double valueNumber() const noexcept; ///< Get number value stored in text-box (only with TextBoxType::number or integer)

      // -- operations --

      void click(RendererContext& context, int32_t mouseX = noMouseCoord());  ///< Report click to control (on mouse click with hover / on keyboard/pad action)
      void addChar(RendererContext& context, char32_t code); ///< Report character input to control (if edit mode is active)
      void removeChar(RendererContext& context);             ///< Report character removal to control (if edit mode is active)
      inline void previousChar(RendererContext& context) {   ///< Move caret at previous character if available (on keyboard/pad action, if edit mode is active)
        if (caretLocation > 0) {
          --caretLocation;
          updateCaretLocation(context);
        }
      }
      inline void nextChar(RendererContext& context) { ///< Move caret at next character if available (on keyboard/pad action, if edit mode is active)
        if (caretLocation < (uint32_t)inputValue.size() - 1u/*ignore ending zero*/) {
          ++caretLocation;
          updateCaretLocation(context);
        }
      }
      void close() noexcept { isEditing = false; } ///< Force-stop edit mode (on click elsewhere / on keyboard Enter)

      void move(RendererContext& context, int32_t x, int32_t labelY); ///< Change control location (on window resize)
      void replaceValueText(RendererContext& context, const char32_t* textValue); ///< Replace text input value (only with TextBoxType::text)
      void replaceValueInteger(RendererContext& context, uint32_t integerValue);  ///< Replace text input value (only with TextBoxType::integer or number)
      void replaceValueNumber(RendererContext& context, double numberValue);      ///< Replace text input value (only with TextBoxType::number)

      /// @brief Draw text-box background/caret
      /// @remarks - Use 'bindGraphicsPipeline' (for control backgrounds) and 'bindVertexUniforms' (with color modifier) before call.
      ///          - It's recommended to draw all controls using the same pipeline/uniform before using the other draw calls.
      void drawBackground(RendererContext& context);
      /// @brief Draw text-box label
      /// @remarks - Use 'bindGraphicsPipeline' (for control labels) and 'bindFragmentUniforms' (with label colors) before call.
      ///          - It's recommended to draw all labels using the same pipeline/uniform before using the other draw calls.
      inline void drawLabel(RendererContext& context) { labelMesh.draw(*context.renderer); }
      /// @brief Draw text-box input text
      /// @remarks - Use 'bindGraphicsPipeline' (for control labels) and 'bindFragmentUniforms' (with input text colors) before call.
      ///          - It's recommended to draw all labels using the same pipeline/uniform before using the other draw calls.
      inline void drawInput(RendererContext& context) {
        inputMesh.draw(*context.renderer);
        suffixMesh.draw(*context.renderer);
      }

    private:
      void init(RendererContext& context, const char32_t* label, const char32_t* suffix,
                int32_t x, int32_t labelY, uint32_t fixedWidth, const float color[4], const char32_t* initValue);
      void updateCaretLocation(RendererContext& context);
      static constexpr inline uint32_t labelMargin() noexcept { return 6u; }
      static constexpr inline int32_t noMouseCoord() noexcept { return 0x7FFFFFFF; }

      static const char32_t* fromInteger(uint32_t integerValue, char32_t buffer[MAX_INTEGER_LENGTH + 1]) noexcept;
      static std::unique_ptr<char32_t[]> fromNumber(double numberValue, size_t bufferLength);

    private:
      display::controls::ControlMesh controlMesh;
      display::controls::ControlMesh caretMesh;
      display::controls::TextMesh labelMesh;
      display::controls::TextMesh suffixMesh;
      display::controls::TextMesh inputMesh;
      std::vector<char32_t> inputValue;
      const bool* enabler = nullptr;
      bool isEditing = false;
      uint32_t caretLocation = 0;
      uint32_t caretDrawCount = 0;

      TextBoxType valueType = TextBoxType::text;
      uint32_t maxValueLength = 0xFFFFFFFFu;
      uint32_t minLabelWidth = 0;
      uint32_t paddingX = 0;
      uint32_t paddingY = 0;
    };
  }
}
