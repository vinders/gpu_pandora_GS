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
#include "menu/controls/control.h"

namespace menu {
  enum class ColorThemeType : uint32_t { ///< Menu color theme type
    white = 0,  ///< white background, blue controls, classic
    blue,       ///< blue background, blue controls, gradients
    green,      ///< green background, green controls, gradients
    darkGreen,  ///< dark background, green controls, gradient boxes
    darkYellow  ///< dark background, yellow controls, gradient boxes
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

    // -- color accessors --

    inline const float* disabledControlModifier() const noexcept { return disabledControl; }///< Color modifier for disabled control backgrounds
    inline const float* activeControlModifier() const noexcept { return activeControl; }    ///< Color modifier for active/hover control backgrounds
    inline const float* activeScrollControlModifier() const noexcept{ return activeScrollControl; }///< Color modifier for inverted active/hover scroll controls
    inline const float* regularIconModifier() const noexcept { return regularIcon; }     ///< Color modifier for regular icons
    inline const float* disabledIconModifier() const noexcept { return disabledIcon; }   ///< Color modifier for disabled icons
    inline const float* activeIconModifier() const noexcept { return activeIcon; }       ///< Color modifier for active/hover icons
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
    inline controls::FieldsetStyle fieldsetStyle() const noexcept{ return fieldsetType; }///< Fieldset visual style

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
    inline const controls::ControlColors<3>& comboBoxColorParams() const noexcept { return comboBoxControlColors;  }
    inline const float* comboBoxControlColor() const noexcept { return comboBoxControlColors.colors[0]; }  ///< Combo-box control background color
    inline const float* comboBoxTopControlColor() const noexcept{ return comboBoxControlColors.colors[1]; }///< Combo-box control top of gradient
    inline const float* comboBoxDropdownColor() const noexcept { return comboBoxControlColors.colors[2]; } ///< Combo-box drop-down background color
    inline const float* comboBoxLabelColor() const noexcept { return comboBoxLabel; }               ///< Combo-box control text color
    inline const float* comboBoxDropdownLabelColor() const noexcept{ return comboBoxDropdownLabel; }///< Combo-box drop-down text color
    inline const controls::ControlColors<4>& rulerColorParams() const noexcept { return rulerControlColors;  }
    inline const float* rulerControlColor() const noexcept { return rulerControlColors.colors[0]; }///< Sliding-ruler background color
    inline const float* rulerBorderColor() const noexcept { return rulerControlColors.colors[1]; } ///< Sliding-ruler border color
    inline const float* rulerThumbColor() const noexcept { return rulerControlColors.colors[2]; }  ///< Sliding-ruler thumb color
    inline const float* rulerFillerColor() const noexcept { return rulerControlColors.colors[3]; } ///< Sliding-ruler filler color
    inline const float* sliderArrowColor() const noexcept { return sliderArrow; }        ///< Slider-box arrow color

  private:
    float disabledControl[4];
    float activeControl[4];
    float activeScrollControl[4];
    float regularIcon[4];
    float disabledIcon[4];
    float activeIcon[4];
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
    float comboBoxLabel[4];
    float comboBoxDropdownLabel[4];
    float sliderArrow[4];
    controls::ControlColors<3> comboBoxControlColors;
    controls::ControlColors<4> rulerControlColors;
    
    controls::FieldsetStyle fieldsetType = controls::FieldsetStyle::classic;
    ColorThemeType themeType_ = (ColorThemeType)-1;
  };
}
