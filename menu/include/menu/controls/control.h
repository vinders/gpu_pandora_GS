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
#include <cstring>
#include <display/font.h>
#include "menu/renderer_context.h"

namespace menu {
  namespace controls {
    enum class ControlType : uint32_t { ///< Selectable menu control type
      unknown = 0,
      button,    ///< Button (with optional icon)
      checkBox,  ///< Check-box (with optional label)
      comboBox,  ///< Combo-box dropdown selector (with optional label)
      textBox,   ///< Text edit box (with optional label and suffix)
      ruler,     ///< Sliding ruler (with optional label)
      slider,    ///< Left/right slider selector (with optional label)
      keyBinding,///< Key-binding box (with optional label)
      tile       ///< Selector tile
    };
    enum class ControlStatus : uint32_t { ///< Control status type
      regular = 0, ///< Neutral status
      disabled,    ///< Control currently can't be used
      hover        ///< The mouse is located on the control
    };

    /// @brief Selectable menu control -- interface
    class Control {
    public:
      virtual ~Control() noexcept = default;

      virtual ControlType type() const noexcept = 0; ///< Get control type
      /// @brief Get control status, based on mouse location (hover, disabled...)
      virtual ControlStatus getStatus(int32_t mouseX, int32_t mouseY) const noexcept = 0;

      /// @brief Report click to the control (on mouse click with hover -or- on keyboard/pad action)
      /// @returns True if the control is now open (open combo-box, edited text-box, dragged ruler...)
      virtual bool click(RendererContext& context, int32_t mouseX, int32_t mouseY) = 0;
      /// @brief Report mouse move to control (on mouse move when control is open: dropdown, dragging...)
      virtual void mouseMove(RendererContext& /*context*/, int32_t /*mouseX*/, int32_t /*mouseY*/) {}
      /// @brief Report end of mouse click (after drag)
      /// @returns True if the control has been closed by this action
      virtual bool mouseUp(RendererContext& /*context*/, int32_t /*mouseX*/) { return false; }
      /// @brief Force-close the control (if open: dropdown, text editing, dragging...)
      virtual void close() {}

      // -- page sizes --

      static constexpr inline uint32_t pageLineHeight() noexcept { return 26; }   ///< Content line height (includes inter-line space)
      static constexpr inline uint32_t pageLabelWidth() noexcept { return 200; }  ///< Minimum label width (before controls)
      static constexpr inline uint32_t pageControlWidth() noexcept { return 260; }///< Fixed control/value width
      static constexpr inline uint32_t scrollbarWidth() noexcept { return 16; }   ///< Total scroll-bar width
      static constexpr inline uint32_t tooltipBarHeight() noexcept { return 30; } ///< General tooltip bar height
      static constexpr inline uint32_t tooltipPaddingX() noexcept { return 16; }  ///< Horizontal tooltip padding
      static constexpr inline uint32_t lineHoverPaddingX() noexcept { return 10; }///< Control line hover left/right padding
      static constexpr inline uint32_t autoScrollPaddingY() noexcept { return 8; }///< Padding above/below control during auto-scroll

      static constexpr inline uint32_t sectionWideTabWidth() noexcept { return 120; } ///< Vertical section tab width -- wide
      static inline uint32_t sectionTabWidth(uint32_t clientWidth) noexcept { ///< Vertical section tab width
        return (clientWidth >= 720u) ? sectionWideTabWidth() : 70;
      }
      static constexpr inline uint32_t maxPageTabWidth() noexcept { return 200; }   ///< Maximum page tab width
      static constexpr inline uint32_t minPageTabPaddingX() noexcept { return 12; } ///< Minimum horizontal padding before/after page tabs
      static constexpr inline uint32_t maxPageTabPaddingX() noexcept { return 30; } ///< Maximum horizontal padding before/after page tabs
      static constexpr inline uint32_t pageTabPaddingY() noexcept { return 11; }    ///< Vertical padding above/below page tabs

      // -- control sizes --

