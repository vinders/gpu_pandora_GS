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
#include <display/font.h>
#include "menu/renderer_context.h"

namespace menu {
  namespace controls {
    enum class ControlType : uint32_t { ///< Selectable menu control type
      unknown = 0,
      button,   ///< Button (with optional icon)
      checkBox, ///< Check-box (with optional label)
      comboBox, ///< Combo-box dropdown selector (with optional label)
      textBox,  ///< Text edit box (with optional label and suffix)
      ruler,    ///< Sliding ruler (with optional label)
      slider    ///< Left/right slider selector (with optional label)
    };
    enum class ControlStatus : uint32_t { ///< Control status type
      regular = 0, ///< Neutral status
      disabled,    ///< Control currently can't be used
      hover        ///< The mouse is located on the control
    };

    /// @brief Selectable menu control -- interface
    class Control {
    public:
      static constexpr inline uint32_t labelMargin() noexcept { return 6u; } ///< Horizontal margin between label and control mesh
      virtual ~Control() noexcept = default;

      virtual ControlType Type() const noexcept = 0; ///< Get control type
      /// @brief Get control status, based on mouse location (hover, disabled...)
      virtual ControlStatus getStatus(int32_t mouseX, int32_t mouseY) const noexcept = 0;

      /// @brief Report click to the control (on mouse click with hover -or- on keyboard/pad action)
      /// @returns True if the control is now open (open combo-box, edited text-box, dragged ruler...)
      virtual bool click(RendererContext& context, int32_t mouseX) = 0;
      /// @brief Report mouse move to control (on mouse move when control is open: dropdown, dragging...)
      virtual void mouseMove(RendererContext& /*context*/, int32_t /*mouseX*/, int32_t /*mouseY*/) {}
      /// @brief Report end of mouse click (after drag)
      /// @returns True if the control has been closed by this action
      virtual bool mouseUp(RendererContext& /*context*/, int32_t /*mouseX*/) { return false; }
      /// @brief Force-close the control (if open: dropdown, text editing, dragging...)
      virtual void close() {}
    };
    
    
    // -- control styling -- ---------------------------------------------------
    
    /// @brief Visual style properties for a complex control
    struct ControlStyle final {
      ControlStyle(const float color_[4], uint32_t minLabelWidth, uint32_t paddingX = 0, uint32_t paddingY = 0)
        : minLabelWidth(minLabelWidth), paddingX(paddingX), paddingY(paddingY) {
        this->color[0] = color_[0];
        this->color[1] = color_[1];
        this->color[2] = color_[2];
        this->color[3] = color_[3];
      }
      ControlStyle() = default;
      ControlStyle(const ControlStyle&) = default;
      ControlStyle(ControlStyle&&) noexcept = default;
      ControlStyle& operator=(const ControlStyle&) = default;
      ControlStyle& operator=(ControlStyle&&) noexcept = default;
      ~ControlStyle() noexcept = default;

      float color[4]{ 0.f,0.f,0.f,1.f }; ///< Primary color type (background, symbols...)
      uint32_t minLabelWidth = 0; ///< Minimum width of the label prefixed (if any label value is provided)
      uint32_t paddingX = 0; ///< Left/right padding (between content and container)
      uint32_t paddingY = 0; ///< Top/bottom padding (between content and container)
    };

    /// @brief Visual style properties for a button control
    struct ButtonStyle final {
      ButtonStyle(const float color_[4], FontType fontType, display::ControlIconType icon,
                  uint32_t minButtonWidth = 0, uint32_t paddingX = 0, uint32_t paddingY = 0)
        : fontType(fontType), icon(icon), minButtonWidth(minButtonWidth), paddingX(paddingX), paddingY(paddingY) {
        this->color[0] = color_[0];
        this->color[1] = color_[1];
        this->color[2] = color_[2];
        this->color[3] = color_[3];
      }
      ButtonStyle() = default;
      ButtonStyle(const ButtonStyle&) = default;
      ButtonStyle(ButtonStyle&&) noexcept = default;
      ButtonStyle& operator=(const ButtonStyle&) = default;
      ButtonStyle& operator=(ButtonStyle&&) noexcept = default;
      ~ButtonStyle() noexcept = default;

      float color[4]{ 0.f,0.f,0.f,1.f };    ///< Background color type
      FontType fontType = FontType::titles; ///< Font type to use
      display::ControlIconType icon = display::ControlIconType::none; ///< Icon to display (if available)
      uint32_t minButtonWidth = 0; ///< Minimum button width (if text + paddingX doesn't reach it)
      uint32_t paddingX = 0; ///< Left/right padding (between border and inner text/icon)
      uint32_t paddingY = 0; ///< Top/bottom padding (between border and inner text/icon)
    };
  }
}
