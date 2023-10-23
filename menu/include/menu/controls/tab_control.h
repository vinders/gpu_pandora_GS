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
#include <vector>
#include <functional>
#include <display/controls/control_mesh.h>
#include <display/controls/text_mesh.h>
#include "menu/controls/control.h"

namespace menu {
  namespace controls {
    /// @brief UI tab management control
    class TabControl final : public Control {
    public:
      /// @brief Create tab management control
      /// @param onChange    Event handler to call (with tab index) when the active tab is changed
      TabControl(RendererContext& context, int32_t x, int32_t y, uint32_t barWidth,
                 uint32_t tabPaddingX, uint32_t tabPaddingY, uint32_t minTabWidth, const float tabsColor[4],
                 const float barColor[4], const float activeBarColor[4], const char32_t** tabLabels, size_t tabCount,
                 uint32_t selectedIndex, std::function<void(uint32_t)> onChange)
        : selectedIndex((selectedIndex < (uint32_t)tabCount) ? selectedIndex : 0),
          onChange(std::move(onChange)),
          minTabWidth(minTabWidth),
          paddingX(tabPaddingX),
          paddingY(tabPaddingY) {
        init(context, x, y, barWidth, barColor, activeBarColor, tabsColor, tabLabels, tabCount);
      }

      TabControl() = default;
      TabControl(const TabControl&) = delete;
      TabControl(TabControl&&) noexcept = default;
      TabControl& operator=(const TabControl&) = delete;
      TabControl& operator=(TabControl&&) noexcept = default;
      ~TabControl() noexcept { release(); }

      inline void release() noexcept {
        barMesh.release();
        activeBarMesh.release();
        tabLabelMeshes.clear();
      }
      ControlType Type() const noexcept override;

      // -- accessors --

      inline int32_t x() const noexcept { return barMesh.x(); }
      inline int32_t y() const noexcept { return barMesh.y(); }
      inline uint32_t width() const noexcept { return barMesh.width(); }
      inline uint32_t height() const noexcept { return barMesh.height(); }
      inline uint32_t activeTabIndex() const noexcept { return selectedIndex; }

      inline bool isHover(int32_t mouseX, int32_t mouseY) const noexcept { ///< Verify mouse hover
        return (mouseY >= y() && mouseX >= x() && mouseY < y() + (int32_t)height() && mouseX < x() + (int32_t)width());
      }

      // -- operations --

      void click(RendererContext& context, int32_t mouseX); ///< Report click to control (on mouse click with hover)
      void selectPrevious(RendererContext& context);  ///< Select previous tab if available (on keyboard/pad action)
      void selectNext(RendererContext& context);      ///< Select next tab if available (on keyboard/pad action)
      void selectIndex(RendererContext& context, uint32_t index); ///< Select tab at index if available

      void move(RendererContext& context, int32_t x, int32_t y, uint32_t barWidth); ///< Change control location (on window resize)

      // -- rendering --

      /// @brief Draw tab bar background
      /// @remarks - Use 'bindGraphicsPipeline' (for control backgrounds) and 'bindVertexUniforms' (with color modifier) before call.
      ///          - It's recommended to draw all controls using the same pipeline/uniform before using the other draw calls.
      inline void drawBackground(RendererContext& context) {
        barMesh.draw(context.renderer());
        activeBarMesh.draw(context.renderer());
      }
      /// @brief Draw tab labels
      /// @remarks - Use 'bindGraphicsPipeline' (for control labels) and 'bindFragmentUniforms' (with label colors) before call.
      ///          - It's recommended to draw all labels using the same pipeline/uniform before using the other draw calls.
      /// @warning 'hoverPressedVertexUniform' will always be bound (at least for active tab + optionally for hover)
      void drawLabels(RendererContext& context, int32_t mouseX, int32_t mouseY,
                      video_api::Buffer<video_api::ResourceUsage::staticGpu>& hoverActiveFragmentUniform);

    private:
      void init(RendererContext& context, int32_t x, int32_t y, uint32_t barWidth, const float tabsColor[4],
                const float barColor[4], const float activeBarColor[4], const char32_t** tabLabels, size_t tabCount);

    private:
      display::controls::ControlMesh barMesh;
      display::controls::ControlMesh activeBarMesh;
      std::vector<display::controls::TextMesh> tabLabelMeshes;
      uint32_t selectedIndex = 0;

      std::function<void(uint32_t)> onChange;
      uint32_t minTabWidth = 70;
      uint32_t paddingX = 16;
      uint32_t paddingY = 18;
    };
  }
}
