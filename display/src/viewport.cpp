/*******************************************************************************
Pandora GS - PSEmu-compatible GPU driver
Copyright (C) 2021  Romain Vinders

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation, version 2 of the License.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details (LICENSE file).
*******************************************************************************/
#include <cassert>
#include "display/viewport.h"

using namespace display;


static inline double __getSourceRatio(bool isWideSource, uint32_t& outMinWindowWidth) {
  if (isWideSource) {
    outMinWindowWidth = __MIN_WINDOW_HEIGHT*16/9;
    return 16./9.;
  }
  else {
    outMinWindowWidth = __MIN_WINDOW_HEIGHT*4/3;
    return 4./3.;
  }
}

// ---

// Compute fullscreen viewport
Viewport::Viewport(const pandora::hardware::DisplayMode& resolution, uint32_t stretching,
                   uint32_t cropping, bool isWideSource) noexcept {
  double sourceRatio = __getSourceRatio(isWideSource, _minWindowWidth);
  uint32_t sourceWidth = static_cast<uint32_t>((double)resolution.height * sourceRatio + 0.500001); // round

  if (sourceWidth != resolution.width) {
    if (resolution.width > sourceWidth) { // wide monitor (16:9, 16:10, 21:9...)
      uint32_t croppedHeight;
      uint32_t widthAfterCrop = sourceWidth;

      // only compute non-zero cropping (avoid div by 0)
      if (cropping) {
        // cropping: width_diff * crop_stretch_ratio * cropping_factor/max_value
        //         = (fullWidth - sourceWidth) * (cropping_1_to_8 / (cropping_1_to_8 + stretching_0_to_8)) * cropping_1_to_8/8
        //         = (fullWidth - sourceWidth) * cropping_1_to_8 * cropping_1_to_8 / ((cropping_1_to_8 + stretching_0_to_8) * 8)
        widthAfterCrop += (resolution.width - sourceWidth) * cropping * cropping
                          / ((cropping + stretching) << 3);
        croppedHeight = static_cast<uint32_t>((double)widthAfterCrop / sourceRatio);
      }
      else
        croppedHeight = resolution.height;

      // stretched: widthAfterCrop + (remaining_width_diff * stretching_factor/max_value)
      //          = widthAfterCrop + (fullWidth - widthAfterCrop) * stretching_0_to_8/8
      _scaledSourceWidth = widthAfterCrop + (((resolution.width - widthAfterCrop) * stretching) >> 3);
      _scaledSourceHeight = croppedHeight;
    }

    else { // tall monitor (5:4...): reversed dimension calculations
      uint32_t sourceHeight = static_cast<uint32_t>((double)resolution.width / sourceRatio);
      uint32_t croppedWidth;
      uint32_t heightAfterCrop = sourceHeight;

      if (cropping) {
        heightAfterCrop += (resolution.height - sourceHeight) * cropping * cropping
                           / ((cropping + stretching) << 3);
        croppedWidth = static_cast<uint32_t>((double)heightAfterCrop * sourceRatio + 0.500001);
      }
      else
        croppedWidth = resolution.width;

      _scaledSourceWidth = croppedWidth;
      _scaledSourceHeight = heightAfterCrop + (((resolution.height - heightAfterCrop) * stretching) >> 3);
    }
  }
  else {
    _scaledSourceWidth = resolution.width;
    _scaledSourceHeight = resolution.height;
  }
}

// Compute window mode viewport
Viewport::Viewport(uint32_t clientHeight, bool isWideSource) noexcept {
  double sourceRatio = __getSourceRatio(isWideSource, _minWindowWidth);
  _scaledSourceWidth = static_cast<uint32_t>((double)clientHeight * sourceRatio + 0.500001); // round
  _scaledSourceHeight = clientHeight;
}
