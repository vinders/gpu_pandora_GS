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

#include <cstdint>
#include <cstddef>
#include <memory/light_string.h>

namespace config {
  // -- window / renderer settings -- ----------------------------------
  
  /// @brief 3D rendering API to use
  enum class RenderingApi : int {
    d3d11   = 0, ///< Direct3D 11
    openGL4 = 1, ///< OpenGL 4.5/4.6 (4.1 on macOS)
    vulkan  = 2  ///< Vulkan 1.2+
  };
# if defined(_VIDEO_D3D11_SUPPORT)
    constexpr inline RenderingApi defaultRenderingApi() noexcept { return RenderingApi::d3d11; }
# elif defined(_VIDEO_VULKAN_SUPPORT)
#   constexpr inline RenderingApi defaultRenderingApi() noexcept { return RenderingApi::vulkan; }
# else
#   constexpr inline RenderingApi defaultRenderingApi() noexcept { return RenderingApi::openGL4; }
# endif
  
  /// @brief Geometry subprecision mode
  enum class PrecisionMode : int {
    standard     = 0, ///< Original integer coordinates
    subprecision = 1, ///< GTE subprecision (edgbla's/tweak/ePSXe)
    pgxp         = 2  ///< PGXP-compatible GTE subprecision
  };
  /// @brief Window display mode
  enum class WindowMode : int {
    fullscreen        = 0, ///< Standard fullscreen mode (allows custom resolution, no other screens)
    fullscreenWindow  = 1, ///< Fullscreen-window mode (always desktop resolution, multi-monitor ok)
    window            = 2  ///< Window mode (resizable with homothety)
  };
  constexpr inline uint32_t desktopResolution() noexcept { return 0; }      ///< Operating system resolution
  constexpr inline uint32_t defaultWindowHeight() noexcept { return 720u; } ///< Default height in window mode
  constexpr inline uint32_t defaultRefreshRateMilli() noexcept { return 60000u; } ///< Default rate (milli-Hertz)
  constexpr inline uint32_t maxScreenFraming() noexcept { return 8u; } ///< Max value for stretching/cropping/...
  
  /// @brief Polygon fill mode
  enum class FillMode : int {
    normal           = 0, ///< Standard polygons (filled when requested by commands)
    wireframe        = 1, ///< Wireframe: only edges (all polygons)
    wireframeOverlay = 2  ///< Wireframe overlay on top of visible standard polygons
  };

  /// @brief OSD textual info to display
  enum class OnScreenDisplay : int {
    none       = 0,
    framerate  = 1, ///< Current FPS
    renderInfo = 2  ///< Current FPS + rendering info (vsync, polygon count, source resolution)
  };
  
  
  // -- time management --------------------------------------------------------
  
  /// @brief OSD textual info to display
  enum class FrameSkipping : int {
    none       = 0,
    evenFrames = 1, ///< Skip every 2nd frame (even frames) if too slow
    adaptative = 2  ///< Skip frames based on actual lateness
  };
  constexpr inline float autodetectFramerate() noexcept { return 0.0f; } ///< Auto-detect NTSC/PAL frame-rate limit
  
  
  // -- menu / hotkey management -- --------------------------------------------

  constexpr inline uint32_t disabledKey() noexcept { return 0; } ///< Auto-detect NTSC/PAL frame-rate limit

