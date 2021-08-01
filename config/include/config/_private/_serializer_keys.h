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
  namespace renderer {
    static constexpr const char* api() noexcept { return "api"; }
    static constexpr const char* enableVsync() noexcept { return "vsync"; }
    static constexpr const char* enableFramerateLimit() noexcept { return "limit_on"; }
    static constexpr const char* framerateLimit() noexcept { return "framerate"; }
    static constexpr const char* frameSkip() noexcept { return "skip"; }
    static constexpr const char* precision() noexcept { return "subprec"; }
    static constexpr const char* osd() noexcept { return "osd"; }
  }
  namespace window {
    static constexpr const char* monitorId() noexcept { return "screen"; }
    static constexpr const char* windowMode() noexcept { return "mode"; }
    static constexpr const char* windowHeight() noexcept { return "win_y"; }
    static constexpr const char* fullscreenResX() noexcept { return "full_x"; }
    static constexpr const char* fullscreenResY() noexcept { return "full_y"; }
    static constexpr const char* fullscreenRate() noexcept { return "full_rate"; }
    static constexpr const char* isWideSource() noexcept { return "wide_source"; }
  }
  namespace actions {
    static constexpr const char* keyboardMapping() noexcept { return "key"; }
    static constexpr const char* controllerMapping() noexcept { return "ctl"; }
    static constexpr const char* controllerHotkey() noexcept { return "ctl_hk"; }
  }
  namespace profile {
    static constexpr const char* _array() noexcept { return "profiles"; }
    static constexpr const char* id() noexcept { return "id"; }
    static constexpr const char* file() noexcept { return "file"; }
    static constexpr const char* name() noexcept { return "name"; }
    static constexpr const char* tileColor() noexcept { return "color"; }

    namespace renderer {
      static constexpr const char* internalResFactorX() noexcept { return "ires_x"; }
      static constexpr const char* internalResFactorY() noexcept { return "ires_y"; }
      static constexpr const char* colorMode() noexcept { return "color"; }
      static constexpr const char* fillMode() noexcept { return "wireframe"; }
      static constexpr const char* antiAliasing() noexcept { return "aa"; }
      static constexpr const char* textureUpscaling() noexcept { return "tx_upscale"; }
      static constexpr const char* textureUpscalingFactor() noexcept { return "tx_factor"; }
      static constexpr const char* useTextureBilinear() noexcept { return "tx_bilinear"; }
      static constexpr const char* spriteUpscaling() noexcept { return "spr_upscale"; }
      static constexpr const char* spriteUpscalingFactor() noexcept { return "spr_factor"; }
      static constexpr const char* useSpriteBilinear() noexcept { return "spr_bilinear"; }
      static constexpr const char* screenUpscaling() noexcept { return "dsp_upscale"; }
      static constexpr const char* screenUpscalingFactor() noexcept { return "dsp_factor"; }
      static constexpr const char* mdecUpscaling() noexcept { return "mdec"; }
      static constexpr const char* isPalRecentered() noexcept { return "pal_pos"; }
      static constexpr const char* isOverscanVisible() noexcept { return "ovscan"; }
      static constexpr const char* isMirrored() noexcept { return "mirror"; }
      static constexpr const char* screenCurvature() noexcept { return "curved"; }
      static constexpr const char* blackBorderSizes() noexcept { return "border"; }
    }
    namespace window {
      static constexpr const char* screenStretching() noexcept { return "stretch"; }
      static constexpr const char* screenCropping() noexcept { return "crop"; }
    }
    namespace effects {
      static constexpr const char* textureGrain() noexcept { return "tx_grain"; }
      static constexpr const char* screenGrain() noexcept { return "dsp_grain"; }
      static constexpr const char* dithering() noexcept { return "dither"; }
      static constexpr const char* useTextureDithering() noexcept { return "tx_dither"; }
      static constexpr const char* useSpriteDithering() noexcept { return "spr_dither"; }

    }
  }
}
