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
#pragma once

#include <cstddef>
#include <cstdint>
#include "menu/hotkey_actions.h"
#include "menu/interpolation_type.h"
#include "menu/tile_colors.h"

namespace menu {
  enum class LocalizationType : uint32_t { ///< Message localization type (language)
    en = 0, ///< English
    fr,     ///< French (Français)
    de,     ///< German (Deutsch)
    es,     ///< Spanish (Español)
    COUNT
  };
  class LocalizationTypeHelper final {
  public:
    static const char16_t* toLanguageName(LocalizationType type) noexcept; ///< Localization language type to string
  };
  
  // ---
  
  enum class CommonMessages : size_t { ///< Message types common to all pages
    disabled = 0,
    ok,
    cancel,
    apply,
    maximum,
    fps,
    left,
    center,
    right,
    top,
    bottom,
    to,
    COUNT
  };

  enum class GeneralSettingsMessages : size_t { ///< Message types for general settings page
    title = 0,
    windowGroup, // window/display group
    displayMode,
    displayMode_fullscreen,
    displayMode_borderless,
    displayMode_window,
    displayMode_tooltip,
    resolution,
    resolution_tooltip,
    windowSize,
    windowSize_tooltip,
    emulatorGroup, // emulator compatibility group
    precision,
    precision_original,
    precision_subprecision,
    precision_tooltip,
    widescreen,
    widescreen_tooltip,
    autosaveOnExit,
    autosaveOnExit_tooltip,
    autoloadOnStart,
    autoloadOnStart_tooltip,
    rateGroup, // frame rate group
    rateLimit_tooltip,
    rateLimit,
    rateLimit_autodetect,
    rateLimit_custom,
    customRate,
    customRate_tooltip,
    frameSkip,
    frameSkip_tooltip,
    uiGroup, // user interface group
    theme,
    theme_blue,
    theme_green,
    theme_scifi,
    theme_yellow,
    theme_tooltip,
    language,
    language_tooltip,
    COUNT
  };
  enum class HotkeyBindingsMessages : size_t { ///< Message types for hotkey bindings page
    title = 0,
    behaviorGroup,
    menuHintMouseMove,
    menuHintMouseMove_tooltip,
    enableKeyBindings,
    enableKeyBindings_tooltip,
    hotkey,
    hotkey_tooltip,
    hold,
    toggle,
    slowMotionMode,
    slowMotionMode_tooltip,
    fastForwardMode,
    fastForwardMode_tooltip,
    bindingsGroup,
    COUNT
  };
  enum class OsdSettingsMessages : size_t { ///< Message types for OSD settings page
    title = 0,
    clockGroup, // clock settings group
    clockVisibility,
    clockVisibility_mouseMove,
    clockVisibility_always,
    clockFormat,
    clockLocationX,
    clockLocationY,
    techInfoGroup, // technical info group
    techInfoVisibility,
    techInfoType,
    techInfoType_fps,
    techInfoType_format,
    techInfoType_all,
    COUNT
  };

  enum class ProfileSelectorMessages : size_t { ///< Message types for profile selector page
    title = 0,
    createProfile,
    editProfile,
    deleteProfile,
    select,
    create,
    removePopupTitle,
    removePopupMessage,
    COUNT
  };
  enum class ProfileSettingsMessages : size_t { ///< Message types for base profile settings page
    title = 0,
    profileIdGroup,
    profileName,
    tileColor,
    presetGroup,
    predefinedPreset,
    existingProfile,
    applyPopupTitle,
    applyPopupMessage,
    COUNT
  };
  enum class ScreenStretchingMessages : size_t { ///< Message types for screen stretching page
    title = 0,
    aspectRatioGroup,
    aspectRatio_keep,
    aspectRatio_stretch,
    aspectRatio_crop,
    aspectRatio_both,
    aspectRatio_half,
    aspectRatio_custom,
    aspectRatio_tooltip,
    keepRatio,
    stretch,
    stretching_tooltip,
    entire,
    cropped,
    cropping_tooltip,
    pixelRatio_square,
    pixelRatio_crt,
    pixelRatio_tooltip,
    displayAdjustmentsGroup,
    showOverscanArea,
    showOverscanArea_tooltip,
    centerX,
    centerX_tooltip,
    centerY,
    centerY_tooltip,
    mirror_none,
    mirror_mirrorX,
    mirror_tooltip,
    blackBorders,
    blackBorders_tooltip,
    screenCurvature,
    screenCurvature_tooltip,
    COUNT
  };
  enum class SmoothingUpscalingMessages : size_t { ///< Message types for smoothing/upscaling page
    title = 0,
    framebufferGroup, // frame-buffer group
    internalResolution,
    internalResolution_tooltip,
    framebufferResolution_prefix,
    displaySize_prefix,
    antiAliasing,
    antiAliasing_tooltip,
    mdecMovieFilter,
    mdecMovieFilter_tooltip,
    upscaling_tooltip,
    smoothing_tooltip,
    grain_photo,
    grain_gauss,
    grain_tooltip,
    screenGroup, // screen group
    screenUpscaling,
    screenBlur,
    screenBlur_tooltip,
    screenGrain,
    textureGroup, // texture group
    textureUpscaling,
    textureSmoothing,
    textureGrain,
    textureSplatting,
    textureSplatting_tooltip,
    spriteGroup, // sprite group
    spriteSameAsTexture,
    spriteSameAsTexture_tooltip,
    spriteUpscaling,
    spriteSmoothing,
    spriteGrain,
    COUNT
  };
  enum class AdvancedEffectsMessages : size_t { ///< Message types for compatibility settings page
    title = 0,
    gammaGroup,
    gammaCorrection_tooltip,
    colorGroup,
    colorBitDepth,
    colorBitDepth_tooltip,
    scanlines,
    scanlines_tooltip,
    colorFilter,
    colorFilter_tooltip,
    specialShading,
    specialShading_tooltip,
    hdrGroup,
    bloom,
    bloom_tooltip,
    lumaSharpen,
    lumaSharpen_tooltip,
    specialGameFixesGroup,
    lowCompatibilityFrameRead,
    lowCompatibilityFrameRead_tooltip,
    ignoreSmallFramebufferMoves,
    ignoreSmallFramebufferMoves_tooltip,
    fakeGpuBusyStates,
    fakeGpuBusyStates_tooltip,
    COUNT
  };