  // Keyboard control - key mapping
  namespace keyboardMap {
    static constexpr inline uint32_t showHideMenu()     noexcept { return 0; } ///< Show/hide profile selection menu
    static constexpr inline uint32_t pause()            noexcept { return 1; } ///< Pause/resume game (ePSXe: F10)
    static constexpr inline uint32_t turbo()            noexcept { return 2; } ///< Turbo game mode while pressed
                                                                               ///  (requires plugin's rate limit instead of emulator's (ePSXe2.0.5: F4))
    static constexpr inline uint32_t toggleSlowMotion() noexcept { return 3; } ///< Enable/disable slow-motion game mode
    static constexpr inline uint32_t toggleVsync()      noexcept { return 4; } ///< Enable/disable vsync in renderer
    static constexpr inline uint32_t toggleWindowMode() noexcept { return 5; } ///< Toggle fullscreen/window mode
    static constexpr inline uint32_t toggleWideSource() noexcept { return 6; } ///< Toggle widescreen mode
                                                                               ///  (for widescreen hacks or games with 16:9 mode)
    static constexpr inline uint32_t toggleOSD()        noexcept { return 7; } ///< Change on-screen-display info (FPS...)
    static constexpr inline size_t length() noexcept { return 8; }
  }
  // Gamepad control - button mapping
  namespace controllerMap {
    static constexpr inline uint32_t showHideMenu()     noexcept { return 0; } ///< Show/hide profile selection menu
    static constexpr inline uint32_t turbo()            noexcept { return 1; } ///< Turbo game mode while pressed
                                                                               ///  (requires plugin's rate limit instead of emulator's (ePSXe2.0.5: F4))
    static constexpr inline uint32_t toggleSlowMotion() noexcept { return 2; } ///< Enable/disable slow-motion game mode
    static constexpr inline uint32_t saveState()        noexcept { return 3; } ///< Send save-state command (ePSXe/PCSX: F1)
    static constexpr inline uint32_t loadState()        noexcept { return 4; } ///< Send load-state command (ePSXe/PCSX: F3)
    static constexpr inline uint32_t nextStateSlot()    noexcept { return 5; } ///< Move to next slot (ePSXe/PCSX: F2)
    static constexpr inline size_t length() noexcept { return 6; }
  }


  // -- rendering settings -- --------------------------------------------------

  /// @brief Color output mode - original 16-bit / 32-bit
  enum class ColorOutput : int {
    rgb16 = 16, ///< 16-bit - accurate colors, causes banding or grain effect
    rgb32 = 32  ///< 32-bit - converted colors, no artifacts
  };
  /// @brief Color dithering mode - used to reduce color banding with 16-bit colors
  enum class ColorDithering : int {
    none         = 0, ///< No dithering - color banding in 16-bit / normal mode for 32-bit colors
    ditherSource = 1, ///< Dithering applied before scaling - no more color banding, grainy effect
    ditherOutput = 2  ///< Dithering applied after scaling - semi color banding, no grain
  };
  /// @brief Anti-aliasing methods
  enum class AntiAliasing : int {
    none = 0,
    fxaa = 0x11, ///< Fast-approximate: blurry, very fast
    nfaa = 0x12, ///< Normal filter: very good with 2D, fast
    smaa2 = 0x23, ///< Subpixel morph 2x: faster than MSAA, great results (if supported)
    smaa4 = 0x43, ///< Subpixel morph 4x: faster than MSAA, great results (if supported)
    smaa8 = 0x83, ///< Subpixel morph 8x: faster than MSAA, great results (if supported)
    msaa2 = 0x24, ///< Multi-sample 2x: great results, slow
    msaa4 = 0x44, ///< Multi-sample 2x: great results, slow
    msaa8 = 0x84, ///< Multi-sample 2x: great results, very slow
  };

  // ---

# define __UPSCALER_ID(id,x2,x3,x4,x5,x6,x8) (id | (x2<<16) | (x3<<17) | (x4<<18) | (x5<<19) | (x6<<20) | (x8<<22))

  /// @brief Upscaling filter for sprites/textures/screen (integer factor)
  enum class UpscalingFilter : int {
    none      = 0,                            ///< Nearest pixel: fastest (no upscaling)
    lanczos   = __UPSCALER_ID(1,1,0,1,0,0,1), ///< Lanczos: smooth filter with improved/rounded gradients (average aliasing)
    jinc2     = __UPSCALER_ID(2,1,0,1,0,0,1), ///< Jinc2: smooth filter, great for 2D games and cinematics (average aliasing)
    xSaI      = __UPSCALER_ID(3,1,0,1,0,0,1), ///< SaI: fast and sharp, but high aliasing
    SABR      = __UPSCALER_ID(4,1,1,1,0,1,1), ///< SABR: smoother than xBR/SaI, great for 2D games, causes color banding
    xBR       = __UPSCALER_ID(5,1,1,1,1,1,1), ///< xBR: very sharp, excellent edge detection, low aliasing, causes color banding
    xBRZ      = __UPSCALER_ID(6,0,1,1,1,0,0), ///< xBRZ: xBR sharp edge detection with improved curves, slower
    super_xBR = __UPSCALER_ID(7,1,0,1,0,0,1), ///< super-xBR-fast-bilateral: xBR sharp edges combined with smoother gradients (low aliasing)
  };
  /// @brief Verify if a scaling factor can be used for a specific upscaling filter
  constexpr inline bool isScalingFactorValid(UpscalingFilter filter, int factor) noexcept {
    return ((int)filter & (1 << (factor + 14)) );
  }

