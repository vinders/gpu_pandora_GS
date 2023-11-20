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
#include "menu/controls/tile.h"
#include "menu/pages/page.h"

namespace menu {
  namespace pages {
    class ProfileSelector final : public Page {
    public:
      ProfileSelector(std::shared_ptr<RendererContext> context, std::shared_ptr<RendererStateBuffers> buffers,
                      const std::shared_ptr<ColorTheme>& theme, const MessageResources& localizedText,
                      int32_t x, int32_t y, uint32_t width, uint32_t height,
                      uint32_t activeProfileId, std::vector<ConfigProfile>& profiles)
        : Page(std::move(context), std::move(buffers), *theme, x, y, width, height, false, false),
        theme(theme),
        profiles(&profiles),
        activeProfileId(activeProfileId) {
        init(localizedText, x, y, width);
      }
      ~ProfileSelector() noexcept override;

      // -- window event --

      /// @brief Report page resize event
      void move(int32_t x, int32_t y, uint32_t width, uint32_t height) override;

      // -- rendering --

      /// @brief Draw page control icons
      /// @remarks Use 'bindGraphicsPipeline' (for flat-shaded images) before call.
      void drawIcons() override;

    private:
      void init(const MessageResources& localizedText, int32_t x, int32_t y, uint32_t width);
      void onButtonAction(uint32_t id);
      void onTileAction(uint32_t id, controls::TileAction type);
      void drawPageBackgrounds(int32_t mouseX, int32_t mouseY) override;
      void drawPageLabels() override;

      uint32_t getProfileIndex(uint32_t id) const noexcept;
      static constexpr inline uint32_t notFound() noexcept { return 0xFFFFFFFFu; }

    private:
      std::shared_ptr<ColorTheme> theme = nullptr;
      std::vector<ConfigProfile>* profiles = nullptr;
      uint32_t activeProfileId = 0;
      display::controls::TextMesh title;

      // profile tiles
      std::vector<controls::Tile> profileTiles;

      // actions
      controls::Button createProfile;
      controls::Button editProfile;
      controls::Button deleteProfile;
      bool isDeleteEnabled = true;

      //controls::Popup confirmationPopup;
    };
  }
}
