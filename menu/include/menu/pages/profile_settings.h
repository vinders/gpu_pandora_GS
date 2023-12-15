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
#include <cstring>
#include <memory>
#include <vector>
#include "menu/color_theme.h"
#include "menu/config_profile.h"
#include "menu/message_resources.h"
#include "menu/renderer_state_buffers.h"
#include "menu/controls/button.h"
#include "menu/controls/combo_box.h"
#include "menu/controls/fieldset.h"
#include "menu/controls/text_box.h"
#include "menu/controls/popup.h"
#include "menu/pages/page.h"

namespace menu {
  namespace pages {
    class ProfileSettings final : public Page {
    public:
      ProfileSettings(std::shared_ptr<RendererContext> context, std::shared_ptr<RendererStateBuffers> buffers,
                      const std::shared_ptr<ColorTheme>& theme, const MessageResources& localizedText,
                      int32_t x, int32_t y, uint32_t width, uint32_t height,
                      uint32_t profileId, const std::vector<ConfigProfile>& profiles, const std::vector<ConfigProfile>& presets)
        : Page(std::move(context), std::move(buffers), *theme, x, y, width, height, true, true),
          theme(theme),
          profileId(profileId),
          hasOtherProfiles(profiles.size() > (size_t)1) {
        init(localizedText, x, y, width, profiles, presets);
      }
      ~ProfileSettings() noexcept override;

      PageId PageType() const noexcept override;

      // -- window event --

      /// @brief Report page resize event
      void move(int32_t x, int32_t y, uint32_t width, uint32_t height) override;

    private:
      void init(const MessageResources& localizedText, int32_t x, int32_t y, uint32_t width,
                const std::vector<ConfigProfile>& profiles, const std::vector<ConfigProfile>& presets);
      void onChange(uint32_t id);
      void onValueChange(uint32_t id, uint32_t value);
      void drawPageBackgrounds(int32_t mouseX, int32_t mouseY) override;
      void drawPageLabels() override;

    private:
      std::shared_ptr<ColorTheme> theme = nullptr;
      display::controls::TextMesh title;

      // profile ID
      controls::Fieldset profileIdGroup;
      controls::TextBox profileName;
      display::controls::ControlMesh colorPreview;
      controls::ComboBox tileColor;
      uint32_t profileId = 0;

      // presets
      controls::Fieldset presetGroup;
      controls::ComboBox presetToApply;
      controls::ComboBox profileToCopy;
      controls::Button applyPreset;
      controls::Button copyProfile;
      bool hasOtherProfiles = false;

      controls::Popup confirmationPopup;
      uint32_t pendingAction = 0;
    };
  }
}
