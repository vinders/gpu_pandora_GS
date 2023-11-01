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
#include "menu/renderer_state_buffers.h"
#include "menu/controls/control.h"

#define MAX_INTEGER_LENGTH 10

namespace menu {
  namespace controls {
    enum class TextBoxType : uint32_t { ///< Text edit control value constraints
      text,    ///< Free text: no restrictions
      integer, ///< Integer: only allow 0-9
      number   ///< Floating-point number: only allow 0-9 and . and ,
    };

    /// @brief UI text edit control
    class TextBox final : public Control {
    public:
      /// @brief Create text edit control -- text value
      /// @param boundValue  Data/config value to bind to the text-box value (get/set)
      /// @param onChange    Event handler to call (with 'operationId') when the text-box value changes
      /// @param enabler     Optional data/config value to which the text-box state should be bound
      explicit TextBox(RendererContext& context, const char32_t* label, const char32_t* suffix,
                       int32_t x, int32_t labelY, uint32_t minLabelWidth, uint32_t fixedWidth, const float backgroundColor[4],
                       uint32_t operationId, std::function<void(uint32_t)> onChange, const char32_t* textValue,
                       uint32_t maxValueLength, const bool* enabler = nullptr)
        : enabler(enabler),
          onChange(std::move(onChange)),
          operationId(operationId),
          valueType(TextBoxType::text),
          maxValueLength(maxValueLength),
          minLabelWidth(minLabelWidth) {
        init(context, label, suffix, x, labelY, fixedWidth, backgroundColor, textValue);
      }
      /// @brief Create text edit control -- integer value
      /// @param boundValue  Data/config value to bind to the text-box value (get/set)
      /// @param onChange    Event handler to call (with 'operationId') when the text-box value changes
      /// @param enabler     Optional data/config value to which the text-box state should be bound
      explicit TextBox(RendererContext& context, const char32_t* label, const char32_t* suffix,
                       int32_t x, int32_t labelY, uint32_t minLabelWidth, uint32_t fixedWidth, const float backgroundColor[4],
                       uint32_t operationId, std::function<void(uint32_t)> onChange, uint32_t integerValue,
                       uint32_t maxValueLength, const bool* enabler = nullptr)
        : enabler(enabler),
          onChange(std::move(onChange)),
          operationId(operationId),
          valueType(TextBoxType::integer),
          maxValueLength(maxValueLength),
          minLabelWidth(minLabelWidth) {
        char32_t buffer[MAX_INTEGER_LENGTH+1];
        init(context, label, suffix, x, labelY, fixedWidth, backgroundColor, fromInteger(integerValue, buffer));
      }
      /// @brief Create text edit control -- number value
      /// @param boundValue  Data/config value to bind to the text-box value (get/set)
      /// @param onChange    Event handler to call (with 'operationId') when the text-box value changes
      /// @param enabler     Optional data/config value to which the text-box state should be bound
      explicit TextBox(RendererContext& context, const char32_t* label, const char32_t* suffix,
                       int32_t x, int32_t labelY, uint32_t minLabelWidth, uint32_t fixedWidth, const float backgroundColor[4],
                       uint32_t operationId, std::function<void(uint32_t)> onChange, double numberValue,
                       uint32_t maxValueLength, const bool* enabler = nullptr)
        : enabler(enabler),
          onChange(std::move(onChange)),
          operationId(operationId),
          valueType(TextBoxType::number),
          maxValueLength(maxValueLength),
          minLabelWidth(minLabelWidth) {
        auto buffer = fromNumber(numberValue, maxValueLength);
        init(context, label, suffix, x, labelY, fixedWidth, backgroundColor, buffer.get());
      }

      TextBox() = default;
      TextBox(const TextBox&) = delete;
      TextBox(TextBox&&) noexcept = default;
      TextBox& operator=(const TextBox&) = delete;
      TextBox& operator=(TextBox&&) noexcept = default;
      ~TextBox() noexcept { release(); }

      inline void release() noexcept {
        controlMesh.release();
        caretMesh.release();
        labelMesh.release();
        suffixMesh.release();
        inputMesh.release();
      }
      ControlType Type() const noexcept override;

      // -- accessors --

