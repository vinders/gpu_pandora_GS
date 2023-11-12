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
    static const char32_t* toLanguageName(LocalizationType type) noexcept; ///< Message localization type to string
  };
  
  // ---
  
  enum class GeneralSettingsMessages : size_t { ///< Message types for general settings page
    title = 0,
    windowGroup, // window/display group
    displayMode,
    displayMode_fullscreen,
    displayMode_borderless,
    displayMode_window,
    displayMode_tooltip,
    refreshRate_tooltip,
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
    rateGroup, // frame rate group
    rateLimit_tooltip,
    rateLimit,
    rateLimit_disabled,
    rateLimit_autodetect,
    rateLimit_custom,
    customRate,
    customRate_fps,
    customRate_tooltip,
    frameSkip,
    frameSkip_tooltip,
    vsync,
    vsync_tooltip,
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
    clockVisibility_disabled,
    clockVisibility_mouseMove,
    clockVisibility_always,
    clockFormat,
    clockLocationX,
    clockLocationX_left,
    clockLocationX_center,
    clockLocationX_right,
    clockLocationY,
    clockLocationY_top,
    clockLocationY_bottom,
    techInfoGroup, // technical info group
    techInfoVisibility,
    techInfoType,
    techInfoType_fps,
    techInfoType_format,
    techInfoType_all,
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
    apply,
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
    maxCurved,
    screenCurvature_tooltip,
    COUNT
  };
  enum class CompatibilitySettingsMessages : size_t { ///< Message types for compatibility settings page
    title = 0,
    specialGameFixesGroup,
    lowCompatibilityFrameRead,
    lowCompatibilityFrameRead_tooltip,
    ignoreSmallFrameBufferMoves,
    ignoreSmallFrameBufferMoves_tooltip,
    fakeGpuBusyStates,
    fakeGpuBusyStates_tooltip,
    COUNT
  };

  // ---
  
  using MessageResource = const char32_t*;

  /// @brief UI localized text messages
  class MessageResources final {
  public:
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

    /// @brief Message resources for general settings UI page
    /// @remarks Access messages using GET_UI_MESSAGE(messageArray, GeneralSettingsMessages::<...>)
    inline const MessageResource* generalSettingsMessageArray() const noexcept { return generalSettings; }
    /// @brief Message resources for hotkey bindings UI page - general
    /// @remarks Access messages using GET_UI_MESSAGE(messageArray, HotkeyBindingsMessages::<...>)
    inline const MessageResource* hotkeyBindingsMessageArray() const noexcept { return hotkeyBindings; }
    /// @brief Message resources for hotkey bindings UI page - actions
    /// @remarks Access messages using GET_UI_MESSAGE(messageArray, HotkeyActions::<...>)
    inline const MessageResource* hotkeyActionsMessageArray() const noexcept { return hotkeyActions; }
    /// @brief Message resources for hotkey bindings UI page - actions tooltips
    /// @remarks Access messages using GET_UI_MESSAGE(messageArray, HotkeyActions::<...>)
    inline const MessageResource* hotkeyActionsTooltipsMessageArray() const noexcept { return hotkeyActionsTooltips; }
    /// @brief Message resources for on-screen display settings UI page
    /// @remarks Access messages using GET_UI_MESSAGE(messageArray, OsdSettingsMessages::<...>)
    inline const MessageResource* osdSettingsMessageArray() const noexcept { return osdSettings; }

    /// @brief Message resources for base profile settings UI page
    /// @remarks Access messages using GET_UI_MESSAGE(messageArray, ProfileSettingsMessages::<...>)
    inline const MessageResource* profileSettingsMessageArray() const noexcept { return profileSettings; }
    /// @brief Message resources for UI tile color picker
    /// @remarks Access messages using GET_UI_MESSAGE(messageArray, TileColors::<...>)
    inline const MessageResource* tileColorsMessageArray() const noexcept { return tileColors; }
    /// @brief Message resources for screen stretching UI page
    /// @remarks Access messages using GET_UI_MESSAGE(messageArray, ScreenStretchingMessages::<...>)
    inline const MessageResource* screenStretchingMessageArray() const noexcept { return screenStretching; }
    /// @brief Message resources for compatibility UI page
    /// @remarks Access messages using GET_UI_MESSAGE(messageArray, CompatibilitySettingsMessages::<...>)
    inline const MessageResource* compatibilityMessageArray() const noexcept { return compatibility; }

  private:
    void fillMessagesEN();
    void fillMessagesFR();
    void fillMessagesDE();
    void fillMessagesES();
    
  private:
    MessageResource generalSettings[(size_t)GeneralSettingsMessages::COUNT];
    MessageResource hotkeyBindings[(size_t)HotkeyBindingsMessages::COUNT];
    MessageResource hotkeyActions[(size_t)HotkeyActions::COUNT];
    MessageResource hotkeyActionsTooltips[(size_t)HotkeyActions::COUNT];
    MessageResource osdSettings[(size_t)OsdSettingsMessages::COUNT];
    MessageResource profileSettings[(size_t)ProfileSettingsMessages::COUNT];
    MessageResource tileColors[(size_t)TileColors::COUNT];
    MessageResource screenStretching[(size_t)ScreenStretchingMessages::COUNT];
    MessageResource compatibility[(size_t)CompatibilitySettingsMessages::COUNT];
    LocalizationType language_ = LocalizationType::en;
  };
}
#define GET_UI_MESSAGE(messageArray,enumValue) messageArray[(size_t)enumValue]
