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
#include "menu/message_resources.h"
#include "menu/renderer_state_buffers.h"
#include "menu/controls/check_box.h"
#include "menu/controls/combo_box.h"
#include "menu/controls/fieldset.h"
#include "menu/controls/ruler.h"
#include "menu/controls/slider.h"
#include "menu/pages/page.h"

namespace menu {
  namespace pages {
    class AdvancedEffects final : public Page {
    public:
      AdvancedEffects(std::shared_ptr<RendererContext> context, std::shared_ptr<RendererStateBuffers> buffers,
                    const ColorTheme& theme, const MessageResources& localizedText,
                    int32_t x, int32_t y, uint32_t width, uint32_t height)
        : Page(std::move(context), std::move(buffers), theme, x, y, width, height, true) {
        init(theme, localizedText, x, y, width);
      }
      ~AdvancedEffects() noexcept override;

      // -- window event --

      /// @brief Report page resize event
      void move(int32_t x, int32_t y, uint32_t width, uint32_t height) override;

      // -- rendering --

      /// @brief Draw page control icons
      /// @remarks Use 'bindGraphicsPipeline' (for flat-shaded images) before call.
      void drawIcons() override;

    private:
      void init(const ColorTheme& theme, const MessageResources& localizedText, int32_t x, int32_t y, uint32_t width);
      void onValueChange(uint32_t id, uint32_t value);
      void drawPageBackgrounds(int32_t mouseX, int32_t mouseY) override;
      void drawPageLabels() override;

      static constexpr inline double gammaFromRulerValue(uint32_t value) noexcept {
        return static_cast<double>(18u + value) / 10.0;
      }
      void generateGammaPreview(const float* fieldsetControlRgba, int32_t x, int32_t y, double gamma);
      void updateGammaPreview(double gamma);

    private:
      display::controls::TextMesh title;

      controls::Fieldset gammaGroup;
      controls::Ruler gammaCorrection;
      display::controls::ControlMesh gammaPreview;
      uint32_t gammaCorrectionValue = 4; //2.2

      controls::Fieldset colorGroup;
      controls::Slider colorBitDepth;
      controls::ComboBox scanlines;
      controls::ComboBox colorFilter; // natural-vision-CRT/other-CRT/color-blind/color-masks/...
      controls::ComboBox specialShading; // cel-shading/kirsch-negative/storybook/broken glass...

      controls::Fieldset hdrGroup;
      controls::Ruler bloom;
      controls::Ruler lumaSharpen;
      uint32_t bloomValue = 0;
      uint32_t lumaSharpenValue = 0;

      controls::Fieldset specialGameFixesGroup;
      controls::CheckBox lowCompatibilityFrameRead;
      controls::CheckBox ignoreSmallFramebufferMoves;
      controls::CheckBox fakeGpuBusyStates;
      bool lowCompatibilityFrameReadValue = false;
      bool ignoreSmallFramebufferMovesValue = false;
      bool fakeGpuBusyStatesValue = false;
    };
  }
}