      static constexpr inline uint32_t fieldsetTitleShortPaddingX() noexcept{ return 9; }///< Fieldset title horizontal padding -- gradient style
      static constexpr inline uint32_t fieldsetTitleWidePaddingX() noexcept{ return 12; }///< Fieldset title horizontal padding -- classic style
      static constexpr inline uint32_t fieldsetTitlePaddingY() noexcept { return 10; }   ///< Fieldset title vertical padding
      static constexpr inline int32_t fieldsetMarginX(uint32_t pageWidth) noexcept {     ///< Fieldset left margin in the page
        return (pageWidth >= Control::fieldsetMaxWidth() + Control::scrollbarWidth() + 2u*30u)
               ? ((pageWidth - Control::fieldsetMaxWidth() - Control::scrollbarWidth()) >> 1)
               : (pageWidth >= pageLabelWidth() + pageControlWidth() + scrollbarWidth() + 80u) ? 30 : 10;
      }
      static constexpr inline uint32_t fieldsetContentMarginX(uint32_t pageWidth) noexcept { ///< Margin to the left of fieldset inner controls
        return (pageWidth >= pageLabelWidth() + pageControlWidth() + scrollbarWidth() + 80u) ? 20 : 8;
      }
      static constexpr inline uint32_t fieldsetContentPaddingTop() noexcept { return 6; }   ///< Padding above first fieldset inner control
      static constexpr inline uint32_t fieldsetContentPaddingBottom() noexcept { return 5; }///< Padding below last fieldset inner control
      static constexpr inline uint32_t fieldsetContentMarginBottom() noexcept{ return 12; } ///< Margin after last fieldset inner control (before next fieldset)
      static constexpr inline uint32_t fieldsetContentHeight(uint32_t lineCount) { ///< Fieldset content height (based on the number of content lines)
        return (pageLineHeight()*lineCount + fieldsetContentPaddingTop() + fieldsetContentPaddingBottom());
      }
      static constexpr inline uint32_t fieldsetMaxWidth() noexcept { return 580; } ///< Fieldset maximum width

      static constexpr inline uint32_t titleMarginTop() noexcept { return 24; }       ///< Top margin above main page title
      static constexpr inline uint32_t labelMargin() noexcept { return 6u; }          ///< Horizontal margin between label and control mesh
      static constexpr inline uint32_t buttonIconLabelMargin() noexcept { return 4u; }///< Margin between button icon and label
      static constexpr inline uint32_t controlSideMargin() noexcept { return 3u; }  ///< Margin between button and control
      static constexpr inline uint32_t buttonPaddingX() noexcept { return 12u; }      ///< Horizontal button padding

      static constexpr inline uint32_t comboBoxPaddingX() noexcept { return 10u; } ///< Horizontal combo-box padding
      static constexpr inline uint32_t comboBoxPaddingY() noexcept { return 7u; }  ///< Vertical combo-box padding
      static constexpr inline uint32_t textBoxPaddingX() noexcept { return 10u; }  ///< Horizontal text-box padding
      static constexpr inline uint32_t textBoxPaddingY() noexcept { return 6u; }   ///< Vertical text-box padding
      static constexpr inline uint32_t rulerPaddingX() noexcept { return 4u; }     ///< Horizontal sliding-ruler padding
      static constexpr inline uint32_t rulerPaddingY() noexcept { return 4u; }     ///< Vertical sliding-ruler padding
      static constexpr inline uint32_t sliderPaddingY() noexcept { return 6u; }    ///< Vertical slider padding
      static constexpr inline uint32_t keyboardKeySideX() noexcept { return 4u; }   ///< Keyboard key side width
      static constexpr inline uint32_t keyboardKeySideY() noexcept { return 3u; }   ///< Keyboard key side height
      static constexpr inline uint32_t keyboardKeyPaddingX() noexcept { return 6u; }///< Horizontal keyboard key inner padding
      static constexpr inline uint32_t keyboardKeyPaddingY() noexcept { return 5u; }///< Vertical keyboard key inner padding

