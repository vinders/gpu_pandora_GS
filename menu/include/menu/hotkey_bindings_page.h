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
#include "menu/color_theme.h"
#include "menu/hotkey_actions.h"
#include "menu/message_resources.h"
#include "menu/renderer_state_buffers.h"
#include "menu/page.h"
#include "menu/controls/check_box.h"
#include "menu/controls/fieldset.h"
#include "menu/controls/key_binding.h"
#include "menu/controls/slider.h"

namespace menu {
  class HotkeyBindingsPage final : public Page {
  public:
    HotkeyBindingsPage(std::shared_ptr<RendererContext> context, std::shared_ptr<RendererStateBuffers> buffers,
                       const ColorTheme& theme, const MessageResources& localizedText,
                       int32_t x, int32_t y, uint32_t width, uint32_t height)
      : Page(std::move(context), std::move(buffers), theme, x, y, width, height, true) {
      init(theme, localizedText, x, y, width);
    }
    ~HotkeyBindingsPage() noexcept override;

    // -- window event --

    /// @brief Report page resize event
    void move(int32_t x, int32_t y, uint32_t width, uint32_t height) override;

    // -- rendering --

    /// @brief Draw page control icons
    /// @remarks Use 'bindGraphicsPipeline' (for flat-shaded images) before call.
    void drawIcons() override;

  private:
    void init(const ColorTheme& theme, const MessageResources& localizedText, int32_t x, int32_t y, uint32_t width);
    void resolveKeyboardBindings(const controls::KeyBinding* updatedControl) noexcept override;

    bool drawPageBackgrounds(int32_t mouseX, int32_t mouseY) override;
    void drawPageLabels() override;

  private:
    display::controls::TextMesh title;

    // hotkey/hint group
    controls::Fieldset behaviorGroup;
    controls::CheckBox menuHintMouseMove;
    controls::CheckBox enableKeyBindings;
    controls::KeyBinding hotkeyBinding;
    controls::Slider slowMotionMode;
    controls::Slider fastForwardMode;
    bool showMenuHint = true;
    bool isBindingEnabled = true;

    // bindings group
    controls::Fieldset bindingsGroup;
    controls::KeyBinding bindings[(size_t)HotkeyActions::COUNT]{};
  };
}
