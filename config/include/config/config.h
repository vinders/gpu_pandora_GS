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
#include <memory/light_string.h>
#include <hardware/display_monitor.h>
#include "config/types.h"

namespace config {
  // -- common config -- -------------------------------------------------------

  /// @brief General display window settings
  struct WindowConfig final {
    pandora::hardware::DisplayMonitor::DeviceId monitorId; ///< ID of display monitor to use (empty to use primary)
    WindowMode windowMode = WindowMode::fullscreenWindow;  ///< Display mode: window / fullscreen-window / fullscreen

    pandora::hardware::DisplayMode fullscreen{ ///< Fullscreen display mode (px,px,bit,mHz)
      desktopResolution(),desktopResolution(),32,defaultRefreshRateMilli()
    };
    uint32_t windowHeight = defaultWindowHeight(); ///< Height of client area in window mode (px)
    bool isWideSource = false; ///< 16:9 source data (emulator with widescreen hack or games with 16:9 mode)
  };

  /// @brief General renderer settings
  struct RendererConfig final {
    RenderingApi api = defaultRenderingApi();          ///< Rendering API used for display (Direct3D11, Vulkan...)
    PrecisionMode precision = PrecisionMode::standard; ///< Geometry precision/subprecision mode for coordinates
    bool enableVsync = false;                          ///< Vertical sync (no tearing, lower power consump., but higher input delay)
    bool enableFramerateLimit = true;                  ///< Enable framerate limiter (with 'framerateLimit')
    float framerateLimit = autodetectFramerate();      ///< Framerate limit (frames per second / autodetectFramerate())
    FrameSkipping frameSkip = FrameSkipping::none;     ///< Frame skipping mode
    OnScreenDisplay osd = OnScreenDisplay::none;       ///< On-screen-display: none / FPS / rendering info
  };

  // ---
  
  /// @brief User action settings (menu / hotkeys)
  struct ActionsConfig final {
  public:
    void initDefaultMapping() noexcept; ///< Initialize keyboardMapping/controllerMapping with default values

    /// @brief Keyboard control: key mapping (index access with keyboardMap::<name_of_action>)
    uint32_t keyboardMapping[keyboardMap::length()]{ disabledKey() };
    /// @brief Gamepad control: button mapping (index access with controllerMap::<name_of_action>)
    uint32_t controllerMapping[controllerMap::length()]{ disabledKey() };
    uint32_t controllerHotkey = disabledKey(); ///< Gamepad hotkey (combined with mapped buttons to trigger actions)
  };


  // -- profile config -- ------------------------------------------------------

  /// @brief Additional display window settings - per profile
  struct WindowProfile final {
    uint32_t screenStretching = 0; ///< Stretch screen to fit (if output isn't 4:3): 0 - 8 (maxScreenFraming)
    uint32_t screenCropping = 0;   ///< Crop screen to fit (if output isn't 4:3): 0 - 8 (maxScreenFraming)
    uint32_t screenCurvature = 0;  ///< Apply screen curvature effect (similar to CRT TV): 0 - 8 (maxScreenFraming)
    bool isMirrored = false;       ///< Mirror display content

    bool isPalRecentered = true;    ///< Recenter PAL: fix mis-centered screen positions used by many PAL games (usually NTSC ports)
    bool isOverscanVisible = false; ///< Show pixels located outside of TV boundaries (frame wider than TV, e.g. in Capcom fighters)
    uint8_t blackBorderSizes[4] = { '\0','\0','\0','\0' }; ///< Hide edge pixels with black borders (to fix flickering in some games)
  };

  /// @brief Additional renderer settings - per profile
  struct RendererProfile final {
    uint32_t internalResFactorX = 4; ///< Internal resolution X factor
    uint32_t internalResFactorY = 4; ///< Internal resolution Y factor
    ColorOutput colorMode = ColorOutput::rgb32;      ///< Color output mode (16/32-bit)
    FillMode fillMode = FillMode::normal;            ///< Normal rendering / wireframe / wireframe overlay
    AntiAliasing antiAliasing = AntiAliasing::none;  ///< Anti-aliasing: to remove jagged edges

