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
#include <cstddef>
#include <functional>
#include <display/controls/control_mesh.h>
#include <display/controls/text_mesh.h>
#include "menu/renderer_state_buffers.h"
#include "menu/controls/control.h"

namespace menu {
  namespace controls {
    enum class TileAction : uint32_t { ///< Tile action type (on click)
      select = 0,
      edit,
      remove
    };
    
    // ---
    
    /// @brief UI selector tile (with edit/remove buttons)
    class Tile final : public Control {
    public:
      /// @brief Create selector tile control
      /// @param tileId    Unique tile selection identifier (profile/item ID)
      /// @param onChange  Event handler to call (with 'tileId' and TileAction) on user interaction
      Tile(RendererContext& context, const char16_t* label, int32_t x, int32_t labelY, uint32_t fixedWidth,
           const float backgroundColor[4], uint32_t tileId, std::function<void(uint32_t,TileAction)> onChange, bool addButtons)
        : tileId(tileId),
          onChange(std::move(onChange)) {
        init(context, label, x, labelY, fixedWidth, backgroundColor, addButtons);
      }

      Tile() = default;
      Tile(const Tile&) = delete;
      Tile(Tile&&) noexcept = default;
      Tile& operator=(const Tile&) = delete;
      Tile& operator=(Tile&&) noexcept = default;
      ~Tile() noexcept { release(); }

      inline void release() noexcept {
        controlMesh.release();
        editMesh.release();
        deleteMesh.release();
        labelTopMesh.release();
        labelBottomMesh.release();
      }
      ControlType type() const noexcept override;

      // -- accessors --

      inline int32_t x() const noexcept { return controlMesh.x(); }
      inline int32_t y() const noexcept { return controlMesh.y(); }
      inline int32_t hoverMarginY() const noexcept { return 0; }
      inline uint32_t width() const noexcept { return controlMesh.width(); }
      inline uint32_t height() const noexcept { return controlMesh.height(); }

      inline bool isEnabled() const noexcept { return true; } ///< Verify if control is enabled
      inline bool isHover(int32_t mouseX, int32_t mouseY) const noexcept { ///< Verify mouse hover
        return (mouseY >= y() && mouseX >= x() && mouseY < y() + (int32_t)height() && mouseX < x() + (int32_t)width());
      }
      /// @brief Get control status, based on mouse location (hover, disabled...)
      ControlStatus getStatus(int32_t mouseX, int32_t mouseY) const noexcept override;
      inline uint32_t id() const noexcept { return tileId; } ///< Get tile ID

      // -- operations --

      /// @brief Report click to the control (on mouse click with hover)
      bool click(RendererContext& context, int32_t mouseX, int32_t mouseY) override;
      inline void edit() { if(onChange) onChange(tileId, TileAction::edit); }     ///< Trigger tile-edit event (on keyboard/pad action)
      inline void remove() { if(onChange) onChange(tileId, TileAction::remove); } ///< Trigger tile-remove event (on keyboard/pad action)

      void move(RendererContext& context, int32_t x, int32_t labelY, uint32_t fixedWidth); ///< Change control location (on window resize)

      // -- rendering --

      /// @brief Draw selector tile background + buttons (if hover)
      /// @remarks - Use 'bindGraphicsPipeline' (for control backgrounds) before call.
      ///          - It's recommended to draw all controls using the same pipeline/uniform before using the other draw calls.
      void drawBackground(RendererContext& context, int32_t mouseX, int32_t mouseY,
                          RendererStateBuffers& buffers, bool isSelected, bool isActive);
      /// @brief Draw selector tile label
      /// @remarks - Use 'bindGraphicsPipeline' (for control labels) before call.
      ///          - It's recommended to draw all labels using the same pipeline/uniform before using the other draw calls.
      void drawLabel(RendererContext& context, RendererStateBuffers& buffers, bool isSelected, bool isActive);

    private:
      void init(RendererContext& context, const char16_t* label, int32_t x, int32_t labelY,
                uint32_t fixedWidth, const float backgroundColor[4], bool addButtons);

    private:
      display::controls::ControlMesh controlMesh;
      display::controls::ControlMesh deleteMesh;
      display::controls::ControlMesh editMesh;
      display::controls::TextMesh labelTopMesh;
      display::controls::TextMesh labelBottomMesh;
      
      uint32_t tileId = 0;
      std::function<void(uint32_t,TileAction)> onChange;
    };
  }
}
