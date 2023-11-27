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
#include <hardware/display_monitor.h>
#include "menu/color_theme.h"
#include "menu/config_profile.h"
#include "menu/message_resources.h"
#include "menu/renderer_state_buffers.h"
#include "menu/controls/button.h"
#include "menu/controls/fieldset.h"
#include "menu/controls/label.h"
#include "menu/controls/slider.h"
#include "menu/controls/tile.h"
#include "menu/controls/popup.h"
#include "menu/pages/page.h"

namespace menu {
  namespace pages {
    class MainMenu final : public Page {
    public:
      MainMenu(std::shared_ptr<RendererContext> context, std::shared_ptr<RendererStateBuffers> buffers,
               const std::shared_ptr<ColorTheme>& theme, const MessageResources& localizedText,
               int32_t x, int32_t y, uint32_t width, uint32_t height, uint32_t activeProfileId, std::vector<ConfigProfile>& profiles)
        : Page(std::move(context), std::move(buffers), *theme, x, y, width, height, false, false),
          profiles(&profiles),
          activeProfileId(activeProfileId),
          theme(theme) {
        init(localizedText, x, y, width, height);
      }
      ~MainMenu() noexcept override;

      // -- window event --

      /// @brief Report page resize event
      void move(int32_t x, int32_t y, uint32_t width, uint32_t height) override;

      // -- rendering --

      /// @brief Draw page control icons
      /// @remarks Use 'bindGraphicsPipeline' (for flat-shaded images) before call.
      void drawIcons() override;

    private:
      void init(const MessageResources& localizedText, int32_t x, int32_t y, uint32_t width, uint32_t height);
      void onButtonAction(uint32_t id);
      void onTileAction(uint32_t id, controls::TileAction type);

      void adaptButtonStyle(controls::Button& button, const controls::Control* activeControl);
      void drawPageBackgrounds(int32_t mouseX, int32_t mouseY) override;
      void drawPageLabels() override;

    private:
      display::controls::TextMesh title;
      std::vector<ConfigProfile>* profiles = nullptr;
      uint32_t activeProfileId = 0;

      controls::Button resume;
      controls::Button loadState;
      controls::Button saveState;
      controls::Button resetGame;
      controls::Button exitGame;
      controls::Slider activeSaveSlot;
      display::controls::ControlMesh saveSlotHoverMesh;

      controls::Label navigateControllerInfo;
      controls::Label selectControllerInfo;

      controls::Popup resetPopup;

      std::shared_ptr<ColorTheme> theme = nullptr;
    };
  }
}
