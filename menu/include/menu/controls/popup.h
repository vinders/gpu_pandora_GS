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
#include <display/controls/text_mesh.h>
#include "menu/color_theme.h"
#include "menu/renderer_state_buffers.h"
#include "menu/controls/button.h"

namespace menu {
  namespace controls {
    /// @brief UI confirmation popup
    class Popup final {
    public:
      /// @brief Open confirmation popup
      Popup(RendererContext& context, const ColorTheme& theme, const char16_t* title, const char16_t* message,
            std::function<void(uint32_t)> onAction, const char16_t** buttonLabels, size_t buttonCount);

      Popup() = default;
      Popup(const Popup&) = delete;
      Popup(Popup&&) noexcept = default;
      Popup& operator=(const Popup&) = delete;
      Popup& operator=(Popup&&) noexcept = default;
      ~Popup() noexcept { release(); }

      inline void release() noexcept {
        backMesh.release();
        controlMesh.release();
        titleMesh.release();
        messageMesh.release();
        buttons.clear();
      }

      // -- accessors --

      /// @brief Get index of button selected by user (or noAction() if no click occurred)
      inline int32_t actionIndex() const noexcept { return actionIndex_; }
      inline int32_t selectedIndex() const noexcept { return selectedIndex_; }
      inline bool isOpen() const noexcept { return (!buttons.empty() && actionIndex_ == noAction()); } ///< Verify if popup is still open
      inline void open(std::function<void(uint32_t)> onAction_) noexcept { ///< Force-open (if initialized)
        this->onAction = std::move(onAction_);
        actionIndex_ = noAction();
      } 
      inline void close() noexcept { actionIndex_ = 0;  } ///< Force-close (if initialized)

      // -- operations --

      /// @brief Report click to the control (on mouse click with hover -or- on keyboard/pad action)
      /// @returns True if the popup is still open
      bool click(RendererContext& context, int32_t mouseX, int32_t mouseY);
      bool clickSelectedIndex() noexcept; ///< Report keyboard/pad action

      void mouseMove(int32_t mouseX, int32_t mouseY) noexcept; ///< Report mouse move
      inline void selectIndex(uint32_t index) noexcept { ///< Move selection
        hoverIndex = -1;
        if (index < (uint32_t)buttons.size())
          selectedIndex_ = index;
      }
      inline void selectPrevious() noexcept { ///< Move selection to previous button
        if (selectedIndex_ > 0)
          hoverIndex = --selectedIndex_;
        else // -1 or 0
          hoverIndex = selectedIndex_ = 0;
      }
      inline void selectNext() noexcept { ///< Move selection to next button
        if (selectedIndex_ < (int32_t)buttons.size() - 1)
          hoverIndex = ++selectedIndex_;
      }
      void move(RendererContext& context); ///< Change control location (on window resize)

      // -- rendering --

      /// @brief Draw background and popup
      /// @remarks - Use 'bindGraphicsPipeline' (for flat-shaded images) before call.
      ///          - It's recommended to draw all labels using the same pipeline/uniform before using the other draw calls.
      void drawBackground(RendererContext& context, RendererStateBuffers& buffers);
      /// @brief Draw title, message and button labels
      /// @remarks - Use 'bindGraphicsPipeline' (for control labels) before call.
      ///          - It's recommended to draw all labels using the same pipeline/uniform before using the other draw calls.
      void drawLabels(RendererContext& context, RendererStateBuffers& buffers);

      static constexpr inline int32_t noAction() noexcept { return -1; }
      static constexpr inline uint32_t minWidth() noexcept { return 320; }
      static constexpr inline uint32_t height() noexcept { return 120; }
      
    private:
      display::controls::ControlMesh backMesh;
      display::controls::ControlMesh controlMesh;
      display::controls::TextMesh titleMesh;
      display::controls::TextMesh messageMesh;
      std::vector<controls::Button> buttons;

      int32_t actionIndex_ = noAction();
      int32_t selectedIndex_ = 0;
      int32_t hoverIndex = -1;
      std::function<void(uint32_t)> onAction;
    };
  }
}
