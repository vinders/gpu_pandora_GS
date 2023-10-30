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
#include <display/video_api.h>

namespace menu {
  enum class ColorThemeType : uint32_t { ///< Menu color theme type
    lightBlue = 0, ///< light background, blue controls
    darkBlue,      ///< dark background, blue controls
    darkGreen,     ///< dark background, green controls
    darkYellow     ///< dark background, yellow controls
  };
  enum class FieldsetStyle : uint32_t { ///< Fieldset visual style
    classic = 0, ///< Contour border with a title bar
    title,       ///< Title with underline decoration and vertical line
    titleBack    ///< Title with underline decoration and vertical line and content background
  };

  // ---

  /// @brief UI color theme -- used to customize controls
  class ColorTheme final {
  public:
    inline ColorTheme(video_api::Renderer& renderer, ColorThemeType type_) {
      updateTheme(renderer, type_);
    }
    ColorTheme(const ColorTheme&) = default;
    ColorTheme& operator=(const ColorTheme&) = default;
    ~ColorTheme() noexcept = default;

    // -- theme selection --

    ColorThemeType themeType() const noexcept { return themeType_; } ///< Current theme type
    void updateTheme(video_api::Renderer& renderer, ColorThemeType type); ///< Change theme type

    // -- control size accessors --

    static constexpr inline uint32_t scrollbarWidth() noexcept { return 16; }   ///< Total scroll-bar width
    static constexpr inline uint32_t tooltipBarHeight() noexcept { return 30; } ///< Total scroll-bar width
    static constexpr inline uint32_t lineHoverPaddingX() noexcept { return 10; }///< Control line hover left/right padding
    static constexpr inline uint32_t autoScrollPaddingY() noexcept { return 8; }///< Padding above/below control during auto-scroll

    static constexpr inline int32_t pageFieldsetMarginX(uint32_t pageWidth) noexcept { ///< Left fieldset margin in the page
      return (pageWidth >= pageLabelWidth() + pageControlWidth() + scrollbarWidth() + 80u) ? 30 : 10;
    }
    static constexpr inline uint32_t fieldsetPaddingX(uint32_t pageWidth) noexcept { ///< Fieldset padding to the left of inner controls
      return (pageWidth >= pageLabelWidth() + pageControlWidth() + scrollbarWidth() + 80u) ? 20 : 8;
    }
    static constexpr inline uint32_t fieldsetPaddingY() noexcept { return 5; }  ///< Fieldset padding above first and below last inner control
    static constexpr inline uint32_t fieldsetBottom() noexcept { return 12; }   ///< Fieldset margin after last item (before next fieldset)
    static constexpr inline uint32_t fieldsetTitlePaddingX() noexcept { return 12; }///< Fieldset title horizontal padding
    static constexpr inline uint32_t fieldsetTitlePaddingY() noexcept { return 10; }///< Fieldset title vertical padding
    static constexpr inline uint32_t fieldsetMaxWidth() noexcept { return 720; }///< Fieldset maximum width

    static constexpr inline uint32_t pageLineHeight() noexcept { return 26; }   ///< Content line height (includes inter-line space)
    static constexpr inline uint32_t pageLabelWidth() noexcept { return 200; }  ///< Minimum label width (before controls)
    static constexpr inline uint32_t pageControlWidth() noexcept { return 260; }///< Fixed control/value width

    // -- control color accessors --

    inline const float* disabledControlModifier() const noexcept { return disabledControl; }///< Color modifier for disabled control backgrounds/icons
    inline const float* activeControlModifier() const noexcept { return activeControl; }    ///< Color modifier for active/hover control backgrounds/icons
    inline const float* activeLightControlModifier() const noexcept { return activeLightControl; } ///< Color modifier for active/hover light control backgrounds/icons
    inline const float* activeInvertControlModifier() const noexcept{ return activeInvertControl; }///< Color modifier for inverted active/hover control backgrounds/icons
    inline const float* regularLabelColor() const noexcept { return regularLabel; }      ///< Regular control label color
    inline const float* disabledLabelColor() const noexcept { return disabledLabel; }    ///< Disabled control label color
    inline const float* activeLabelColor() const noexcept { return activeLabel; }        ///< Active/hover control label color

