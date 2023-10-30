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

#include <cassert>
#include <cstdint>
#include <vector>
#include <display/controls/control_mesh.h>
#include "menu/renderer_context.h"
#include "menu/renderer_state_buffers.h"
#include "menu/controls/control.h"
#include "menu/controls/scroll_bar.h"
#include "menu/controls/tooltip.h"

namespace menu {
  class ControlRegistration final { ///< Interactive control registration (to allow hover/click/drag/select)
  public:
    template <typename CtrlT>
    ControlRegistration(CtrlT& control, bool isInScrollableArea, const char32_t* tooltip = nullptr) noexcept
      : target(&control),
        top(control.y()),
        bottom(control.y() + (int32_t)control.height()),
        left(control.x()),
        right(control.x() + (int32_t)control.width()),
        isScrollable(isInScrollableArea),
        tooltip(tooltip) {}
    ControlRegistration() noexcept = default;
    ControlRegistration(const ControlRegistration&) = default;
    ControlRegistration(ControlRegistration&&) noexcept = default;
    ControlRegistration& operator=(const ControlRegistration&) = default;
    ControlRegistration& operator=(ControlRegistration&&) noexcept = default;
    ~ControlRegistration() noexcept = default;

    // -- accessors --

    inline int32_t x() const noexcept { return left; }  ///< Horizontal left location
    inline int32_t rightX() const noexcept { return right; } ///< Horizontal right location
    inline int32_t width() const noexcept { return static_cast<uint32_t>(right - left); } ///< Horizontal size
    inline int32_t y() const noexcept { return top; } ///< Vertical top location
    inline int32_t bottomY() const noexcept { return bottom; } ///< Vertical bottom location
    inline uint32_t height() const noexcept { return static_cast<uint32_t>(bottom - top); } ///< Vertical size
    inline const controls::Control* control() const noexcept { return target; } ///< Access target control
    inline controls::Control* control() noexcept { return target; }             ///< Access target control

    /// @brief Get current control status
    inline controls::ControlStatus controlStatus(int32_t mouseX, int32_t mouseY, int32_t scrollY) const noexcept {
      return target->getStatus(mouseX, isScrollable ? mouseY + scrollY : mouseY);
    }
    /// @brief Get tooltip message associated with the control (or NULL if no message exists)
    inline const char32_t* tooltipMessage() const noexcept { return tooltip; }

    /// @brief Compare mouse location with control location
    /// @return * -1 if control is located before mouse location (higher or to the left);
    ///         * 0 if control is located at mouse location;
    ///         * 1 if control is located after mouse location (lower or to the right).
    inline int compareLocation(int32_t mouseX, int32_t mouseY, int32_t scrollY) const noexcept {
      if (isScrollable)
        mouseY += scrollY;
      return (mouseY < top) ? 1 : ( (mouseY >= bottom || mouseX >= right) ? -1 : ( (mouseX < left) ? 1 : 0) );
    }
    inline bool isFixed() const noexcept { return !isScrollable; } ///< Verify if a control has a fixed (non-scrollable) position
    
    // -- operations --

    /// @brief Update control location (on window resize event)
    /// @warning Open controls must be closed BEFORE calling this
    template <typename CtrlT>
    inline void updateLocation(CtrlT& control) const noexcept {
      assert(target == &control);
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
    const char32_t* tooltip = nullptr;
  };

  // ---

  /// @brief UI page or tab page
  class Page {
  public:
    virtual ~Page() noexcept;

    // -- accessors --

    inline int32_t x() const noexcept { return backgroundMesh.x(); }
    inline int32_t y() const noexcept { return backgroundMesh.y(); }
    inline uint32_t width() const noexcept { return backgroundMesh.width(); }
    inline uint32_t height() const noexcept { return backgroundMesh.height(); }
    inline int32_t scrollLevel() const noexcept { return scrollY; }
    inline uint32_t contentHeight() const noexcept {
      return tooltip.width() ? static_cast<uint32_t>(tooltip.y() - scrollbar.y()) : scrollbar.height();
    }

