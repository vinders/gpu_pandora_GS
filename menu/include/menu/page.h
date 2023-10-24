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
#include <system/align.h>
#include "menu/renderer_context.h"
#include "menu/renderer_state_buffers.h"
#include "menu/controls/control.h"

namespace menu {
  class ControlRegistration final {
  public:
    template <typename CtrlT>
    ControlRegistration(CtrlT& control, bool isInScrollableArea) noexcept
      : target(&control),
        top(control.y()),
        bottom(control.y() + (int32_t)control.height())
        left(control.x()),
        right(control.x() + (int32_t)control.width()),
        isScrollable(isInScrollableArea) {}
    ControlRegistration() noexcept = default;
    ControlRegistration(const ControlRegistration&) = default;
    ControlRegistration(ControlRegistration&&) noexcept = default;
    ControlRegistration& operator=(const ControlRegistration&) = default;
    ControlRegistration& operator=(ControlRegistration&&) noexcept = default;
    ~ControlRegistration() noexcept = default;

    // -- accessors --

    /// @brief Compare mouse location with control location
    /// @return * -1 if mouse is located before control (higher or to the left);
    ///         * 0 if mouse is located on control;
    ///         * 1 if mouse is located after control (lower or to the right).
    inline int compareLocation(int32_t mouseX, int32_t mouseY, int32_t scrollY) const noexcept {
      if (isScrollable)
        mouseY += scrollY;
      return (mouseY < top) ? -1 : ( (mouseY >= bottom || mouseX >= right) ? 1 : ( (mouseX < left) ? -1 : 0) );
    }
    bool isEnabled() const noexcept; ///< Verify if a control can be clicked/hovered/selected
    
    // -- operations --

    /// @brief Update control location (on window resize event)
    /// @warning Open controls must be closed BEFORE calling this
    template <typename CtrlT>
    inline void move(CtrlT& control) const noexcept {
      top = control.y();
      bottom = control.y() + (int32_t)control.height();
      left = control.x();
      right = control.x() + (int32_t)control.width();
    }

  private:
    controls::Control* target = nullptr;
    int32_t top = 0;
    int32_t bottom = 0;
    int32_t left = 0;
    int32_t right = 0;
    bool isScrollable = false;
  };

  // ---

  /// @brief UI page or tab page
  class Page {
  public:
    // -- window event --
    
    /// @brief Report page resize event
    /// @remarks Implementation should internally call protected parent 'move'
    virtual void move(int32_t x, int32_t y, uint32_t width, uint32_t height) = 0;
  
    // -- user interactions --
  
    /// @brief Report mouse down (click) -- coords relative to window
    void mouseDown(int32_t mouseX, int32_t mouseY);
    /// @brief Report mouse move -- coords relative to window
    void mouseMove(int32_t mouseX, int32_t mouseY);
    /// @brief Report mouse up (end of click) -- coords relative to window
    void mouseUp(int32_t mouseX, int32_t mouseY);
    /// @brief Report mouse wheel delta
    void mouseScroll(int32_t deltaY);
    
    /// @brief Report key down (keyboard)
    virtual void keyDown(char32_t keyCode) = 0;
    /// @brief Report virtual key down (keyboard)
    virtual void vkeyDown(uint32_t virtualKeyCode) = 0;
    /// @brief Report controller button down (pad)
    virtual void padButtonDown(uint32_t virtualKeyCode) = 0;
    
    // -- rendering --
    
    /// @brief Draw page control backgrounds
    /// @remarks Use 'bindGraphicsPipeline' (for control backgrounds) before call.
    /// @returns Presence of foregrounds to draw (true) or not
    virtual bool drawBackgrounds(RendererStateBuffers& buffers, int32_t mouseX, int32_t mouseY) = 0;
    /// @brief Draw page control foregrounds (if any)
    /// @remarks Use 'bindGraphicsPipeline' (for control backgrounds) before call.
    virtual void drawForegrounds(RendererStateBuffers& buffers, int32_t mouseX, int32_t mouseY) = 0;
    /// @brief Draw page control icons
    /// @remarks Use 'bindGraphicsPipeline' (for flat-shaded images) before call.
    virtual void drawIcons(RendererStateBuffers& buffers, int32_t mouseX, int32_t mouseY) = 0;
    /// @brief Draw page control labels
    /// @remarks Use 'bindGraphicsPipeline' (for control labels) before call.
    virtual void drawLabels(RendererStateBuffers& buffers, int32_t mouseX, int32_t mouseY) = 0;
    /// @brief Draw page control foreground labels (if any)
    /// @remarks Use 'bindGraphicsPipeline' (for control labels) before call.
    virtual void drawForegroundLabels(RendererStateBuffers& buffers, int32_t mouseX, int32_t mouseY) = 0;

  protected:
    Page(std::shared_ptr<RendererContext> context, int32_t x, int32_t y,
         uint32_t width, uint32_t visibleHeight, uint32_t totalPageHeight);
    void move(int32_t x, int32_t y, uint32_t width, uint32_t visibleHeight, uint32_t totalPageHeight);
    static constexpr inline int32_t noLineSelection() noexcept { return 0x7FFFFFFF; }

  protected:
    /*std::shared_ptr<RendererContext> context;
    std::shared_ptr<RendererStateBuffers> buffers;

    controls::ScrollBar scrollbarMesh;
    display::controls::ControlMesh lineHoverMesh;
    uint32_t scroll = 0;

    controls::Control* activeControl = nullptr;
    int32_t activeLineIndex = noLineSelection();

    int32_t x = 0;
    uint32_t width = 0;*/
  };
}
