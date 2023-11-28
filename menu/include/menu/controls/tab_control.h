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
#include "menu/renderer_context.h"
#include "menu/renderer_state_buffers.h"
#include "menu/controls/control.h"

namespace menu {
  namespace controls {
    /// @brief UI tab management control
    class TabControl final {
    public:
      /// @brief Create tab management control
      /// @param onChange    Event handler to call (with tab index) when the active tab is changed
      TabControl(RendererContext& context, int32_t x, int32_t y, uint32_t barWidth, uint32_t tabPaddingX, uint32_t tabPaddingY,
                 uint32_t minTabWidth, const TabControlColors& colors, const char16_t** tabLabels, size_t tabCount,
                 std::function<void(uint32_t)> onChange, uint32_t selectedIndex = 0)
        : selectedIndex((selectedIndex < (uint32_t)tabCount) ? selectedIndex : 0),
          onChange(std::move(onChange)),
          minTabWidth(minTabWidth),
          paddingX(tabPaddingX),
          paddingY(tabPaddingY) {
        init(context, x, y, barWidth, colors, tabLabels, tabCount);
      }

      TabControl() = default;
      TabControl(const TabControl&) = delete;
      TabControl(TabControl&&) noexcept = default;
      TabControl& operator=(const TabControl&) = delete;
      TabControl& operator=(TabControl&&) noexcept = default;
      ~TabControl() noexcept { release(); }

      inline void release() noexcept {
        barMesh.release();
        tabMeshes.clear();
      }

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

      void click(RendererContext& context, int32_t mouseX, int32_t mouseY); ///< Report click to control (on mouse click with hover)
      void selectPrevious(RendererContext& context);  ///< Select previous tab if available (on keyboard/pad action)
      void selectNext(RendererContext& context);      ///< Select next tab if available (on keyboard/pad action)
      void selectIndex(RendererContext& context, uint32_t index); ///< Select tab at index if available

      void move(RendererContext& context, int32_t x, int32_t y, uint32_t barWidth); ///< Change control location (on window resize)

      // -- rendering --

      /// @brief Draw tab bar background
      /// @remarks - Use 'bindGraphicsPipeline' (for control backgrounds) before call.
      ///          - It's recommended to draw all controls using the same pipeline/uniform before using the other draw calls.
      void drawBackground(RendererContext& context, int32_t mouseX, int32_t mouseY, RendererStateBuffers& buffers);
      /// @brief Draw tab labels
      /// @remarks - Use 'bindGraphicsPipeline' (for control labels) before call.
      ///          - It's recommended to draw all labels using the same pipeline/uniform before using the other draw calls.
      void drawLabels(RendererContext& context, int32_t mouseX, int32_t mouseY, RendererStateBuffers& buffers);

    private:
      void init(RendererContext& context, int32_t x, int32_t y, uint32_t barWidth, const TabControlColors& colors,
                const char16_t** tabLabels, size_t tabCount);
      void updateSelection(RendererContext& context, uint32_t index);

    private:
      struct TabMesh final { // selectable value stored
        TabMesh(int32_t y, uint32_t height, display::controls::ControlMesh&& background, display::controls::TextMesh&& name)
          : backgroundMesh(std::move(background)), nameMesh(std::move(name)), y(y), height(height) {}
        TabMesh() = default;
        TabMesh(const TabMesh&) = default;
        TabMesh(TabMesh&&) noexcept = default;
        TabMesh& operator=(const TabMesh&) = default;
        TabMesh& operator=(TabMesh&&) noexcept = default;
        ~TabMesh() noexcept {
          backgroundMesh.release();
          nameMesh.release();
        }

        display::controls::ControlMesh backgroundMesh;
        display::controls::TextMesh nameMesh;
        int32_t y = 0;
        uint32_t height = 0;
      };

      std::vector<TabMesh> tabMeshes;
      display::controls::ControlMesh barMesh;
      uint32_t selectedIndex = 0;

      std::function<void(uint32_t)> onChange;
      uint32_t minTabWidth = 70;
      uint32_t paddingX = 16;
      uint32_t paddingY = 18;
    };
  }
}