  // ---
  
  /// @brief UI localized text messages
  class MessageResources final {
  public:
    using Message = const char16_t*;

    inline MessageResources(LocalizationType language) {
      updateLocalization(language);
    }
    MessageResources(const MessageResources&) = default;
    MessageResources& operator=(const MessageResources&) = default;
    ~MessageResources() noexcept = default;

    // -- language selection --

    LocalizationType language() const noexcept { return language_; } ///< Current localization type
    void updateLocalization(LocalizationType language) noexcept; ///< Change localization type

    // -- message array accessors --

    /// @brief Message resources common to all UI pages
    inline const char16_t* getMessage(CommonMessages id) const noexcept { return common[(size_t)id]; }

    /// @brief Message resources for general settings UI page
    inline const char16_t* getMessage(GeneralSettingsMessages id) const noexcept { return generalSettings[(size_t)id]; }
    /// @brief Message resources for hotkey bindings UI page - general
    inline const char16_t* getMessage(HotkeyBindingsMessages id) const noexcept { return hotkeyBindings[(size_t)id]; }
    /// @brief Message resources for hotkey bindings UI page - actions
    inline const char16_t* getMessage(HotkeyActions id) const noexcept { return hotkeyActions[(size_t)id]; }
    /// @brief Message resources for hotkey bindings UI page - actions tooltips
    inline const char16_t* getMessageTooltip(HotkeyActions id) const noexcept { return hotkeyActionsTooltips[(size_t)id]; }
    /// @brief Message resources for on-screen display settings UI page
    inline const char16_t* getMessage(OsdSettingsMessages id) const noexcept { return osdSettings[(size_t)id]; }

    /// @brief Message resources for base profile settings UI page
    inline const char16_t* getMessage(ProfileSelectorMessages id) const noexcept { return profileSelector[(size_t)id]; }
    /// @brief Message resources for base profile settings UI page
    inline const char16_t* getMessage(ProfileSettingsMessages id) const noexcept { return profileSettings[(size_t)id]; }
    /// @brief Message resources for UI tile color picker
    inline const char16_t* getMessage(TileColors id) const noexcept { return tileColors[(size_t)id]; }
    /// @brief Message resources for screen stretching UI page
    inline const char16_t* getMessage(ScreenStretchingMessages id) const noexcept { return screenStretching[(size_t)id]; }
    /// @brief Message resources for smoothing/upscaling UI page
    inline const char16_t* getMessage(SmoothingUpscalingMessages id) const noexcept { return smoothingUpscaling[(size_t)id]; }
    /// @brief Message resources for UI interpolation choice
    inline const char16_t* getMessage(InterpolationType id) const noexcept { return interpolations[(size_t)id]; }
    /// @brief Message resources for UI upscaling choice
    inline const char16_t* getMessage(UpscalingType id) const noexcept { return upscalings[(size_t)id]; }
    /// @brief Message resources for UI MDEC filter choice
    inline const char16_t* getMessage(MdecFilter id) const noexcept { return mdecFilters[(size_t)id]; }
    /// @brief Message resources for UI anti-aliasing choice
    inline const char16_t* getMessage(AntiAliasing id) const noexcept { return antiAliasings[(size_t)id]; }
    /// @brief Message resources for advanced effects UI page
    inline const char16_t* getMessage(AdvancedEffectsMessages id) const noexcept { return advancedEffects[(size_t)id]; }

  private:
    void fillMessagesEN();
    void fillMessagesFR();
    void fillMessagesDE();
    void fillMessagesES();
    
  private:
    Message common[(size_t)CommonMessages::COUNT];

    Message generalSettings[(size_t)GeneralSettingsMessages::COUNT];
    Message hotkeyBindings[(size_t)HotkeyBindingsMessages::COUNT];
    Message hotkeyActions[(size_t)HotkeyActions::COUNT];
    Message hotkeyActionsTooltips[(size_t)HotkeyActions::COUNT];
    Message osdSettings[(size_t)OsdSettingsMessages::COUNT];

    Message profileSelector[(size_t)ProfileSelectorMessages::COUNT];
    Message profileSettings[(size_t)ProfileSettingsMessages::COUNT];
    Message tileColors[(size_t)TileColors::COUNT];
    Message screenStretching[(size_t)ScreenStretchingMessages::COUNT];
    Message smoothingUpscaling[(size_t)SmoothingUpscalingMessages::COUNT];
    Message interpolations[(size_t)InterpolationType::COUNT];
    Message upscalings[(size_t)UpscalingType::COUNT];
    Message mdecFilters[(size_t)MdecFilter::COUNT];
    Message antiAliasings[(size_t)AntiAliasing::COUNT];
    Message advancedEffects[(size_t)AdvancedEffectsMessages::COUNT];
    LocalizationType language_ = LocalizationType::en;
  };
}