    // -- window event --
    
    /// @brief Report page resize event
    /// @remarks Implementation should internally call protected parent 'moveBase'
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
    /// @brief Report mouse leaving screen
    void mouseLeave() noexcept;
    
    /// @brief Report key down (keyboard)
    void keyDown(char32_t keyCode);
    /// @brief Report virtual key down (keyboard)
    void vkeyDown(uint32_t virtualKeyCode);
    /// @brief Report controller button down (pad)
    void padButtonDown(uint32_t virtualKeyCode);
    
    // -- rendering --
    
    /// @brief Draw page control backgrounds
    /// @remarks Use 'bindGraphicsPipeline' (for control backgrounds) before call.
    /// @returns Presence of foregrounds to draw (true) or not
    bool drawBackgrounds();
    /// @brief Draw page control icons
    /// @remarks Use 'bindGraphicsPipeline' (for flat-shaded images) before call.
    virtual void drawIcons() = 0;
    /// @brief Draw page control labels
    /// @remarks Use 'bindGraphicsPipeline' (for control labels) before call.
    void drawLabels();

    /// @brief Draw page control foregrounds (if any) -- should only be called if 'drawBackgrounds' returns true
    /// @remarks Use 'bindGraphicsPipeline' (for control backgrounds) before call.
    virtual void drawForegrounds() = 0;
    /// @brief Draw page control foreground labels (if any) -- should only be called if 'drawBackgrounds' returns true
    /// @remarks Use 'bindGraphicsPipeline' (for control labels) before call.
    virtual void drawForegroundLabels() = 0;

  protected:
    Page(std::shared_ptr<RendererContext> context, std::shared_ptr<RendererStateBuffers> buffers,
         const ColorTheme& theme, int32_t x, int32_t y, uint32_t width, uint32_t visibleHeight, bool enableTooltip);
    void moveBase(int32_t x, int32_t y, uint32_t width, uint32_t visibleHeight);
    inline void moveScrollbarThumb(int32_t bottomY) {
      scrollbar.moveThumb(*context, static_cast<uint32_t>(bottomY - scrollbar.y()) + tooltip.height()); // will call onScroll if needed
    }
    void onScroll(uint32_t visibleTopY);
    void onHover(int32_t controlIndex);

    // Declare interactive controls in order (top->bottom then left->right)
    // -> note: fixed/non-scrollable controls must be at the beginning (top) or end of the vector (bottom)
    inline void registerControls(std::vector<ControlRegistration>&& controlsOrderedByLocation) {
      controlRegistry = std::move(controlsOrderedByLocation);
    }
    inline const controls::Control* getActiveControl() const noexcept {
      return (activeControlIndex != noControlSelection()) ? controlRegistry[activeControlIndex].control() : nullptr;
    }
    inline const controls::Control* getOpenControl() const noexcept {
      return openControl ? openControl->control() : nullptr;
    }

    int32_t findActiveControlIndex(int32_t mouseX, int32_t mouseY) const noexcept;
    void selectPreviousControlIndex() noexcept;
    void selectNextControlIndex() noexcept;
    void adaptControlSelection(int32_t controlIndex, ControlRegistration* control) noexcept;
    static constexpr inline int32_t noControlSelection() noexcept { return -1; }

    virtual bool drawPageBackgrounds(int32_t mouseX, int32_t mouseY) = 0;
    virtual void drawPageLabels() = 0;

  protected:
    std::shared_ptr<RendererContext> context = nullptr;
    std::shared_ptr<RendererStateBuffers> buffers = nullptr;
  private:
    controls::ScrollBar scrollbar;
    controls::Tooltip tooltip;
    int32_t scrollY = 0;

    display::controls::ControlMesh backgroundMesh;
    display::controls::ControlMesh controlHoverMesh;
    std::vector<ControlRegistration> controlRegistry;
    ControlRegistration* openControl = nullptr;
    int32_t activeControlIndex = noControlSelection();
    int32_t mouseX_ = -1;
    int32_t mouseY_ = -1;
  };
}
