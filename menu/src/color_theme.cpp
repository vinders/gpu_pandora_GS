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
#include "menu/color_theme.h"

using namespace video_api;
using namespace menu;

#define SET_MULTIPLIER(renderer, r,g,b,a, output) { \
          float color[4] = { r,g,b,a }; \
          renderer.sRgbToGammaCorrectColor(color, output); \
        }
#define SET_THEME_COLOR(renderer, r,g,b, opacity, output) { \
          float color[4] = { (float)r/255.f, (float)g/255.f, (float)b/255.f, opacity }; \
          renderer.sRgbToGammaCorrectColor(color, output); \
        }

// ---

void ColorTheme::updateTheme(video_api::Renderer& renderer, ColorThemeType type) {
  switch (type) {
    case ColorThemeType::lightBlue:
    default: {
      SET_MULTIPLIER(renderer, 1.f,  1.f,  1.f,  0.5f, disabledControl)
      SET_MULTIPLIER(renderer, 1.15f,1.15f,1.15f,1.f,  activeControl)
      SET_MULTIPLIER(renderer, 0.85f,0.85f,0.85f,1.f,  specialControl)
      SET_THEME_COLOR(renderer,  64, 64, 64, 1.f, regularLabel)
      SET_THEME_COLOR(renderer, 160,160,160, 1.f, disabledLabel)
      SET_THEME_COLOR(renderer,  82, 82, 82, 1.f, activeLabel)

      SET_THEME_COLOR(renderer, 240,240,240, 1.f, scrollbarControl)
      SET_THEME_COLOR(renderer, 205,205,205, 1.f, scrollbarThumb)
      SET_THEME_COLOR(renderer, 138,174,208, 0.25f, lineSelectorControl)
      SET_THEME_COLOR(renderer, 220,220,220, 1.f, tooltipControl)
      SET_THEME_COLOR(renderer,  64, 64, 64, 1.f, tooltipLabel)
      SET_THEME_COLOR(renderer,  22, 22, 22, 1.f, titleLabel)
      SET_THEME_COLOR(renderer, 213,223,229, 1.f, fieldsetControl)
      SET_THEME_COLOR(renderer, 100,103,105, 1.f, fieldsetLabel)

      SET_THEME_COLOR(renderer,   0,  0,  0, 1.f, tabControl)
      SET_THEME_COLOR(renderer, 138,174,208, 1.f, tabLine)
      SET_THEME_COLOR(renderer, 173,217,131, 1.f, tabActiveLine)
      SET_THEME_COLOR(renderer, 100,100,100, 1.f, tabLabel)
      SET_THEME_COLOR(renderer,  40, 40, 40, 1.f, tabActiveLabel)
      SET_THEME_COLOR(renderer, 220,220,220, 1.f, verticalTabControl)
      SET_THEME_COLOR(renderer, 200,200,200, 1.f, verticalTabBorder)
      SET_THEME_COLOR(renderer, 100,100,100, 1.f, verticalTabLabel)
      SET_THEME_COLOR(renderer,  40, 40, 40, 1.f, verticalTabActiveLabel)

      SET_THEME_COLOR(renderer, 138,174,208, 1.f, buttonControl)
      SET_THEME_COLOR(renderer,  67, 82, 97, 1.f, buttonLabel)
      SET_THEME_COLOR(renderer, 240,240,240, 1.f, textBoxControl)
      SET_THEME_COLOR(renderer, 112,112,112, 1.f, textBoxLabel)
      SET_THEME_COLOR(renderer, 147,184,218, 0.8f, comboBoxControl) //138,174,208
      SET_THEME_COLOR(renderer, 233,239,244, 1.f, comboBoxDropdown)
      SET_THEME_COLOR(renderer,  78, 93,109, 1.f, comboBoxLabel) //67, 82, 97  //70, 86,102
      SET_THEME_COLOR(renderer,  88,100,116, 1.f, comboBoxDropdownLabel)
      SET_THEME_COLOR(renderer, 102,102,102, 1.f, rulerControl)
      SET_THEME_COLOR(renderer,  80, 80, 80, 1.f, rulerBorder)
      SET_THEME_COLOR(renderer, 200,200,200, 1.f, rulerThumb)
      SET_THEME_COLOR(renderer,   0, 83,166, 1.f, rulerFiller)
      SET_THEME_COLOR(renderer,  74,120,166, 1.f, sliderArrow)
      SET_THEME_COLOR(renderer,  67, 82, 97, 1.f, sliderLabel)
      fieldsetType = FieldsetStyle::classic;
      break;
    }
  }
  themeType_ = type;
}