      inline int32_t x() const noexcept { return labelMesh.x(); }
      inline int32_t y() const noexcept { return controlMesh.y(); }
      inline int32_t controlX() const noexcept { return controlMesh.x(); }
      inline int32_t rightX() const noexcept {
        return suffixMesh.width() ? (suffixMesh.x() + (int32_t)suffixMesh.width())
                                  : (controlMesh.x() + (int32_t)controlMesh.width());
      }
      inline int32_t labelY() const noexcept { return labelMesh.y(); }

      inline uint32_t width() const noexcept { return static_cast<uint32_t>(rightX() - x()); }
      inline uint32_t height() const noexcept { return controlMesh.height(); }

      inline bool isEnabled() const noexcept { return (enabler == nullptr || *enabler); } ///< Verify if control is enabled
      inline bool isHover(int32_t mouseX, int32_t mouseY) const noexcept { ///< Verify mouse hover
        return (mouseY >= y() && mouseX >= x() && mouseY < y() + (int32_t)height() && mouseX < rightX());
      }
      /// @brief Get control status, based on mouse location (hover, disabled...)
      ControlStatus getStatus(int32_t mouseX, int32_t mouseY) const noexcept override;

      inline bool isEditMode() const noexcept { return isEditing; } ///< Verify if control is currently in edit mode
      inline const char32_t* valueText() const noexcept { return inputValue.data(); } ///< Get text value stored in text-box
      uint32_t valueInteger() const noexcept; ///< Get integer value stored in text-box (only with TextBoxType::integer)
      double valueNumber() const noexcept; ///< Get number value stored in text-box (only with TextBoxType::number or integer)

      // -- operations --

      /// @brief Report click to the control (on mouse click with hover -or- on keyboard/pad action)
      /// @returns True if the control is now open (text editing mode)
      bool click(RendererContext& context, int32_t mouseX = noMouseCoord()) override;
      void addChar(RendererContext& context, char32_t code); ///< Report character input to control (if edit mode is active)
      void removeChar(RendererContext& context);             ///< Report character removal to control (if edit mode is active)
      inline void previousChar(RendererContext& context) {   ///< Move caret at previous character if available (on keyboard/pad action, if edit mode is active)
        if (caretLocation > 0) {
          --caretLocation;
          updateCaretLocation(context);
        }
      }
      inline bool nextChar(RendererContext& context) { ///< Move caret at next character if available (on keyboard/pad action, if edit mode is active)
        if (caretLocation < (uint32_t)inputValue.size() - 1u/*ignore ending zero*/) {
          ++caretLocation;
          updateCaretLocation(context);
          return true;
        }
        return false;
      }
      void close() override; ///< Force-stop edit mode (on click elsewhere / on keyboard Enter)

      void move(RendererContext& context, int32_t x, int32_t labelY); ///< Change control location (on window resize)
      void replaceValueText(RendererContext& context, const char32_t* textValue); ///< Replace text input value (only with TextBoxType::text)
      void replaceValueInteger(RendererContext& context, uint32_t integerValue);  ///< Replace text input value (only with TextBoxType::integer or number)
      void replaceValueNumber(RendererContext& context, double numberValue);      ///< Replace text input value (only with TextBoxType::number)
      void updateLabels(RendererContext& context, const char32_t* label, const char32_t* suffix); ///< Change control label + suffix

      // -- rendering --

      /// @brief Draw text-box background/caret
      /// @remarks - Use 'bindGraphicsPipeline' (for control backgrounds) before call.
      ///          - It's recommended to draw all controls using the same pipeline/uniform before using the other draw calls.
      void drawBackground(RendererContext& context, RendererStateBuffers& buffers);
      /// @brief Draw text-box label + input value
      /// @remarks - Use 'bindGraphicsPipeline' (for control labels) before call.
      ///          - It's recommended to draw all labels using the same pipeline/uniform before using the other draw calls.
      void drawLabels(RendererContext& context, RendererStateBuffers& buffers, bool isActive);

      static constexpr inline int32_t noMouseCoord() noexcept { return 0x7FFFFFFF; } ///< Click coord for key/pad
    private:
      void init(RendererContext& context, const char32_t* label, const char32_t* suffix,
                int32_t x, int32_t labelY, uint32_t fixedWidth, const float color[4], const char32_t* initValue);
      void updateCaretLocation(RendererContext& context);

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

      std::function<void(uint32_t)> onChange;
      uint32_t operationId = 0;
      TextBoxType valueType = TextBoxType::text;
      uint32_t maxValueLength = 0xFFFFFFFFu;
      uint32_t minLabelWidth = 0;
    };
  }
}
