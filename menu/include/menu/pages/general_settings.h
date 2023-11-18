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

#include <cstdint>
#include <memory>
#include <vector>
#include <functional>
#include <hardware/display_monitor.h>
#include "menu/color_theme.h"
#include "menu/message_resources.h"
#include "menu/renderer_state_buffers.h"
#include "menu/controls/button.h"
#include "menu/controls/check_box.h"
#include "menu/controls/combo_box.h"
#include "menu/controls/fieldset.h"
#include "menu/controls/slider.h"
#include "menu/controls/text_box.h"
#include "menu/pages/page.h"

namespace menu {
  namespace pages {
    struct ScreenResolution final {
      ScreenResolution(uint32_t width, uint32_t height) noexcept
        : width(width), height(height) {}
      ScreenResolution() noexcept = default;
      ScreenResolution(const ScreenResolution&) = default;
      ScreenResolution& operator=(const ScreenResolution&) = default;
      ~ScreenResolution() noexcept = default;

      uint32_t width = 0;
      uint32_t height = 0;
    };

    // ---

    class GeneralSettings final : public Page {
    public:
      GeneralSettings(std::shared_ptr<RendererContext> context, std::shared_ptr<RendererStateBuffers> buffers,
                      const std::shared_ptr<ColorTheme>& theme, const std::shared_ptr<MessageResources>& localizedText,
                      const pandora::hardware::DisplayMonitor& monitor, int32_t x, int32_t y,
                      uint32_t width, uint32_t height, std::function<void()> onThemeChange = nullptr);
      ~GeneralSettings() noexcept override;

      // -- window event --

      /// @brief Report page resize event
      void move(int32_t x, int32_t y, uint32_t width, uint32_t height) override;

      // -- rendering --

      /// @brief Draw page control icons
      /// @remarks Use 'bindGraphicsPipeline' (for flat-shaded images) before call.
      void drawIcons() override;

    private:
      void init(int32_t x, int32_t y, uint32_t width);
      void onChange(uint32_t id);
      void onValueChange(uint32_t id, uint32_t value);
      void drawPageBackgrounds(int32_t mouseX, int32_t mouseY) override;
      void drawPageLabels() override;

    private:
      display::controls::TextMesh title;

      // window/display mode
      controls::Fieldset windowGroup;
      controls::Slider displayMode;
      controls::ComboBox fullscreenSize;
      controls::ComboBox fullscreenRate;
      controls::TextBox windowHeight;
      display::controls::TextMesh windowSizeInfo;
      bool isFullscreenMode = false;
      bool isWindowMode = false;
      std::vector<ScreenResolution> fullscreenResolutions;
      std::vector<std::vector<uint32_t> > fullscreenRatesPerSize;

      // emulator compatibility
      controls::Fieldset compatibilityGroup;
      controls::Slider subprecisionMode;
      controls::CheckBox widescreenMode;
      controls::CheckBox autosaveOnExit;
      controls::CheckBox autoloadOnStart;
      bool enableWidescreenMode = false;
      bool isAutosaved = false;
      bool isAutoloaded = false;

      // framerate
      controls::Fieldset framerateGroup;
      controls::Slider framerateLimit;
      controls::CheckBox frameSkipping;
      controls::TextBox fixedFramerate;
      bool isFramerateLimit = true;
      bool isFixedFramerate = false;
      bool isFrameSkipping = false;

      // user interface
      controls::Fieldset userInterfaceGroup;
      controls::Slider interfaceColor;
      controls::ComboBox interfaceLanguage;

      std::shared_ptr<ColorTheme> theme = nullptr;
      std::shared_ptr<MessageResources> localizedText = nullptr;
      std::function<void()> onThemeChange;
    };
  }
}
