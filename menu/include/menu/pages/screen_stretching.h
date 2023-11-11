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
#include "display/controls/control_mesh.h"
#include "display/controls/icon_mesh.h"
#include "menu/color_theme.h"
#include "menu/message_resources.h"
#include "menu/renderer_state_buffers.h"
#include "menu/controls/check_box.h"
#include "menu/controls/combo_box.h"
#include "menu/controls/fieldset.h"
#include "menu/controls/ruler.h"
#include "menu/controls/slider.h"
#include "menu/controls/text_box.h"
#include "menu/pages/page.h"

namespace menu {
  namespace pages {
    class ScreenStretching final : public Page {
    public:
      ScreenStretching(std::shared_ptr<RendererContext> context, std::shared_ptr<RendererStateBuffers> buffers,
                       const ColorTheme& theme, const MessageResources& localizedText,
                       int32_t x, int32_t y, uint32_t width, uint32_t height)
        : Page(std::move(context), std::move(buffers), theme, x, y, width, height, true) {
        init(theme, localizedText, x, y, width);
      }
      ~ScreenStretching() noexcept override;

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
      void init(const ColorTheme& theme, const MessageResources& localizedText, int32_t x, int32_t y, uint32_t width);
      void onChange(uint32_t id, uint32_t value);
      bool drawPageBackgrounds(int32_t mouseX, int32_t mouseY) override;
      void drawPageLabels() override;

      void generatePreview(const float* fieldsetControlRgba, int32_t x, int32_t y);
      void updatePreview();

    private:
      display::controls::TextMesh title;

      // aspect ratio
      controls::Fieldset aspectRatioGroup;
      display::controls::ControlMesh ratioPreviewScreen;
      display::controls::IconMesh ratioPreviewImage;
      controls::ComboBox aspectRatioPreset;
      controls::Ruler stretching;
      controls::Ruler cropping;
      controls::Slider pixelRatio;
      controls::Slider mirror;
      uint32_t stretchingValue = 0;
      uint32_t croppingValue = 0;

      // tech info
      controls::Fieldset displayAdjustGroup;
      controls::TextBox blackBordersX;
      controls::TextBox blackBordersY;
      controls::Ruler screenCurvature;
      controls::CheckBox showOverscanArea;
      controls::CheckBox centerX;
      controls::CheckBox centerY;
      bool isOverscanVisible = false;
      bool isCenteredX = false;
      bool isCenteredY = false;
      uint32_t screenCurvatureValue = 0;
    };
  }
}