  /// @brief Upscaling filter for MDEC videos
  enum class MdecFilter : int {
    none      = 0,
    bilinear  = 1, ///< Smooth/blurry standard filter
    super_xBR = 2, ///< super-xBR: sharp edge upscaling combined with linear gradient for cartoon cinematics
    jinc2     = 3, ///< Jinc2 (2x/4x/8x): smooth upscaling filter for realistic cinematics (average aliasing, ringing effect)
    nnedi3    = 4  ///< NNEDI3: sharp upscaling filter for realistic cinematics (not too sharp, low aliasing, no ringing, slower)
  };
  /// @brief Noise filter to add grain on smoothed surfaces
  enum class NoiseFilter : int {
    none  = 0,
    grain = 1, ///< Photographic grain
    gauss = 2  ///< Gaussian noise
  };


  // -- profile menu -- --------------------------------------------------------

  using ProfileId = uint32_t;

  /// @brief Profile tile color in menu
  enum class MenuTileColor : uint32_t {
    white = 0xFFFFFF,
    red = 0xFA3333,
    orange = 0xEE8800,
    yellow = 0xE5C700,
    green = 0x33BB33,
    teal = 0x22BB77,
    cyan = 0x44BBCC,
    blue = 0x3377EE,
    violet = 0x9944FF,
    pink = 0xCC44BB
  };
  /// @brief Convert tile color enum to RGB components
  inline void toColorComponents(MenuTileColor color, float outComponents[3]) noexcept {
    outComponents[0] = static_cast<float>(((uint32_t)color & 0xFF0000)>>16) / 255.f;
    outComponents[1] = static_cast<float>(((uint32_t)color & 0x00FF00)>>8) / 255.f;
    outComponents[2] = static_cast<float>((uint32_t)color & 0x0000FF) / 255.f;
  }


  // -- string containers -- ---------------------------------------------------

# ifdef _WINDOWS
    using UnicodeString = pandora::memory::LightWString;
#   define __UNICODE_CHAR wchar_t
#   define __UNICODE_STR(str) L"" str
#   define __ABS_PATH_SEP "\\"
# else
    using UnicodeString = pandora::memory::LightString;
#   define __UNICODE_CHAR char
#   define __UNICODE_STR(str) u8"" str
#   define __ABS_PATH_SEP "/"
# endif

  /// @brief Array of unicode character-codes
  /// @remarks Used to store string labels for display (with char-code based font-map)
  class CharCodeArray final {
  public:
    CharCodeArray() = default;
    CharCodeArray(const char* utf8, size_t length); ///< Create from UTF-8 encoded string (length: number of bytes)
    CharCodeArray(const char16_t* utf16, size_t length); ///< Create from UTF-16 encoded string (length: number of bytes)
#   ifdef _WINDOWS
      CharCodeArray(const wchar_t* utf16, size_t length) : CharCodeArray((const char16_t*)utf16, length) {}
#   endif

    CharCodeArray(const CharCodeArray& rhs);
    CharCodeArray(CharCodeArray&& rhs) noexcept
      : _array(rhs._array), _length(rhs._length) { rhs._array=nullptr; rhs._length=0; }
    CharCodeArray& operator=(const CharCodeArray& rhs);
    CharCodeArray& operator=(CharCodeArray&& rhs) noexcept {
      _array=rhs._array; _length=rhs._length;
      rhs._array=nullptr; rhs._length=0;
      return *this;
    }
    ~CharCodeArray() noexcept;

    inline bool empty() const noexcept { return (_array == nullptr); } ///< Array initialized (false) or NULL (true)

    inline const uint32_t* value() const noexcept { return _array; } ///< Array of char-codes (no trailing zero) / NULL if empty
    inline size_t size() const noexcept { return _length; } ///< Array length (no trailing zero)

    bool operator==(const CharCodeArray& rhs) const noexcept;
    inline bool operator!=(const CharCodeArray& rhs) const noexcept { return !(this->operator==(rhs)); }

  private:
    uint32_t* _array = nullptr;
    size_t _length = 0;
  };
}