    inline const float* backgroundColor() const noexcept { return background; }             ///< Page background color
    inline const float* backgroundCornerColor() const noexcept { return backgroundCorner; } ///< Page background bottom-right color
    inline const float* scrollbarControlColor() const noexcept { return scrollbarControl; } ///< Page scroll-bar background color
    inline const float* scrollbarThumbColor() const noexcept { return scrollbarThumb; }     ///< Page scroll-bar thumb color
    inline const float* lineSelectorControlColor() const noexcept{ return lineSelectorControl; }///< Page active/hover line selector background color
    inline const float* tooltipControlColor() const noexcept { return tooltipControl; }  ///< Page tooltip background color
    inline const float* titleLabelColor() const noexcept { return titleLabel; }          ///< Page title label color
    inline const float* fieldsetControlColor() const noexcept { return fieldsetControl; }///< Fieldset decoration color
    inline const float* fieldsetLabelColor() const noexcept { return fieldsetLabel; }    ///< Fieldset text color
    inline FieldsetStyle fieldsetStyle() const noexcept{ return fieldsetType; }///< Fieldset visual style

    inline const float* tabControlColor() const noexcept { return tabControl; }          ///< Tab-control tab background color
    inline const float* tabLineColor() const noexcept { return tabLine; }                ///< Tab-control bar color
    inline const float* tabActiveLineColor() const noexcept { return tabActiveLine; }    ///< Tab-control bar active area color
    inline const float* tabLabelColor() const noexcept { return tabLabel; }              ///< Tab-control tab text color
    inline const float* tabActiveLabelColor() const noexcept { return tabActiveLabel; }  ///< Tab-control active/hover tab text color
    inline const float* verticalTabControlColor() const noexcept { return verticalTabControl; }///< Vertical tab bar color
    inline const float* verticalTabBorderColor() const noexcept { return verticalTabBorder; }  ///< Vertical tab border color
    inline const float* verticalTabLabelColor() const noexcept { return verticalTabLabel; }    ///< Vertical tab text color
    inline const float* verticalTabActiveLabelColor() const noexcept{ return verticalTabActiveLabel; }///< Vertical active/hover tab text color

    inline const float* buttonControlColor() const noexcept { return buttonControl; }    ///< Regular button background color
    inline const float* buttonLabelColor() const noexcept { return buttonLabel; }        ///< Regular button text color
    inline const float* textBoxControlColor() const noexcept { return textBoxControl; }  ///< Text-box background color
    inline const float* textBoxLabelColor() const noexcept { return textBoxLabel; }      ///< Text-box text color
    inline const float* comboBoxControlColor() const noexcept { return comboBoxControl; }///< Combo-box control background color
    inline const float* comboBoxTopControlColor() const noexcept { return comboBoxTopControl; }///< Combo-box control top of gradient
    inline const float* comboBoxDropdownColor() const noexcept { return comboBoxDropdown; }///< Combo-box drop-down background color
    inline const float* comboBoxLabelColor() const noexcept { return comboBoxLabel; }    ///< Combo-box control text color
    inline const float* comboBoxDropdownLabelColor() const noexcept{ return comboBoxDropdownLabel; }///< Combo-box drop-down text color
    inline const float* rulerControlColor() const noexcept { return rulerControl; }      ///< Sliding-ruler background color
    inline const float* rulerBorderColor() const noexcept { return rulerBorder; }        ///< Sliding-ruler border color
    inline const float* rulerThumbColor() const noexcept { return rulerThumb; }          ///< Sliding-ruler thumb color
    inline const float* rulerFillerColor() const noexcept { return rulerFiller; }        ///< Sliding-ruler filler color
    inline const float* sliderArrowColor() const noexcept { return sliderArrow; }        ///< Slider-box arrow color

  private:
    float disabledControl[4];
    float activeControl[4];
    float activeLightControl[4];
    float activeInvertControl[4];
    float regularLabel[4];
    float disabledLabel[4];
    float activeLabel[4];

    float background[4];
    float backgroundCorner[4];
    float scrollbarControl[4];
    float scrollbarThumb[4];
    float lineSelectorControl[4];
    float tooltipControl[4];
    float titleLabel[4];
    float fieldsetControl[4];
    float fieldsetLabel[4];

    float tabControl[4];
    float tabLine[4];
    float tabActiveLine[4];
    float tabLabel[4];
    float tabActiveLabel[4];
    float verticalTabControl[4];
    float verticalTabBorder[4];
    float verticalTabLabel[4];
    float verticalTabActiveLabel[4];

    float buttonControl[4];
    float buttonLabel[4];
    float textBoxControl[4];
    float textBoxLabel[4];
    float comboBoxControl[4];
    float comboBoxTopControl[4];
    float comboBoxDropdown[4];
    float comboBoxLabel[4];
    float comboBoxDropdownLabel[4];
    float rulerControl[4];
    float rulerBorder[4];
    float rulerThumb[4];
    float rulerFiller[4];
    float sliderArrow[4];
    
    FieldsetStyle fieldsetType = FieldsetStyle::classic;
    ColorThemeType themeType_ = (ColorThemeType)-1;
  };
}
