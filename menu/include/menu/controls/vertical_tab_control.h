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
#include <display/controls/icon_mesh.h>
#include <display/controls/text_mesh.h>
#include "menu/renderer_context.h"
#include "menu/renderer_state_buffers.h"

namespace menu {
  namespace controls {
    /// @brief Option for vertical tab control creation
    struct TabOption final {
      TabOption(const char32_t* name, display::ControlIconType icon)
        : name(display::controls::TextMesh::toString(name)), icon(icon) {}
      TabOption() = default;
      TabOption(const TabOption&) = default;
      TabOption(TabOption&&) noexcept = default;
      TabOption& operator=(const TabOption&) = default;
      TabOption& operator=(TabOption&&) noexcept = default;
      ~TabOption() noexcept = default;

      std::unique_ptr<char32_t[]> name = nullptr;
      display::ControlIconType icon = display::ControlIconType::none;
    };

    // ---

    /// @brief UI vertical tab management control (with icons)
    class VerticalTabControl final {
    public:
      /// @brief Create vertical tab management control
      /// @param onChange    Event handler to call (with tab index) when the active tab is changed
      VerticalTabControl(RendererContext& context, int32_t x, int32_t y, uint32_t tabWidth, uint32_t barHeight,
                         uint32_t tabPaddingY, uint32_t paddingTop, const float barColor[4], const float borderColor[4],
                         const TabOption* tabs, size_t tabCount, uint32_t selectedIndex,
                         std::function<void(uint32_t)> onChange)
        : selectedIndex((selectedIndex < (uint32_t)tabCount) ? selectedIndex : 0),
          onChange(std::move(onChange)) {
        init(context, x, y, tabWidth, barHeight, tabPaddingY, paddingTop, barColor, borderColor, tabs, tabCount);
      }

      VerticalTabControl() = default;
      VerticalTabControl(const VerticalTabControl&) = delete;
      VerticalTabControl(VerticalTabControl&&) noexcept = default;
      VerticalTabControl& operator=(const VerticalTabControl&) = delete;
      VerticalTabControl& operator=(VerticalTabControl&&) noexcept = default;
      ~VerticalTabControl() noexcept { release(); }

      inline void release() noexcept {
        barMesh.release();
        activeTabMesh.release();
        tabMeshes.clear();
      }

      // -- accessors --

      inline int32_t x() const noexcept { return barMesh.x(); }
      inline int32_t y() const noexcept { return barMesh.y(); }
      inline uint32_t width() const noexcept { return barMesh.width(); }
      inline uint32_t height() const noexcept { return barMesh.height(); }
      inline uint32_t activeTabIndex() const noexcept { return selectedIndex; }

      inline bool isHover(int32_t mouseX, int32_t mouseY) const noexcept { ///< Verify mouse hover
        return (mouseX < x() + (int32_t)width() && mouseY >= y() && mouseX >= x() && mouseY < y() + (int32_t)height());
      }

      // -- operations --

      void click(RendererContext& context, int32_t mouseY); ///< Report click to control (on mouse click with hover)
      void selectPrevious(RendererContext& context);  ///< Select previous tab if available (on keyboard/pad action)
      void selectNext(RendererContext& context);      ///< Select next tab if available (on keyboard/pad action)
      void selectIndex(RendererContext& context, uint32_t index); ///< Select tab at index if available

      void move(RendererContext& context, int32_t x, int32_t y, uint32_t barHeight); ///< Change control location (on window resize)

      // -- rendering --

      /// @brief Draw tab bar background
      /// @remarks - Use 'bindGraphicsPipeline' (for control backgrounds) and 'bindVertexUniforms' (with color modifier) before call.
      ///          - It's recommended to draw all controls using the same pipeline/uniform before using the other draw calls.
      inline void drawBackground(RendererContext& context, RendererStateBuffers& buffers) {
        buffers.bindControlBuffer(context.renderer(), ControlBufferType::regular);
        barMesh.draw(context.renderer());
        activeTabMesh.draw(context.renderer());
      }
      /// @brief Draw tab icons
      /// @remarks - Use 'bindGraphicsPipeline' (for flat-shaded images) before call.
      ///          - It's recommended to draw all icons using the same pipeline/uniform before using the other draw calls.
      void drawIcons(RendererContext& context, int32_t mouseX, int32_t mouseY, RendererStateBuffers& buffers);
      /// @brief Draw tab labels
      /// @remarks - Use 'bindGraphicsPipeline' (for control labels) before call.
      ///          - It's recommended to draw all labels using the same pipeline/uniform before using the other draw calls.
      void drawLabels(RendererContext& context, int32_t mouseX, int32_t mouseY, RendererStateBuffers& buffers);

    private:
      void init(RendererContext& context, int32_t x, int32_t y, uint32_t tabWidth, uint32_t barHeight, uint32_t paddingY,
                uint32_t paddingTop, const float barColor[4], const float borderColor[4], const TabOption* tabs, size_t tabCount);
      static constexpr inline uint32_t iconLabelMargin() noexcept { return 11u; }

      struct TabMesh final { // selectable value stored
        TabMesh(int32_t y, uint32_t height, display::controls::IconMesh&& icon, display::controls::TextMesh&& name)
          : iconMesh(std::move(icon)), nameMesh(std::move(name)), y(y), height(height) {}
        TabMesh() = default;
        TabMesh(const TabMesh&) = default;
        TabMesh(TabMesh&&) noexcept = default;
        TabMesh& operator=(const TabMesh&) = default;
        TabMesh& operator=(TabMesh&&) noexcept = default;
        ~TabMesh() noexcept {
          iconMesh.release();
          nameMesh.release();
        }

        display::controls::IconMesh iconMesh;
        display::controls::TextMesh nameMesh;
        int32_t y = 0;
        uint32_t height = 0;
      };
    private:
      display::controls::ControlMesh barMesh;
      display::controls::ControlMesh activeTabMesh;
      std::vector<TabMesh> tabMeshes;
      uint32_t selectedIndex = 0;
      std::function<void(uint32_t)> onChange;
    };
  }
}
