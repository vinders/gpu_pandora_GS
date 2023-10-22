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
#include <functional>
#include <display/controls/control_mesh.h>
#include "menu/controls/types.h"

namespace menu {
  namespace controls {
    /// @brief UI scroll-bar control
    class ScrollBar final {
    public:
      /// @brief Create scroll-bar control
      /// @param operationId Unique scroll-bar operation identifier (should be cast from an enum or constant)
      /// @param onChange    Event handler to call (with 'operationId') when the scroll-bar position changes
      /// @param enabler     Optional data/config value to which the scroll-bar state should be bound
      ScrollBar(RendererContext& context, const float barColor[4], const float thumbColor[4],
                int32_t x, int32_t y, uint32_t width, uint32_t height, std::function<void(uint32_t)> onChange,
                uint32_t screenHeightPx, uint32_t totalScrollAreaPx, uint32_t scrollStepPx)
        : onChange(std::move(onChange)),
          visibleScrollArea(screenHeightPx),
          totalScrollArea(totalScrollAreaPx),
          scrollStep(scrollStepPx) {
        init(context, barColor, thumbColor, x, y, width, height);
      }

      ScrollBar() = default;
      ScrollBar(const ScrollBar&) = delete;
      ScrollBar(ScrollBar&&) noexcept = default;
      ScrollBar& operator=(const ScrollBar&) = delete;
      ScrollBar& operator=(ScrollBar&&) noexcept = default;
      ~ScrollBar() noexcept { release(); }

      inline void release() noexcept {
        backMesh.release();
        thumbMesh.release();
        upMesh.release();
        downMesh.release();
      }

      // -- accessors --

      inline int32_t x() const noexcept { return backMesh.x(); }
      inline int32_t y() const noexcept { return backMesh.y(); }
      inline int32_t width() const noexcept { return backMesh.width(); }
      inline int32_t height() const noexcept { return backMesh.height(); }
      inline uint32_t visibleTop() const noexcept { return topPosition; }
      inline uint32_t visibleBottom() const noexcept { return (topPosition + visibleScrollArea); }

      inline bool isEnabled() const noexcept { return (visibleScrollArea < totalScrollArea); } ///< Verify if control is enabled
      inline bool isDragged() const noexcept { return (dragThumbOffsetY >= 0); } ///< Verify if thumb is currently being dragged
      inline bool isHover(int32_t mouseX, int32_t mouseY) const noexcept { ///< Verify mouse hover
        return (mouseX >= x() && mouseY >= y() && mouseX < x() + (int32_t)width() && mouseY < y() + (int32_t)height());
      }

      // -- operations --

      void click(RendererContext& context, int32_t mouseY, bool isMouseDown); ///< Report click to control (on mouse click with hover)
      void mouseMove(RendererContext& context, int32_t mouseY);       ///< Report mouse move to control (on mouse move with mouse down during drag)
      inline void mouseUp(RendererContext& context, int32_t mouseY) { ///< Report end of mouse click (after drag)
        mouseMove(context, mouseY);
        dragThumbOffsetY = noDrag();
      }

      /// @brief Report click to control (on mouse wheel move / on up/down key)
      inline void scroll(RendererContext& context, int32_t delta) {
        int32_t top = (int32_t)topPosition - delta;
        updateThumbPosition(context, (top >= 0) ? (uint32_t)top : 0);
      }
      /// @brief Scroll at a position (set top of visible area) (on keyboard/pad action)
      inline void ScrollBar::setTopPosition(RendererContext& context, uint32_t top) {
        updateThumbPosition(context, top);
      }
      /// @brief Scroll at a position (set bottom of visible area) (on keyboard/pad action)
      inline void setBottomPosition(RendererContext& context, uint32_t bottom) {
        updateThumbPosition(context, (bottom+1u >= visibleScrollArea) ? (bottom+1u - visibleScrollArea) : 0);
      }

      /// @brief Change control location + scrolling limits (on window resize)
      void move(RendererContext& context, int32_t x, int32_t y, uint32_t height,
                uint32_t screenHeightPx, uint32_t totalScrollAreaPx);

      /// @brief Draw scroll-bar background
      /// @remarks Use 'bindGraphicsPipeline' (for control backgrounds) before call.
      /// @returns True if 'hoverPressedVertexUniform' has been bound (if mouse hover on special part)
      bool drawControl(RendererContext& context, int32_t mouseX, int32_t mouseY,
                       video_api::Buffer<video_api::ResourceUsage::staticGpu>& hoverPressedVertexUniform);

    private:
      void init(RendererContext& context, const float barColor[4], const float thumbColor[4],
                int32_t x, int32_t y, uint32_t width, uint32_t height);
      void updateThumbPosition(RendererContext& context, uint32_t top);
      static constexpr inline int32_t noDrag() noexcept { return -1; }

    private:
      display::controls::ControlMesh backMesh;
      display::controls::ControlMesh thumbMesh;
      display::controls::ControlMesh upMesh;
      display::controls::ControlMesh downMesh;

      std::function<void(uint32_t)> onChange;
      uint32_t visibleScrollArea = 0;
      uint32_t totalScrollArea = 0;
      uint32_t scrollStep = 10;
      uint32_t topPosition = 0;
      uint32_t maxTopPosition = 0;

      int32_t thumbAreaY = 0;
      uint32_t thumbAreaHeight = 0;
      int32_t dragThumbOffsetY = noDrag();
    };
  }
}
