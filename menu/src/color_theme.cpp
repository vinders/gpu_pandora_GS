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
#include <cstring>
#include <display/video_api.h>
#include "menu/color_theme.h"

using namespace video_api;
using namespace menu;
using namespace menu::controls;

#define SET_MULTIPLIER(r,g,b,a, output) { \
          float color[4] = { r,g,b,a }; \
          memcpy(output, color, sizeof(float)*4u); \
        }
#define SET_THEME_COLOR(r,g,b, opacity, output) { \
          float color[4] = { (float)r/255.f, (float)g/255.f, (float)b/255.f, opacity }; \
          video_api::Renderer::sRgbToGammaCorrectColor(color, output); \
        }

// ---

void ColorTheme::updateTheme(ColorThemeType type) noexcept {
  switch (type) {
    case ColorThemeType::blue:
    default: {
      SET_MULTIPLIER(0.3f,0.6f,0.8f,0.2f,   disabledControl)
      SET_MULTIPLIER(1.35f,1.35f,1.35f,1.f, activeControl)
      SET_MULTIPLIER(1.25f,1.25f,1.25f,1.f, activeScrollControl)
      SET_THEME_COLOR( 44, 72, 93, 1.f,     coloredIcon) // 152,200,235  // 147,194,229 // 181,210,231
      SET_THEME_COLOR( 44, 72, 93, 0.2f,    disabledIcon) // 44, 72, 93
      SET_THEME_COLOR(147,194,229, 1.2f,    activeIcon) // 160,205,239
      SET_THEME_COLOR( 83,171,196, 1.f, regularLabel) // 105,191,222  // 84,168,192
      SET_THEME_COLOR( 57,102,123, 1.f, disabledLabel)
      SET_THEME_COLOR(134,214,242, 1.f, activeLabel)
      SET_THEME_COLOR(  0, 14, 20, 0.75f,tileLabel)
      SET_THEME_COLOR(  0,  0,  0, 0.675f, activeTileLabel)
      SET_THEME_COLOR(255,255,255, 0.65f, selectedTileLabel)

      SET_THEME_COLOR( 13, 39, 59, 1.f, background)
      SET_THEME_COLOR( 11, 64,102, 1.f, backgroundGradient) // 12, 67,105
      SET_THEME_COLOR( 22, 55, 80, 1.f, scrollbarControl) // 22, 60, 91  // 23, 57, 80  // 26, 60, 87
      SET_THEME_COLOR( 36, 79,112, 1.f, scrollbarThumb) // 43, 86,122
      SET_THEME_COLOR( 23, 71,104, 0.75f, lineSelectorControl)
      SET_THEME_COLOR( 22, 55, 80, 1.f, tooltipControl) // 23, 71,104
      SET_THEME_COLOR( 17,118,154, 1.f, titleLabel) // 35,116,145
      SET_THEME_COLOR( 23, 78,120, 1.f, fieldsetControl) // 27, 69,101
      SET_THEME_COLOR( 46,153,187, 1.f, fieldsetLabel)
      SET_THEME_COLOR( 25, 72,108, 1.f, popupTitleBar)

      SET_THEME_COLOR( 16, 44, 66, 1.f, tabControlColors.colors[0]) // 22, 55, 80
      SET_THEME_COLOR( 27, 69,101, 1.f, tabControlColors.colors[1])
      SET_THEME_COLOR( 42, 94,134, 1.f, tabControlColors.colors[2])
      SET_THEME_COLOR(101,148,183, 1.f, tabLabel)
      SET_THEME_COLOR(146,187,218, 1.f, tabActiveLabel)
      SET_THEME_COLOR( 28, 77,113, 0.1f, verticalTabControl)
      SET_THEME_COLOR( 38, 88,124, 0.15f, verticalTabBorder)
      SET_THEME_COLOR( 35,111,145, 1.f, verticalTabLabel) // 34,102,135  // 35,106,140
      SET_THEME_COLOR( 80,162,189, 1.f, verticalTabActiveLabel) // 120,187,206 // 76,158,185
      SET_THEME_COLOR( 30, 93,124, 1.1f, verticalTabIcon)

      SET_THEME_COLOR( 37,110,145, 1.f, buttonReference)
      SET_THEME_COLOR( 30, 93,124, 1.f, buttonControl)
      SET_THEME_COLOR( 34,114,153, 1.f, buttonBorder)
      SET_THEME_COLOR(180,223,247, 1.f, buttonLabel)
      SET_THEME_COLOR( 35, 81,108, 1.f, textBoxControl) // 170,223,247  // 119,192,208  // 135,199,213  // 88,156,180  // 21, 65, 96
      SET_THEME_COLOR( 96,178,203, 1.f, textBoxLabel)  // 53, 77, 93  // 41, 72, 84
      SET_THEME_COLOR( 42,121,160, 1.f, comboBoxControlColors.colors[0]) // 38,113,149
      SET_THEME_COLOR( 77,153,189, 1.f, comboBoxControlColors.colors[1]) // 70,164,197 // 74,150,186
      SET_THEME_COLOR(150,187,216, 1.f, comboBoxControlColors.colors[2])
      SET_THEME_COLOR( 18, 54, 79, 1.f, comboBoxLabel) // 23, 59, 88  // 19, 57, 83
      SET_THEME_COLOR( 23, 53, 76, 1.f, comboBoxDropdownLabel)
      SET_THEME_COLOR( 51, 87,112, 1.f, rulerControlColors.colors[0])
      SET_THEME_COLOR( 63, 85,102, 1.f, rulerControlColors.colors[1]) // 69, 82, 92
      SET_THEME_COLOR(200,200,200, 1.f, rulerControlColors.colors[2])
      SET_THEME_COLOR( 71,143,179, 1.f, rulerControlColors.colors[3]) // 83,171,196 // 72,147,179
      SET_THEME_COLOR( 54,141,182, 1.f, sliderArrow) // 43,161,229

      SET_THEME_COLOR(111,128,140, 1.f, keyboardKeyControlColors.colors[0])
      SET_THEME_COLOR(182,182,182, 1.f, keyboardKeyControlColors.colors[1])
      SET_THEME_COLOR(215,215,215, 1.f, keyboardKeyControlColors.colors[2])
      SET_THEME_COLOR(144,144,144, 1.f, keyboardKeyControlColors.colors[3])
      SET_THEME_COLOR(220,220,220, 1.f, keyboardKeyControlColors.colors[4])
      SET_THEME_COLOR(240,240,240, 1.f, keyboardKeyControlColors.colors[5])
      SET_THEME_COLOR(210,210,210, 1.f, keyboardKeyControlColors.colors[6])
      memcpy(tileColors.colors[(uint32_t)TileColors::themeColor], comboBoxControlColors.colors[0], sizeof(float)*4u);
      break;
    }
    case ColorThemeType::green: {
      SET_MULTIPLIER(0.5f,0.7f,0.65f,0.2f,  disabledControl)
      SET_MULTIPLIER(1.35f,1.35f,1.35f,1.f, activeControl)
      SET_MULTIPLIER(1.25f,1.25f,1.25f,1.f, activeScrollControl)
      SET_THEME_COLOR( 47, 92, 43, 1.f,     coloredIcon)
      SET_THEME_COLOR( 47, 92, 43, 0.2f,    disabledIcon)
      SET_THEME_COLOR(154,229,147, 1.2f,    activeIcon)
      SET_THEME_COLOR(177,235,142, 1.f, regularLabel)
      SET_THEME_COLOR( 80,130, 68, 1.f, disabledLabel)
      SET_THEME_COLOR(213,247,193, 1.f, activeLabel)
      SET_THEME_COLOR(  0, 14, 20, 0.75f,tileLabel)
      SET_THEME_COLOR(  0,  0,  0, 0.675f, activeTileLabel)
      SET_THEME_COLOR(255,255,255, 0.65f, selectedTileLabel)

      SET_THEME_COLOR( 22, 65, 18, 1.f, background)
      SET_THEME_COLOR( 27, 87, 21, 1.f, backgroundGradient)
      SET_THEME_COLOR( 36, 83, 32, 1.f, scrollbarControl)
      SET_THEME_COLOR( 46,104, 41, 1.f, scrollbarThumb)
      SET_THEME_COLOR( 33, 89, 25, 0.75f, lineSelectorControl)
      SET_THEME_COLOR( 22, 86, 16, 1.f, tooltipControl)
      SET_THEME_COLOR(193,227,166, 1.f, titleLabel)
      SET_THEME_COLOR( 43,120, 18, 1.f, fieldsetControl)
      SET_THEME_COLOR(110,182, 82, 1.f, fieldsetLabel)
      SET_THEME_COLOR( 36,105, 25, 1.f, popupTitleBar)

      SET_THEME_COLOR( 22, 55, 80, 1.f, tabControlColors.colors[0])
      SET_THEME_COLOR( 27, 69,101, 1.f, tabControlColors.colors[1])
      SET_THEME_COLOR( 42, 94,134, 1.f, tabControlColors.colors[2])
      SET_THEME_COLOR(101,148,183, 1.f, tabLabel)
      SET_THEME_COLOR(146,187,218, 1.f, tabActiveLabel)
      SET_THEME_COLOR( 26, 68, 22, 0.25f, verticalTabControl)
      SET_THEME_COLOR( 35, 82, 31, 0.35f, verticalTabBorder)
      SET_THEME_COLOR( 84,148, 74, 1.f, verticalTabLabel)
      SET_THEME_COLOR(135,218,122, 1.f, verticalTabActiveLabel)
      SET_THEME_COLOR( 95,171, 84, 1.f, verticalTabIcon) // 68,128, 58

      SET_THEME_COLOR( 53,111, 32, 1.f, buttonReference) // 89,124, 30  // 54,115, 32
      SET_THEME_COLOR( 53,111, 32, 1.f, buttonControl)
      SET_THEME_COLOR( 66,136, 41, 1.f, buttonBorder) // 109,153, 34  // 70,144, 43
      SET_THEME_COLOR(221,247,180, 1.f, buttonLabel)
      SET_THEME_COLOR( 46,111, 40, 1.f, textBoxControl)
      SET_THEME_COLOR(130,208,123, 1.f, textBoxLabel)
      SET_THEME_COLOR(104,148, 27, 1.f, comboBoxControlColors.colors[0]) // 105,151,  3
      SET_THEME_COLOR(148,198, 61, 1.f, comboBoxControlColors.colors[1]) // 151,198, 60
      SET_THEME_COLOR(195,228,149, 1.f, comboBoxControlColors.colors[2])
      SET_THEME_COLOR( 41, 72,  8, 1.f, comboBoxLabel)
      SET_THEME_COLOR( 50, 74, 19, 1.f, comboBoxDropdownLabel)
      SET_THEME_COLOR( 44, 92, 40, 1.f, rulerControlColors.colors[0])
      SET_THEME_COLOR( 57, 88, 49, 1.f, rulerControlColors.colors[1])
      SET_THEME_COLOR(200,200,200, 1.f, rulerControlColors.colors[2])
      SET_THEME_COLOR(119,166, 12, 1.f, rulerControlColors.colors[3])
      SET_THEME_COLOR( 83,166, 52, 1.f, sliderArrow)

      SET_THEME_COLOR(130,130,130, 1.f, keyboardKeyControlColors.colors[0])
      SET_THEME_COLOR(192,192,192, 1.f, keyboardKeyControlColors.colors[1])
      SET_THEME_COLOR(230,230,230, 1.f, keyboardKeyControlColors.colors[2])
      SET_THEME_COLOR(160,160,160, 1.f, keyboardKeyControlColors.colors[3])
      SET_THEME_COLOR(220,220,220, 1.f, keyboardKeyControlColors.colors[4])
      SET_THEME_COLOR(240,240,240, 1.f, keyboardKeyControlColors.colors[5])
      SET_THEME_COLOR(210,210,210, 1.f, keyboardKeyControlColors.colors[6])
      memcpy(tileColors.colors[(uint32_t)TileColors::themeColor], comboBoxControlColors.colors[0], sizeof(float)*4u);
      break;
    }
    case ColorThemeType::scifi: {
      SET_MULTIPLIER(0.5f,0.7f,0.65f,0.2f,  disabledControl)
      SET_MULTIPLIER(1.35f,1.35f,1.35f,1.f, activeControl)
      SET_MULTIPLIER(1.25f,1.25f,1.25f,1.f, activeScrollControl)
      SET_THEME_COLOR( 44, 92, 56, 1.f,     coloredIcon)
      SET_THEME_COLOR( 44, 92, 56, 0.2f,    disabledIcon)
      SET_THEME_COLOR(147,229,168, 1.2f,    activeIcon)
      SET_THEME_COLOR(159,254,178, 1.f, regularLabel)
      SET_THEME_COLOR( 73,143, 86, 1.f, disabledLabel)
      SET_THEME_COLOR(184,252,196, 1.f, activeLabel)
      SET_THEME_COLOR(  0, 14, 20, 0.75f,tileLabel)
      SET_THEME_COLOR(  0,  0,  0, 0.675f, activeTileLabel)
      SET_THEME_COLOR(255,255,255, 0.65f, selectedTileLabel)

      SET_THEME_COLOR( 21, 53, 30, 1.f, background)
      SET_THEME_COLOR( 19, 75, 33, 1.f, backgroundGradient)
      SET_THEME_COLOR( 14, 54, 24, 1.f, scrollbarControl)
      SET_THEME_COLOR( 22, 93, 39, 1.f, scrollbarThumb)
      SET_THEME_COLOR( 22, 93, 39, 0.75f, lineSelectorControl)
      SET_THEME_COLOR( 28, 71, 40, 1.f, tooltipControl)
      SET_THEME_COLOR(137,184,146, 1.f, titleLabel)
      SET_THEME_COLOR( 42,140, 70, 1.f, fieldsetControl) // 35,148, 67
      SET_THEME_COLOR( 63,200,102, 1.f, fieldsetLabel) // 60,217,105
      SET_THEME_COLOR( 25,105, 45, 1.f, popupTitleBar)

      SET_THEME_COLOR( 22, 55, 80, 1.f, tabControlColors.colors[0])
      SET_THEME_COLOR( 27, 69,101, 1.f, tabControlColors.colors[1])
      SET_THEME_COLOR( 42, 94,134, 1.f, tabControlColors.colors[2])
      SET_THEME_COLOR(101,148,183, 1.f, tabLabel)
      SET_THEME_COLOR(146,187,218, 1.f, tabActiveLabel)
      SET_THEME_COLOR( 20, 53, 29, 0.5f, verticalTabControl)
      SET_THEME_COLOR( 28, 66, 38, 0.65f, verticalTabBorder)
      SET_THEME_COLOR( 45,133, 68, 1.f, verticalTabLabel)
      SET_THEME_COLOR( 62,177,116, 1.f, verticalTabActiveLabel)
      SET_THEME_COLOR( 26, 86, 42, 1.f, verticalTabIcon)

      SET_THEME_COLOR( 40,111, 59, 1.f, buttonReference)
      SET_THEME_COLOR( 40,111, 59, 1.f, buttonControl)
      SET_THEME_COLOR( 38,148, 67, 1.f, buttonBorder)
      SET_THEME_COLOR(178,247,197, 1.f, buttonLabel)
      SET_THEME_COLOR( 29,116, 51, 1.f, textBoxControl)
      SET_THEME_COLOR(159,255,177, 1.f, textBoxLabel)
      SET_THEME_COLOR( 41,175, 77, 1.f, comboBoxControlColors.colors[0]) // 44,176, 66
      SET_THEME_COLOR( 73,194,106, 1.f, comboBoxControlColors.colors[1]) // 80,198,100
      SET_THEME_COLOR(150,210,166, 1.f, comboBoxControlColors.colors[2]) // 157,222,170
      SET_THEME_COLOR( 22, 71, 35, 1.f, comboBoxLabel) // 18, 65, 29
      SET_THEME_COLOR( 45, 87, 56, 1.f, comboBoxDropdownLabel) // 45, 86, 53
      SET_THEME_COLOR( 51,112, 66, 1.f, rulerControlColors.colors[0])
      SET_THEME_COLOR( 63,102, 74, 1.f, rulerControlColors.colors[1])
      SET_THEME_COLOR(200,200,200, 1.f, rulerControlColors.colors[2])
      SET_THEME_COLOR( 71,179, 93, 1.f, rulerControlColors.colors[3])
      SET_THEME_COLOR( 48,171, 81, 1.f, sliderArrow) // 44,176, 66

      SET_THEME_COLOR(130,130,130, 1.f, keyboardKeyControlColors.colors[0])
      SET_THEME_COLOR(192,192,192, 1.f, keyboardKeyControlColors.colors[1])
      SET_THEME_COLOR(230,230,230, 1.f, keyboardKeyControlColors.colors[2])
      SET_THEME_COLOR(160,160,160, 1.f, keyboardKeyControlColors.colors[3])
      SET_THEME_COLOR(220,220,220, 1.f, keyboardKeyControlColors.colors[4])
      SET_THEME_COLOR(240,240,240, 1.f, keyboardKeyControlColors.colors[5])
      SET_THEME_COLOR(210,210,210, 1.f, keyboardKeyControlColors.colors[6])
      SET_THEME_COLOR( 58,150, 83, 1.f, tileColors.colors[(uint32_t)TileColors::themeColor])
      break;
    }
    case ColorThemeType::gold: {
      SET_MULTIPLIER(0.7f,0.65f,0.5f,0.2f,  disabledControl)
      SET_MULTIPLIER(1.35f,1.35f,1.35f,1.f, activeControl)
      SET_MULTIPLIER(1.25f,1.25f,1.25f,1.f, activeScrollControl)
      SET_THEME_COLOR( 92, 77, 44, 1.f,     coloredIcon)
      SET_THEME_COLOR( 92, 77, 44, 0.2f,    disabledIcon)
      SET_THEME_COLOR(229,205,147, 1.15f,   activeIcon)
      SET_THEME_COLOR(241,212,146, 1.f, regularLabel) // 255,217,159
      SET_THEME_COLOR(143,115, 73, 1.f, disabledLabel)
      SET_THEME_COLOR(255,231,195, 1.f, activeLabel)
      SET_THEME_COLOR(  0, 14, 20, 0.75f,tileLabel)
      SET_THEME_COLOR(  0,  0,  0, 0.675f, activeTileLabel)
      SET_THEME_COLOR(255,255,255, 0.65f, selectedTileLabel)

      SET_THEME_COLOR( 49, 41, 22, 1.f, background) // 18, 14,  5  // 52, 43, 21
      SET_THEME_COLOR( 95, 69,  8, 1.f, backgroundGradient) // 76, 56, 19  // 52, 43, 21 // 74, 54, 19  // 92, 65,  3
      SET_THEME_COLOR( 53, 43, 22, 1.f, scrollbarControl) // 65, 46, 14  // 54, 43, 19
      SET_THEME_COLOR( 72, 57, 23, 1.f, scrollbarThumb) // 114, 79, 27  // 107, 80, 27 // 82, 61, 21  // 74, 58, 22
      SET_THEME_COLOR( 92, 68, 22, 0.75f, lineSelectorControl)
      SET_THEME_COLOR( 66, 57, 35, 1.f, tooltipControl) // 71, 58, 28
      SET_THEME_COLOR(196,153, 84, 1.f, titleLabel)
      SET_THEME_COLOR(148,114, 35, 1.f, fieldsetControl)
      SET_THEME_COLOR(217,170, 60, 1.f, fieldsetLabel) // 241,158, 18  // 242,175, 17
      SET_THEME_COLOR(108, 83, 25, 1.f, popupTitleBar)

      SET_THEME_COLOR( 22, 55, 80, 1.f, tabControlColors.colors[0])
      SET_THEME_COLOR( 27, 69,101, 1.f, tabControlColors.colors[1])
      SET_THEME_COLOR( 42, 94,134, 1.f, tabControlColors.colors[2])
      SET_THEME_COLOR(101,148,183, 1.f, tabLabel)
      SET_THEME_COLOR(146,187,218, 1.f, tabActiveLabel)
      SET_THEME_COLOR( 49, 47, 41, 0.15f,verticalTabControl) // 91, 67, 22  // 59, 56, 49
      SET_THEME_COLOR( 59, 56, 49, 0.3f, verticalTabBorder)
      SET_THEME_COLOR(131,111, 68, 1.f, verticalTabLabel) // 130,108, 65  // 126,105, 64
      SET_THEME_COLOR(171,154,108, 1.f, verticalTabActiveLabel) // 150,131, 83  // 166,148,102
      SET_THEME_COLOR(153,128, 78, 1.05f, verticalTabIcon) // 141,108, 45  // 153,120, 55, 1.05f

      SET_THEME_COLOR(191,136, 33, 1.f, buttonReference)
      SET_THEME_COLOR( 98, 76, 36, 0.6f, buttonControl) // 191,136, 33 // 165,118, 29  // 141,108, 45  // 119, 95, 52, 0.6f
      SET_THEME_COLOR(150,109, 32, 1.f, buttonBorder) // 191,136, 33
      SET_THEME_COLOR(255,228,176, 1.f, buttonLabel)
      SET_THEME_COLOR(116, 87, 29, 1.f, textBoxControl)
      SET_THEME_COLOR(255,217,159, 1.f, textBoxLabel)
      SET_THEME_COLOR(191,136, 33, 1.f, comboBoxControlColors.colors[0])
      SET_THEME_COLOR(214,166, 76, 1.f, comboBoxControlColors.colors[1])
      SET_THEME_COLOR(255,231,195, 1.f, comboBoxControlColors.colors[2])
      SET_THEME_COLOR( 64, 48, 18, 1.f, comboBoxLabel)
      SET_THEME_COLOR( 52, 43, 21, 1.f, comboBoxDropdownLabel)
      SET_THEME_COLOR(112, 93, 51, 1.f, rulerControlColors.colors[0])
      SET_THEME_COLOR(102, 91, 63, 1.f, rulerControlColors.colors[1])
      SET_THEME_COLOR(200,200,200, 1.f, rulerControlColors.colors[2])
      SET_THEME_COLOR(179,137, 71, 1.f, rulerControlColors.colors[3])
      SET_THEME_COLOR(191,136, 33, 1.f, sliderArrow)

      SET_THEME_COLOR(130,130,130, 1.f, keyboardKeyControlColors.colors[0])
      SET_THEME_COLOR(192,192,192, 1.f, keyboardKeyControlColors.colors[1])
      SET_THEME_COLOR(230,230,230, 1.f, keyboardKeyControlColors.colors[2])
      SET_THEME_COLOR(160,160,160, 1.f, keyboardKeyControlColors.colors[3])
      SET_THEME_COLOR(220,220,220, 1.f, keyboardKeyControlColors.colors[4])
      SET_THEME_COLOR(240,240,240, 1.f, keyboardKeyControlColors.colors[5])
      SET_THEME_COLOR(210,210,210, 1.f, keyboardKeyControlColors.colors[6])
      SET_THEME_COLOR(179,129,30, 1.f, tileColors.colors[(uint32_t)TileColors::themeColor])
      break;
    }
  }

  SET_THEME_COLOR(161, 45, 53, 1.f, tileColors.colors[(uint32_t)TileColors::red])
  SET_THEME_COLOR(172, 89, 39, 1.f, tileColors.colors[(uint32_t)TileColors::orange])
  SET_THEME_COLOR(178,125, 30, 1.f, tileColors.colors[(uint32_t)TileColors::yellow]) // 183,130, 31
  SET_THEME_COLOR( 89,127, 20, 1.f, tileColors.colors[(uint32_t)TileColors::apple])
  SET_THEME_COLOR( 37,121, 59, 1.f, tileColors.colors[(uint32_t)TileColors::green]) // 23,105, 45
  SET_THEME_COLOR( 33,117, 98, 1.f, tileColors.colors[(uint32_t)TileColors::teal]) // 30,111, 93
  SET_THEME_COLOR( 30,124,141, 1.f, tileColors.colors[(uint32_t)TileColors::cyan])
  SET_THEME_COLOR( 29, 98,151, 1.f, tileColors.colors[(uint32_t)TileColors::blue])
  SET_THEME_COLOR(102, 66,159, 1.f, tileColors.colors[(uint32_t)TileColors::purple]) // 95, 62,156
  SET_THEME_COLOR(125, 58,142, 1.f, tileColors.colors[(uint32_t)TileColors::violet]) // 119, 51,133
  SET_THEME_COLOR(150, 57,114, 1.f, tileColors.colors[(uint32_t)TileColors::pink]) // 151, 57,117
  SET_THEME_COLOR( 94, 94, 94, 1.f, tileColors.colors[(uint32_t)TileColors::gray])
  themeType_ = type;
}