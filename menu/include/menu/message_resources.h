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
#include "menu/controls/control.h"

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
    
    windowGroup,
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
    
    emulatorGroup,
    precision,
    precision_original,
    precision_subprecision,
    precision_tooltip,
    widescreen,
    widescreen_tooltip,
    
    rateGroup,
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
    
    uiGroup,
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

  private:
    void fillMessagesEN();
    void fillMessagesFR();
    void fillMessagesDE();
    void fillMessagesES();
    
  private:
    MessageResource generalSettings[(size_t)GeneralSettingsMessages::COUNT];
    LocalizationType language_ = LocalizationType::en;
  };
}
#define GET_UI_MESSAGE(messageArray,enumValue) messageArray[(size_t)enumValue]
