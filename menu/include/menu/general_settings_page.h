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
#include <unordered_map>
#include <hardware/display_monitor.h>
#include "menu/color_theme.h"
#include "menu/renderer_state_buffers.h"
#include "menu/page.h"
#include "menu/controls/button.h"
#include "menu/controls/check_box.h"
#include "menu/controls/combo_box.h"
#include "menu/controls/fieldset.h"
#include "menu/controls/scroll_bar.h"
#include "menu/controls/slider.h"
#include "menu/controls/text_box.h"
#include "menu/controls/tooltip.h"

namespace menu {
  class GeneralSettingsPage final : public Page {
  public:
    GeneralSettingsPage(std::shared_ptr<RendererContext> context, std::shared_ptr<RendererStateBuffers> buffers,
                        const ColorTheme& theme, const pandora::hardware::DisplayMonitor& monitor,
                        int32_t x, int32_t y, uint32_t width, uint32_t height);
    ~GeneralSettingsPage() noexcept override;

    // -- window event --

    /// @brief Report page resize event
    void move(int32_t x, int32_t y, uint32_t width, uint32_t height) override;

    // -- rendering --

    /// @brief Draw page control icons
    /// @remarks Use 'bindGraphicsPipeline' (for flat-shaded images) before call.
    void drawIcons() override;
    /// @brief Draw page control foregrounds (if any)
    /// @remarks Use 'bindGraphicsPipeline' (for control backgrounds) before call.
    void drawForegrounds() override;
    /// @brief Draw page control foreground labels (if any)
    /// @remarks Use 'bindGraphicsPipeline' (for control labels) before call.
    void drawForegroundLabels() override;

  private:
    void onChange(uint32_t id, uint32_t value);
    bool drawPageBackgrounds(int32_t mouseX, int32_t mouseY) override;
    void drawPageLabels() override;

  private:
    display::controls::TextMesh title;

    // display mode
    controls::Fieldset displayGroup;
    controls::Slider displayMode;
    controls::ComboBox fullscreenSize;
    controls::ComboBox fullscreenRate;
    controls::TextBox windowHeight;
    display::controls::TextMesh windowSize;
    bool isFullscreenMode = false;
    bool isWindowMode = false;

    // emulator compatibility
    controls::Fieldset compatibilityGroup;
    controls::Slider subprecisionMode;
    controls::CheckBox widescreenMode;
    bool enableWidescreenMode = false;

    // framerate
    controls::Fieldset framerateGroup;
    controls::Slider framerateLimit;
    controls::CheckBox frameSkipping;
    controls::TextBox fixedFramerate;
    controls::CheckBox vsync;
    bool isFramerateLimit = true;
    bool isFixedFramerate = false;
    bool isFrameSkipping = false;
    bool enableVsync = false;
    std::unordered_map<uint32_t, std::vector<uint32_t> > fullscreenRatesPerSize;
  };
}
