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
    static constexpr const char* enableFramerateLimit() noexcept { return "limit"; }
    static constexpr const char* framerateLimit() noexcept { return "rate"; }
    static constexpr const char* frameSkip() noexcept { return "skip"; }
    static constexpr const char* precision() noexcept { return "prec"; }
    static constexpr const char* osd() noexcept { return "osd"; }
  }
  namespace window {
    static constexpr const char* monitorId() noexcept { return "screen"; }
    static constexpr const char* windowMode() noexcept { return "mode"; }
    static constexpr const char* windowHeight() noexcept { return "win_y"; }
    static constexpr const char* fullscreenResX() noexcept { return "fs_x"; }
    static constexpr const char* fullscreenResY() noexcept { return "fs_y"; }
    static constexpr const char* fullscreenRate() noexcept { return "fs_r"; }
    static constexpr const char* isWideSource() noexcept { return "wide"; }
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
      static constexpr const char* internalResFactorX() noexcept { return "iresx"; }
      static constexpr const char* internalResFactorY() noexcept { return "iresy"; }
      static constexpr const char* colorMode() noexcept { return "bits"; }
      static constexpr const char* fillMode() noexcept { return "wireframe"; }
      static constexpr const char* antiAliasing() noexcept { return "aa"; }
      static constexpr const char* textureUpscaling() noexcept { return "tx_up"; }
      static constexpr const char* useTextureBilinear() noexcept { return "tx_bilin"; }
      static constexpr const char* spriteUpscaling() noexcept { return "spr_up"; }
      static constexpr const char* useSpriteBilinear() noexcept { return "spr_bilin"; }
      static constexpr const char* screenUpscaling() noexcept { return "scr_up"; }
      static constexpr const char* mdecUpscaling() noexcept { return "mdec"; }
    }
    namespace window {
      static constexpr const char* screenStretching() noexcept { return "stretch"; }
      static constexpr const char* screenCropping() noexcept { return "crop"; }
      static constexpr const char* screenCurvature() noexcept { return "curv"; }
      static constexpr const char* isMirrored() noexcept { return "mirror"; }
      static constexpr const char* isPalRecentered() noexcept { return "pal_pos"; }
      static constexpr const char* isOverscanVisible() noexcept { return "ovscan"; }
      static constexpr const char* blackBorderSizes() noexcept { return "border"; }
    }
    namespace effects {
      static constexpr const char* textureGrain() noexcept { return "tx_grain"; }
      static constexpr const char* screenGrain() noexcept { return "scr_grain"; }
      static constexpr const char* dithering() noexcept { return "dith"; }
      static constexpr const char* useTextureDithering() noexcept { return "tx_dith"; }
      static constexpr const char* useSpriteDithering() noexcept { return "spr_dith"; }

    }
  }
}
