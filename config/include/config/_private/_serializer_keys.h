/*******************************************************************************
Pandora GS - PSEmu-compatible GPU driver
Copyright (C) 2021  Romain Vinders

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation, version 2
of the License.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details (LICENSE file).
*******************************************************************************/
#pragma once

// -- JSON key values -- -------------------------------------------------------

namespace config {
  namespace video {
    constexpr const char* api() noexcept { return "api"; }
    constexpr const char* enableVsync() noexcept { return "vsync"; }
    constexpr const char* enableFramerateLimit() noexcept { return "limit_on"; }
    constexpr const char* framerateLimit() noexcept { return "framerate"; }
    constexpr const char* frameSkip() noexcept { return "skip"; }
    constexpr const char* precision() noexcept { return "subprec"; }
    constexpr const char* osd() noexcept { return "osd"; }
  }
  namespace window {
    constexpr const char* monitorId() noexcept { return "screen"; }
    constexpr const char* windowMode() noexcept { return "mode"; }
    constexpr const char* windowHeight() noexcept { return "win_y"; }
    constexpr const char* fullscreenResX() noexcept { return "full_x"; }
    constexpr const char* fullscreenResY() noexcept { return "full_y"; }
    constexpr const char* fullscreenRate() noexcept { return "full_rate"; }
    constexpr const char* isWideSource() noexcept { return "wide_source"; }
  }
  namespace actions {
    constexpr const char* keyboardMapping() noexcept { return "key"; }
    constexpr const char* controllerMapping() noexcept { return "ctl"; }
    constexpr const char* controllerHotkey() noexcept { return "ctl_hk"; }
    constexpr const char* hintMenuOnMouseMove() noexcept { return "menu_hint"; }
  }
  namespace profile {
    constexpr const char* _array() noexcept { return "profiles"; }
    constexpr const char* id() noexcept { return "id"; }
    constexpr const char* file() noexcept { return "file"; }
    constexpr const char* name() noexcept { return "name"; }
    constexpr const char* tileColor() noexcept { return "color"; }

    namespace renderer {
      constexpr const char* screenStretching() noexcept { return "stretch"; }
      constexpr const char* screenCropping() noexcept { return "crop"; }
      constexpr const char* isPalRecentered() noexcept { return "pal_pos"; }
      constexpr const char* isOverscanVisible() noexcept { return "ovscan"; }
      constexpr const char* isMirrored() noexcept { return "mirror"; }
      constexpr const char* screenCurvature() noexcept { return "curved"; }
      constexpr const char* blackBorderSizes() noexcept { return "border"; }
      constexpr const char* internalResFactorX() noexcept { return "ires_x"; }
      constexpr const char* internalResFactorY() noexcept { return "ires_y"; }
      constexpr const char* colorMode() noexcept { return "color"; }
      constexpr const char* fillMode() noexcept { return "wireframe"; }
      constexpr const char* antiAliasing() noexcept { return "aa"; }
      constexpr const char* textureUpscaling() noexcept { return "tx_upscale"; }
      constexpr const char* textureUpscalingFactor() noexcept { return "tx_factor"; }
      constexpr const char* useTextureBilinear() noexcept { return "tx_bilinear"; }
      constexpr const char* spriteUpscaling() noexcept { return "spr_upscale"; }
      constexpr const char* spriteUpscalingFactor() noexcept { return "spr_factor"; }
      constexpr const char* useSpriteBilinear() noexcept { return "spr_bilinear"; }
      constexpr const char* screenUpscaling() noexcept { return "dsp_upscale"; }
      constexpr const char* screenUpscalingFactor() noexcept { return "dsp_factor"; }
      constexpr const char* mdecUpscaling() noexcept { return "mdec"; }
      constexpr const char* textureGrain() noexcept { return "tx_grain"; }
      constexpr const char* screenGrain() noexcept { return "dsp_grain"; }
      constexpr const char* dithering() noexcept { return "dither"; }
      constexpr const char* useTextureDithering() noexcept { return "tx_dither"; }
      constexpr const char* useSpriteDithering() noexcept { return "spr_dither"; }
    }
  }
}