    UpscalingFilter textureUpscaling = UpscalingFilter::lanczos;
    uint32_t textureUpscalingFactor = 4; ///< Texture upscaling factor (1x/2x/3x/4x/5x/8x)
    bool useTextureBilinear = true;      ///< Enable bilinear interp. for texture sampler (after upscaling)
    UpscalingFilter spriteUpscaling = UpscalingFilter::lanczos;
    uint32_t spriteUpscalingFactor = 4; ///< Sprite upscaling factor (1x/2x/3x/4x/5x/8x)
    bool useSpriteBilinear = false;     ///< Enable bilinear interp. for sprite sampler (after upscaling)
    UpscalingFilter screenUpscaling = UpscalingFilter::none; ///< Screen upscaling mode (after internal resolution scaling)
    uint32_t screenUpscalingFactor = 1; ///< Screen upscaling factor (1x/2x/3x/4x/5x/8x)
    MdecFilter mdecUpscaling = MdecFilter::bilinear; ///< MDEC video upscaling
  };
  
  /// @brief Filters and visual effects - per profile
  struct EffectsProfile final {
    NoiseFilter textureGrain = NoiseFilter::none; ///< Add grain to textures (after upscaling), to fix unnatural smoothness/sharpness
    NoiseFilter screenGrain = NoiseFilter::none;  ///< Add noise/grain to screen, to fix unnatural smoothness/sharpness
                                                  ///  or for photographic grain effect
    ColorDithering dithering = ColorDithering::none; ///< Color dithering mode (to reduce color banding with 16-bit colors)
    bool useTextureDithering = false; ///< Texture deposterization (useful after filters with banding artifacts)
    bool useSpriteDithering = false;  ///< Sprite deposterization (useful after filters with banding artifacts)

    //other effects:
    // - texture transition: border mode (clamp, wrap, mirror, auto-detect) + texture splatting
    // - bump mapping
    // - motion blur
    // - HDR: bloom, luma-sharpen, tonemapping pass, light attenuation, pixel vibrance, subpixel dithering
    // - cel-shading V1 - 4, kirsch-negative, storybook, broken glass...
    // - gamma correction (+ presets PAL/NTSC) + S-curve contrast
    // - color-blind mode, natural vision CRT colors, color component/saturation adjustment
    // - scanlines: simple, CRT-royale, ...
    // - cursor: none/cross/circle + color/sat/alpha
  };


  // -- profile identifier -- --------------------------------------------------

  /// @brief Config profile label (name/color) + file ID
  /// @remarks For config dialog
  struct ProfileLabel final {
    ProfileId id = 0;   ///< Unique profile identifier
    UnicodeString file; ///< Name of file containing profile settings
    UnicodeString name; ///< Profile name: unicode string (UTF-8/16 encoding, depending on system)
    MenuTileColor tileColor = MenuTileColor::white; ///< Color of profile item in menu
  };

  /// @brief Config profile label (name/color) + file ID
  /// @remarks For ingame menu
  struct ProfileMenuTile final {
    ProfileMenuTile() = default;
    ProfileMenuTile(const ProfileMenuTile&) = default;
    ProfileMenuTile(ProfileMenuTile&&) noexcept = default;
    ProfileMenuTile& operator=(const ProfileMenuTile&) = default;
    ProfileMenuTile& operator=(ProfileMenuTile&&) noexcept = default;
    ~ProfileMenuTile() noexcept = default;

    ProfileMenuTile(const ProfileLabel& label) : file(label.file), name(label.name.c_str(), label.name.size()) {
      toColorComponents(label.tileColor, tileColor);
    }
    ProfileMenuTile(ProfileLabel&& label) : file(std::move(label.file)), name(label.name.c_str(), label.name.size()) {
      toColorComponents(label.tileColor, tileColor);
    }

    UnicodeString file; ///< Name of file containing profile settings
    CharCodeArray name; ///< Profile name: unicode char-codes (!= Profile.name, which is the UTF-8/16 encoded version)
    float tileColor[3] = { 1.f,1.f,1.f }; ///< RGB color of profile item in menu
  };
}
