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
using namespace menu::controls;

#define SET_MULTIPLIER(renderer, r,g,b,a, output) { \
          float color[4] = { r,g,b,a }; \
          memcpy(output, color, sizeof(float)*4u); \
        }
#define SET_THEME_COLOR(renderer, r,g,b, opacity, output) { \
          float color[4] = { (float)r/255.f, (float)g/255.f, (float)b/255.f, opacity }; \
          renderer.sRgbToGammaCorrectColor(color, output); \
        }

// ---

void ColorTheme::updateTheme(video_api::Renderer& renderer, ColorThemeType type) noexcept {
  switch (type) {
    case ColorThemeType::blue:
    default: {
      SET_MULTIPLIER(renderer, 0.3f,0.6f,0.8f,0.2f,   disabledControl)
      SET_MULTIPLIER(renderer, 1.35f,1.35f,1.35f,1.f, activeControl)
      SET_MULTIPLIER(renderer, 1.25f,1.25f,1.25f,1.f, activeScrollControl)
      SET_MULTIPLIER(renderer, 1.f,1.f,1.f,1.f,       regularIcon)
      SET_MULTIPLIER(renderer, 0.3f,0.6f,0.8f,0.2f,   disabledIcon)
      SET_MULTIPLIER(renderer, 1.2f,1.2f,1.2f,1.f,    activeIcon)
      SET_THEME_COLOR(renderer,  83,171,196, 1.f, regularLabel) // 105,191,222  // 84,168,192
      SET_THEME_COLOR(renderer,  57,102,123, 1.f, disabledLabel)
      SET_THEME_COLOR(renderer, 134,214,242, 1.f, activeLabel)

      SET_THEME_COLOR(renderer,  13, 39, 59, 1.f, background)
      SET_THEME_COLOR(renderer,  11, 64,102, 1.f, backgroundGradient) // 12, 67,105
      SET_THEME_COLOR(renderer,  22, 55, 80, 1.f, scrollbarControl) // 22, 60, 91  // 23, 57, 80  // 26, 60, 87
      SET_THEME_COLOR(renderer,  36, 79,112, 1.f, scrollbarThumb) // 43, 86,122
      SET_THEME_COLOR(renderer,  23, 71,104, 0.75f, lineSelectorControl)
      SET_THEME_COLOR(renderer,  22, 55, 80, 1.f, tooltipControl) // 23, 71,104
      SET_THEME_COLOR(renderer,  17,118,154, 1.f, titleLabel) // 35,116,145
      SET_THEME_COLOR(renderer,  23, 78,120, 1.f, fieldsetControl) // 27, 69,101
      SET_THEME_COLOR(renderer,  46,153,187, 1.f, fieldsetLabel)
      backgroundType = BackgroundStyle::radialGradient;
      fieldsetType = FieldsetStyle::gradientBox;

      SET_THEME_COLOR(renderer,  22, 55, 80, 1.f, tabControlColors.colors[0])
      SET_THEME_COLOR(renderer,  27, 69,101, 1.f, tabControlColors.colors[1])
      SET_THEME_COLOR(renderer,  42, 94,134, 1.f, tabControlColors.colors[2])
      SET_THEME_COLOR(renderer, 101,148,183, 1.f, tabLabel)
      SET_THEME_COLOR(renderer, 146,187,218, 1.f, tabActiveLabel)
      SET_THEME_COLOR(renderer, 220,220,220, 1.f, verticalTabControl)
      SET_THEME_COLOR(renderer, 200,200,200, 1.f, verticalTabBorder)
      SET_THEME_COLOR(renderer, 100,100,100, 1.f, verticalTabLabel)
      SET_THEME_COLOR(renderer,  40, 40, 40, 1.f, verticalTabActiveLabel)

      SET_THEME_COLOR(renderer,  44,144,185, 1.f, buttonControl)
      SET_THEME_COLOR(renderer, 180,223,247, 1.f, buttonLabel)
      SET_THEME_COLOR(renderer,  35, 81,108, 1.f, textBoxControl) // 170,223,247  // 119,192,208  // 135,199,213  // 88,156,180  // 21, 65, 96
      SET_THEME_COLOR(renderer,  96,178,203, 1.f, textBoxLabel)  // 53, 77, 93  // 41, 72, 84
      SET_THEME_COLOR(renderer,  42,121,160, 1.f, comboBoxControlColors.colors[0]) // 38,113,149
      SET_THEME_COLOR(renderer,  77,153,189, 1.f, comboBoxControlColors.colors[1]) // 70,164,197 // 74,150,186
      SET_THEME_COLOR(renderer, 150,187,216, 1.f, comboBoxControlColors.colors[2])
      SET_THEME_COLOR(renderer,  18, 54, 79, 1.f, comboBoxLabel) // 23, 59, 88  // 19, 57, 83
      SET_THEME_COLOR(renderer,  23, 53, 76, 1.f, comboBoxDropdownLabel)
      SET_THEME_COLOR(renderer,  51, 87,112, 1.f, rulerControlColors.colors[0])
      SET_THEME_COLOR(renderer,  69, 82, 92, 1.f, rulerControlColors.colors[1])
      SET_THEME_COLOR(renderer, 200,200,200, 1.f, rulerControlColors.colors[2])
      SET_THEME_COLOR(renderer,  83,171,196, 1.f, rulerControlColors.colors[3])
      SET_THEME_COLOR(renderer,  54,141,182, 1.f, sliderArrow) // 43,161,229
      break;
    }
    case ColorThemeType::green: {
      SET_MULTIPLIER(renderer, 0.5f,0.7f,0.65f,0.2f,  disabledControl)
      SET_MULTIPLIER(renderer, 1.35f,1.35f,1.35f,1.f, activeControl)
      SET_MULTIPLIER(renderer, 1.25f,1.25f,1.25f,1.f, activeScrollControl)
      SET_MULTIPLIER(renderer, 1.f,1.f,1.f,1.f,       regularIcon)
      SET_MULTIPLIER(renderer, 0.5f,0.7f,0.65f,0.2f,  disabledIcon)
      SET_MULTIPLIER(renderer, 1.2f,1.2f,1.2f,1.f,    activeIcon)
      SET_THEME_COLOR(renderer, 177,235,142, 1.f, regularLabel)
      SET_THEME_COLOR(renderer,  80,130, 68, 1.f, disabledLabel)
      SET_THEME_COLOR(renderer, 213,247,193, 1.f, activeLabel)

      SET_THEME_COLOR(renderer,  22, 65, 18, 1.f, background)
      SET_THEME_COLOR(renderer,  27, 87, 21, 1.f, backgroundGradient)
      SET_THEME_COLOR(renderer,  36, 83, 32, 1.f, scrollbarControl)
      SET_THEME_COLOR(renderer,  46,104, 41, 1.f, scrollbarThumb)
      SET_THEME_COLOR(renderer,  33, 89, 25, 0.75f, lineSelectorControl)
      SET_THEME_COLOR(renderer,  22, 86, 16, 1.f, tooltipControl)
      SET_THEME_COLOR(renderer, 193,227,166, 1.f, titleLabel)
      SET_THEME_COLOR(renderer,  43,120, 18, 1.f, fieldsetControl)
      SET_THEME_COLOR(renderer, 110,182, 82, 1.f, fieldsetLabel)
      backgroundType = BackgroundStyle::radialGradient;
      fieldsetType = FieldsetStyle::gradientBox;

      SET_THEME_COLOR(renderer,  22, 55, 80, 1.f, tabControlColors.colors[0])
      SET_THEME_COLOR(renderer,  27, 69,101, 1.f, tabControlColors.colors[1])
      SET_THEME_COLOR(renderer,  42, 94,134, 1.f, tabControlColors.colors[2])
      SET_THEME_COLOR(renderer, 101,148,183, 1.f, tabLabel)
      SET_THEME_COLOR(renderer, 146,187,218, 1.f, tabActiveLabel)
      SET_THEME_COLOR(renderer, 220,220,220, 1.f, verticalTabControl)
      SET_THEME_COLOR(renderer, 200,200,200, 1.f, verticalTabBorder)
      SET_THEME_COLOR(renderer, 100,100,100, 1.f, verticalTabLabel)
      SET_THEME_COLOR(renderer,  40, 40, 40, 1.f, verticalTabActiveLabel)

      SET_THEME_COLOR(renderer, 119,166, 12, 1.f, buttonControl)
      SET_THEME_COLOR(renderer,  41, 72,  8, 1.f, buttonLabel)
      SET_THEME_COLOR(renderer,  46,111, 40, 1.f, textBoxControl)
      SET_THEME_COLOR(renderer, 130,208,123, 1.f, textBoxLabel)
      SET_THEME_COLOR(renderer, 104,148, 27, 1.f, comboBoxControlColors.colors[0]) // 105,151,  3
      SET_THEME_COLOR(renderer, 148,198, 61, 1.f, comboBoxControlColors.colors[1]) // 151,198, 60
      SET_THEME_COLOR(renderer, 195,228,149, 1.f, comboBoxControlColors.colors[2])
      SET_THEME_COLOR(renderer,  41, 72,  8, 1.f, comboBoxLabel)
      SET_THEME_COLOR(renderer,  50, 74, 19, 1.f, comboBoxDropdownLabel)
      SET_THEME_COLOR(renderer,  44, 92, 40, 1.f, rulerControlColors.colors[0])
      SET_THEME_COLOR(renderer,  57, 88, 49, 1.f, rulerControlColors.colors[1])
      SET_THEME_COLOR(renderer, 200,200,200, 1.f, rulerControlColors.colors[2])
      SET_THEME_COLOR(renderer, 119,166, 12, 1.f, rulerControlColors.colors[3])
      SET_THEME_COLOR(renderer,  83,166, 52, 1.f, sliderArrow)
      break;
    }
    case ColorThemeType::darkGreen: {
      SET_MULTIPLIER(renderer, 0.5f,0.7f,0.65f,0.2f,  disabledControl)
      SET_MULTIPLIER(renderer, 1.35f,1.35f,1.35f,1.f, activeControl)
      SET_MULTIPLIER(renderer, 1.25f,1.25f,1.25f,1.f, activeScrollControl)
      SET_MULTIPLIER(renderer, 1.f,1.f,1.f,1.f,       regularIcon)
      SET_MULTIPLIER(renderer, 0.5f,0.7f,0.65f,0.2f,  disabledIcon)
      SET_MULTIPLIER(renderer, 1.2f,1.2f,1.2f,1.f,    activeIcon)
      SET_THEME_COLOR(renderer, 159,254,178, 1.f, regularLabel)
      SET_THEME_COLOR(renderer,  73,143, 86, 1.f, disabledLabel)
      SET_THEME_COLOR(renderer, 184,252,196, 1.f, activeLabel)

      SET_THEME_COLOR(renderer,  21, 53, 30, 1.f, background)
      SET_THEME_COLOR(renderer,  19, 75, 33, 1.f, backgroundGradient)
      SET_THEME_COLOR(renderer,  14, 54, 24, 1.f, scrollbarControl)
      SET_THEME_COLOR(renderer,  22, 93, 39, 1.f, scrollbarThumb)
      SET_THEME_COLOR(renderer,  22, 93, 39, 0.75f, lineSelectorControl)
      SET_THEME_COLOR(renderer,  28, 71, 40, 1.f, tooltipControl)
      SET_THEME_COLOR(renderer, 137,184,146, 1.f, titleLabel)
      SET_THEME_COLOR(renderer,  42,140, 70, 1.f, fieldsetControl) // 35,148, 67
      SET_THEME_COLOR(renderer,  63,200,102, 1.f, fieldsetLabel) // 60,217,105
      backgroundType = BackgroundStyle::radialGradient;
      fieldsetType = FieldsetStyle::gradientBox;

      SET_THEME_COLOR(renderer,  22, 55, 80, 1.f, tabControlColors.colors[0])
      SET_THEME_COLOR(renderer,  27, 69,101, 1.f, tabControlColors.colors[1])
      SET_THEME_COLOR(renderer,  42, 94,134, 1.f, tabControlColors.colors[2])
      SET_THEME_COLOR(renderer, 101,148,183, 1.f, tabLabel)
      SET_THEME_COLOR(renderer, 146,187,218, 1.f, tabActiveLabel)
      SET_THEME_COLOR(renderer, 220,220,220, 1.f, verticalTabControl)
      SET_THEME_COLOR(renderer, 200,200,200, 1.f, verticalTabBorder)
      SET_THEME_COLOR(renderer, 100,100,100, 1.f, verticalTabLabel)
      SET_THEME_COLOR(renderer,  40, 40, 40, 1.f, verticalTabActiveLabel)

      SET_THEME_COLOR(renderer, 119,166, 12, 1.f, buttonControl)
      SET_THEME_COLOR(renderer,  41, 72,  8, 1.f, buttonLabel)
      SET_THEME_COLOR(renderer,  29,116, 51, 1.f, textBoxControl)
      SET_THEME_COLOR(renderer, 159,255,177, 1.f, textBoxLabel)
      SET_THEME_COLOR(renderer,  44,176, 66, 1.f, comboBoxControlColors.colors[0])
      SET_THEME_COLOR(renderer,  80,198,100, 1.f, comboBoxControlColors.colors[1])
      SET_THEME_COLOR(renderer, 157,222,170, 1.f, comboBoxControlColors.colors[2])
      SET_THEME_COLOR(renderer,  18, 65, 29, 1.f, comboBoxLabel)
      SET_THEME_COLOR(renderer,  45, 86, 53, 1.f, comboBoxDropdownLabel)
      SET_THEME_COLOR(renderer,  44, 92, 40, 1.f, rulerControlColors.colors[0])
      SET_THEME_COLOR(renderer,  57, 88, 49, 1.f, rulerControlColors.colors[1])
      SET_THEME_COLOR(renderer, 200,200,200, 1.f, rulerControlColors.colors[2])
      SET_THEME_COLOR(renderer, 119,166, 12, 1.f, rulerControlColors.colors[3])
      SET_THEME_COLOR(renderer,  44,176, 66, 1.f, sliderArrow)
      break;
    }
    case ColorThemeType::yellow: {
      SET_MULTIPLIER(renderer, 0.7f,0.65f,0.5f,0.2f,  disabledControl)
      SET_MULTIPLIER(renderer, 1.35f,1.35f,1.35f,1.f, activeControl)
      SET_MULTIPLIER(renderer, 1.25f,1.25f,1.25f,1.f, activeScrollControl)
      SET_THEME_COLOR(renderer, 255, 242, 213,0.75f,  regularIcon)
      SET_THEME_COLOR(renderer, 255, 217, 159,0.2f,   disabledIcon)
      SET_MULTIPLIER(renderer, 1.2f,1.2f,1.2f,1.f,    activeIcon)
      SET_THEME_COLOR(renderer, 255,217,159, 1.f, regularLabel)
      SET_THEME_COLOR(renderer, 143,115, 73, 1.f, disabledLabel)
      SET_THEME_COLOR(renderer, 255,231,195, 1.f, activeLabel)

      SET_THEME_COLOR(renderer,  52, 43, 21, 1.f, background) // 18, 14,  5
      SET_THEME_COLOR(renderer,  74, 54, 19, 1.f, backgroundGradient) // 76, 56, 19  // 52, 43, 21
      SET_THEME_COLOR(renderer,  54, 43, 19, 1.f, scrollbarControl) // 65, 46, 14
      SET_THEME_COLOR(renderer,  74, 58, 22, 1.f, scrollbarThumb) // 114, 79, 27  // 107, 80, 27 // 82, 61, 21
      SET_THEME_COLOR(renderer,  92, 68, 22, 0.75f, lineSelectorControl)
      SET_THEME_COLOR(renderer,  71, 58, 28, 1.f, tooltipControl)
      SET_THEME_COLOR(renderer, 196,153, 84, 1.f, titleLabel)
      SET_THEME_COLOR(renderer, 148,114, 35, 1.f, fieldsetControl)
      SET_THEME_COLOR(renderer, 217,170, 60, 1.f, fieldsetLabel) // 241,158, 18  // 242,175, 17
      backgroundType = BackgroundStyle::radialGradient;
      fieldsetType = FieldsetStyle::gradientBox;

      SET_THEME_COLOR(renderer,  22, 55, 80, 1.f, tabControlColors.colors[0])
      SET_THEME_COLOR(renderer,  27, 69,101, 1.f, tabControlColors.colors[1])
      SET_THEME_COLOR(renderer,  42, 94,134, 1.f, tabControlColors.colors[2])
      SET_THEME_COLOR(renderer, 101,148,183, 1.f, tabLabel)
      SET_THEME_COLOR(renderer, 146,187,218, 1.f, tabActiveLabel)
      SET_THEME_COLOR(renderer, 220,220,220, 1.f, verticalTabControl)
      SET_THEME_COLOR(renderer, 200,200,200, 1.f, verticalTabBorder)
      SET_THEME_COLOR(renderer, 100,100,100, 1.f, verticalTabLabel)
      SET_THEME_COLOR(renderer,  40, 40, 40, 1.f, verticalTabActiveLabel)

      SET_THEME_COLOR(renderer, 191,136, 33, 1.f, buttonControl)
      SET_THEME_COLOR(renderer, 255,228,176, 1.f, buttonLabel)
      SET_THEME_COLOR(renderer, 116, 87, 29, 1.f, textBoxControl)
      SET_THEME_COLOR(renderer, 255,217,159, 1.f, textBoxLabel)
      SET_THEME_COLOR(renderer, 191,136, 33, 1.f, comboBoxControlColors.colors[0])
      SET_THEME_COLOR(renderer, 214,166, 76, 1.f, comboBoxControlColors.colors[1])
      SET_THEME_COLOR(renderer, 255,231,195, 1.f, comboBoxControlColors.colors[2])
      SET_THEME_COLOR(renderer,  64, 48, 18, 1.f, comboBoxLabel)
      SET_THEME_COLOR(renderer,  52, 43, 21, 1.f, comboBoxDropdownLabel)
      SET_THEME_COLOR(renderer,  44, 92, 40, 1.f, rulerControlColors.colors[0])
      SET_THEME_COLOR(renderer,  57, 88, 49, 1.f, rulerControlColors.colors[1])
      SET_THEME_COLOR(renderer, 200,200,200, 1.f, rulerControlColors.colors[2])
      SET_THEME_COLOR(renderer, 119,166, 12, 1.f, rulerControlColors.colors[3])
      SET_THEME_COLOR(renderer, 191,136, 33, 1.f, sliderArrow)
      break;
    }
    /*case ColorThemeType::white:
    default: {
      SET_MULTIPLIER(renderer, 1.f, 1.f, 1.f, 0.6f, disabledControl)
      SET_MULTIPLIER(renderer, 1.2f,1.2f,1.2f,1.f,  activeControl)
      SET_MULTIPLIER(renderer, 0.8f,0.8f,0.8f,1.f,  activeScrollControl)
      SET_THEME_COLOR(renderer,233, 239, 244, 1.f,  regularIcon)
      SET_MULTIPLIER(renderer, 1.f, 1.f, 1.f, 0.6f, disabledIcon)
      SET_MULTIPLIER(renderer, 1.f, 1.f, 1.f, 1.f,  activeIcon)
      SET_THEME_COLOR(renderer,  67, 86,104, 1.f, regularLabel)
      SET_THEME_COLOR(renderer, 141,162,181, 1.f, disabledLabel)
      SET_THEME_COLOR(renderer,  54, 83,110, 1.f, activeLabel)

      SET_THEME_COLOR(renderer, 255,255,255, 1.f, background)
      SET_THEME_COLOR(renderer, 240,240,240, 1.f, scrollbarControl)
      SET_THEME_COLOR(renderer, 205,205,205, 1.f, scrollbarThumb)
      SET_THEME_COLOR(renderer, 204,231,255, 0.5f, lineSelectorControl) // 138,174,208, 0.25f
      SET_THEME_COLOR(renderer, 213,223,229, 0.8f, tooltipControl)
      SET_THEME_COLOR(renderer,  90, 90, 90, 1.f, titleLabel)
      SET_THEME_COLOR(renderer, 191,223,251, 1.f, fieldsetControl)
      SET_THEME_COLOR(renderer, 110,145,177, 1.f, fieldsetLabel)
      backgroundType = BackgroundStyle::plain;
      fieldsetType = FieldsetStyle::classic;

      SET_THEME_COLOR(renderer,   0,  0,  0, 1.f, tabControlColors.colors[0])
      SET_THEME_COLOR(renderer, 138,174,208, 1.f, tabControlColors.colors[1])
      SET_THEME_COLOR(renderer, 173,217,131, 1.f, tabControlColors.colors[2])
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
      SET_THEME_COLOR(renderer, 147,184,218, 0.8f, comboBoxControlColors.colors[0])
      SET_THEME_COLOR(renderer, 174,202,228, 0.7f, comboBoxControlColors.colors[1])
      SET_THEME_COLOR(renderer, 233,239,244, 1.f, comboBoxControlColors.colors[2])
      SET_THEME_COLOR(renderer,  78, 93,109, 1.f, comboBoxLabel)
      SET_THEME_COLOR(renderer,  88,100,116, 1.f, comboBoxDropdownLabel)
      SET_THEME_COLOR(renderer, 102,102,102, 1.f, rulerControlColors.colors[0])
      SET_THEME_COLOR(renderer,  80, 80, 80, 1.f, rulerControlColors.colors[1])
      SET_THEME_COLOR(renderer, 200,200,200, 1.f, rulerControlColors.colors[2])
      SET_THEME_COLOR(renderer,   0, 83,166, 1.f, rulerControlColors.colors[3])
      SET_THEME_COLOR(renderer,  85,138,191, 1.f, sliderArrow) // 74,120,166
      break;
    }*/
  }
  themeType_ = type;
}