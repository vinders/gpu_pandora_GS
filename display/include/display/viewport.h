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
#pragma once

#include <cstdint>
#include <hardware/display_monitor.h>

#define __MIN_WINDOW_HEIGHT 480

namespace display {
  /// @brief Viewport size for render target image
  /// @remarks - If greater than window size, will be cropped
  ///          - If smaller than window size, black bars will be added
  class Viewport final {
  public:
    /// @brief Compute fullscreen viewport, based on stretching/cropping settings + source type (wide/narrow)
    /// @remarks Stretching/cropping value: [ 0 ; config::maxScreenFraming() ]
    Viewport(const pandora::hardware::DisplayMode& resolution, uint32_t stretching,
             uint32_t cropping, bool isWideSource) noexcept;
    /// @brief Compute window mode viewport, based on source type (wide/narrow)
    Viewport(uint32_t clientHeight, bool isWideSource) noexcept;
    
    Viewport() : _scaledSourceWidth(0), _scaledSourceHeight(0), _minWindowWidth(__MIN_WINDOW_HEIGHT*4/3) {}
    Viewport(const Viewport&) = default;
    Viewport(Viewport&&) = default;
    Viewport& operator=(const Viewport&) = default;
    Viewport& operator=(Viewport&&) = default;
    
    /// @brief Source width after resizing/stretching (render target size) -> will be cropped to fit screen
    inline uint32_t scaledSourceWidth() const noexcept { return _scaledSourceWidth; }
    /// @brief Source height after resizing/stretching (render target size) -> will be cropped to fit screen
    inline uint32_t scaledSourceHeight() const noexcept { return _scaledSourceHeight; }

    /// @brief Minimum window size allowed (based on source ratio) - width
    inline uint32_t minWindowWidth() const noexcept { return _minWindowWidth; }
    /// @brief Minimum window size allowed (based on source ratio) - height
    inline uint32_t minWindowHeight() const noexcept { return __MIN_WINDOW_HEIGHT; }
    
  private:
    uint32_t _scaledSourceWidth;
    uint32_t _scaledSourceHeight;
    uint32_t _minWindowWidth;
  };
}