      static constexpr const uint32_t minTilePaddingX() noexcept { return 4u; }  ///< Minimum horizontal tile padding
      static constexpr const uint32_t maxTilePaddingX() noexcept { return 10u; } ///< Maximum horizontal tile padding
      static constexpr const uint32_t tilePaddingY() noexcept { return 11u; }    ///< Vertical tile padding
      static constexpr const uint32_t tileContentWidth() noexcept { return 192u; } ///< Tile content width (without padding)
      static constexpr const uint32_t tileContentHeight(uint32_t fontHeight) noexcept { ///< Tile content height (without padding)
        return fontHeight ? fontHeight*3u : 50;
      }
      static constexpr inline int32_t tileGridMarginX(uint32_t pageWidth, uint32_t gridWidth,   ///< Tile grid left margin in the page
                                                      uint32_t pageHeight, uint32_t contentHeight) noexcept {
        return (pageHeight > contentHeight)
               ? (((int32_t)pageWidth - (int32_t)gridWidth) >> 1)
               : (((int32_t)pageWidth - (int32_t)gridWidth - (int32_t)Control::scrollbarWidth()) >> 1);
      }
    };
    
    
    // -- control styling -- ---------------------------------------------------

    enum class ComboBoxStyle : uint32_t { ///< Combo-box visual style
      classic = 0, ///< Rectangle
      cutCorner    ///< Rectangle with top-right corner cut
    };
    enum class ButtonStyle : uint32_t { ///< Button visual style
      fromBottomLeft = 0, ///< Top-left/bottom-right corners cut
      fromTopLeft         ///< Bottom-left/top-right corners cut
    };

    template <size_t ColorCount>
    struct ControlColors { ///< Multi-color control style
      float colors[ColorCount][4]; ///< RGBA colors (e.g. background, border...)
    };
    using ComboBoxColors = ControlColors<3>;
    using RulerColors = ControlColors<4>;
    using TabControlColors = ControlColors<3>;
    using KeyboardKeyColors = ControlColors<7>;

    // ---

    /// @brief Visual style properties for a button control
    struct ButtonStyleProperties final {
      ButtonStyleProperties(ButtonStyle style, FontType fontType, display::ControlIconType icon, const float backgroundColor_[4],
                            uint32_t minButtonWidth = 0, uint32_t paddingX = 0, uint32_t paddingY = 0)
        : style(style), fontType(fontType), icon(icon), minButtonWidth(minButtonWidth), paddingX(paddingX), paddingY(paddingY) {
        memcpy(this->backgroundColor, backgroundColor_, sizeof(float)*4u);
      }
      ButtonStyleProperties(ButtonStyle style, FontType fontType, display::ControlIconType icon, const float backgroundColor_[4],
                            const float borderColor_[4], size_t borderSize_, uint32_t minButtonWidth,
                            uint32_t paddingX, uint32_t paddingY)
        : borderSize(borderSize_), style(style), fontType(fontType), icon(icon),
          minButtonWidth(minButtonWidth), paddingX(paddingX), paddingY(paddingY) {
        memcpy(this->backgroundColor, backgroundColor_, sizeof(float)*4u);
        memcpy(this->borderColor, borderColor_, sizeof(float)*4u);
      }
      ButtonStyleProperties() = default;
      ButtonStyleProperties(const ButtonStyleProperties&) = default;
      ButtonStyleProperties(ButtonStyleProperties&&) noexcept = default;
      ButtonStyleProperties& operator=(const ButtonStyleProperties&) = default;
      ButtonStyleProperties& operator=(ButtonStyleProperties&&) noexcept = default;
      ~ButtonStyleProperties() noexcept = default;

      float backgroundColor[4]{ 0.f,0.f,0.f,1.f }; ///< Background color type
      float borderColor[4]{ 0.f,0.f,0.f,1.f };     ///< Border color type
      size_t borderSize = 0;                       ///< Border pixel size
      ButtonStyle style = ButtonStyle::fromBottomLeft; ///< Button visual style
      FontType fontType = FontType::titles;        ///< Font type to use
      display::ControlIconType icon = display::ControlIconType::none; ///< Icon to display (if available)
      uint32_t minButtonWidth = 0; ///< Minimum button width (if text + paddingX doesn't reach it)
      uint32_t paddingX = 0; ///< Left/right padding (between border and inner text/icon)
      uint32_t paddingY = 0; ///< Top/bottom padding (between border and inner text/icon)
    };
  }
}
