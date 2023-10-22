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
#include "menu/controls/fieldset.h"

namespace menu {
  /// @brief UI color theme -- used to customize controls
  struct ColorTheme final {
    float baseLabel[4]{ 64.f/255.f,64.f/255.f,64.f/255.f, 1.f };            ///< Regular control label color
    float baseHoverLabel[4]{ 82.f/255.f,82.f/255.f,82.f/255.f, 1.f };       ///< Regular active/hover control label color
    float sectionLabel[4]{ 22.f/255.f,22.f/255.f,22.f/255.f, 1.f };         ///< Main section label color
    float tooltipBackground[4]{ 220.f/255.f,220.f/255.f,220.f/255.f, 1.f };         ///< Tooltip label color
    float tooltipLabel[4]{ 64.f/255.f,64.f/255.f,64.f/255.f, 1.f };         ///< Tooltip label color
    float titleLabel[4]{ 22.f/255.f,22.f/255.f,22.f/255.f, 1.f };           ///< Fieldset title label color
    float hoverArea[4]{ 138.f/255.f,174.f/255.f,208.f/255.f, 0.25f };            ///< Hover/selected line area color
    
    // ---
    
    float scrollbarControl[4]{ 240.f/255.f,240.f/255.f,240.f/255.f, 1.f };     ///< Scroll-bar background color
    float scrollbarThumb[4]{ 205.f/255.f,205.f/255.f,205.f/255.f, 1.f };       ///< Scroll-bar thumb color
    uint32_t scrollbarWidth = 16;

    float fieldsetControl[4]{ 213.f/255.f,223.f/255.f,229.f/255.f, 1.f };      ///< Fieldset decoration color
    float fieldsetText[4]{ 100.f/255.f,103.f/255.f,105.f/255.f, 1.f };         ///< Fieldset text color
    controls::FieldsetStyle fieldsetStyle = controls::FieldsetStyle::classic;
    
    float textBoxControl[4]{ 240.f/255.f,240.f/255.f,240.f/255.f, 1.f };       ///< Text-box background color
    float textBoxText[4]{ 112.f/255.f,112.f/255.f,112.f/255.f, 1.f };          ///< Text-box text color
    
    float buttonControl[4]{ 138.f/255.f,174.f/255.f,208.f/255.f, 1.f };        ///< Regular button background color
    float buttonText[4]{ 67.f/255.f,82.f/255.f,97.f/255.f, 1.f };           ///< Regular button text color
    float specialButtonControl[4]{ 74.f/255.f,120.f/255.f,166.f/255.f, 1.f }; ///< Special button background color
    float specialButtonText[4]{ 63.f/255.f,81.f/255.f,98.f/255.f, 1.f };    ///< Special button text color
    
    float comboBoxControl[4]{ 138.f/255.f,174.f/255.f,208.f/255.f, 1.f };      ///< Combo-box control background color
    float comboBoxDropdown[4]{ 233.f/255.f,239.f/255.f,244.f/255.f, 1.f };     ///< Combo-box drop-down background color
    float comboBoxText[4]{ 67.f/255.f,82.f/255.f,97.f/255.f, 1.f };         ///< Combo-box control text color
    float comboBoxDropdownText[4]{ 88.f/255.f,100.f/255.f,116.f/255.f, 1.f }; ///< Combo-box drop-down text color
    
    float rulerControl[4]{ 138.f/255.f,174.f/255.f,208.f/255.f, 1.f };         ///< Sliding-ruler background color
    float rulerBorder[4]{ 88.f/255.f,100.f/255.f,116.f/255.f, 1.f };          ///< Sliding-ruler border color
    float rulerThumb[4]{ 63.f/255.f,81.f/255.f,98.f/255.f, 1.f };           ///< Sliding-ruler thumb color
    
    float sliderArrows[4]{ 74.f/255.f,120.f/255.f,166.f/255.f, 1.f };         ///< Slider-box arrow color
    float sliderText[4]{ 67.f/255.f,82.f/255.f,97.f/255.f, 1.f };           ///< Slider-box text color
    
    // ---
    
    float tabControlTabs[4];       ///< Tab-control tab background color
    float tabControlBar[4];        ///< Tab-control bar color
    float tabControlActiveBar[4];  ///< Tab-control bar active area color
    float tabControlText[4];       ///< Tab-control tab text color
    float tabControlActiveText[4]; ///< Tab-control active/hover tab text color
    
    float verticalTabBar[4];       ///< Vertical tab-control bar color
    float verticalTabBorder[4];    ///< Vertical tab-control border color
    float verticalTabText[4];      ///< Vertical tab-control tab text color
    float verticalTabActiveText[4];///< Vertical tab-control active/hover tab text color
  };
}
