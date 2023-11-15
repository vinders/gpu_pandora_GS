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
#include <display/controls/icon_mesh.h>
#include <display/controls/text_mesh.h>
#include "menu/renderer_state_buffers.h"
#include "menu/controls/control.h"

namespace menu {
  namespace controls {
    /// @brief UI button control
    class Button final : public Control {
    public:
      /// @brief Create button control
      /// @param operationId Unique button operation identifier (should be cast from an enum or constant)
      /// @param onClick     Event handler to call (with 'operationId') when the button is clicked
      /// @param enabler     Optional data/config value to which the button state should be bound
      Button(RendererContext& context, const char16_t* label, int32_t x, int32_t labelY, const ButtonStyleProperties& style,
             uint32_t operationId, std::function<void(uint32_t)> onClick, const bool* enabler = nullptr)
        : enabler(enabler),
          onClick(std::move(onClick)),
          operationId(operationId),
          paddingX(style.paddingX),
          paddingY(style.paddingY) {
        init(context, label, x, labelY, style);
      }

      Button() = default;
      Button(const Button&) = delete;
      Button(Button&&) noexcept = default;
      Button& operator=(const Button&) = delete;
      Button& operator=(Button&&) noexcept = default;
      ~Button() noexcept { release(); }

      inline void release() noexcept {
        controlMesh.release();
        iconMesh.release();
        labelMesh.release();
      }
      ControlType type() const noexcept override;

      // -- accessors --

      inline int32_t x() const noexcept { return controlMesh.x(); }
      inline int32_t y() const noexcept { return controlMesh.y(); }
      inline uint32_t width() const noexcept { return controlMesh.width(); }
      inline uint32_t height() const noexcept { return controlMesh.height(); }

      inline bool isEnabled() const noexcept { return (enabler == nullptr || *enabler); } ///< Verify if control is enabled
      inline bool isHover(int32_t mouseX, int32_t mouseY) const noexcept { ///< Verify mouse hover
        return (mouseY >= y() && mouseX >= x() - (int32_t)controlButtonMargin()
             && mouseY < y() + (int32_t)height() && mouseX < x() + (int32_t)width());
      }
      /// @brief Get control status, based on mouse location (hover, disabled...)
      ControlStatus getStatus(int32_t mouseX, int32_t mouseY) const noexcept override;

      // -- operations --

      /// @brief Report click to the control (on mouse click with hover -or- on keyboard/pad action)
      /// @returns True if the control is now open (always false)
      bool click(RendererContext& context, int32_t mouseX, int32_t) override;
      inline void click() const {
        if (isEnabled())
          onClick(operationId);
      }
      
      void move(RendererContext& context, int32_t x, int32_t labelY); ///< Change control location (on window resize)

      // -- rendering --

      /// @brief Draw button background
      /// @remarks - Use 'bindGraphicsPipeline' (for control backgrounds) before call.
      ///          - It's recommended to draw all controls using the same pipeline/uniform before using the other draw calls.
      void drawBackground(RendererContext& context, RendererStateBuffers& buffers, bool isActive);
      /// @brief Draw button icon (if any)
      /// @remarks - Use 'bindGraphicsPipeline' (for flat-shaded images) before call.
      ///          - It's recommended to draw all labels using the same pipeline/uniform before using the other draw calls.
      void drawIcon(RendererContext& context, RendererStateBuffers& buffers, bool isActive);
      /// @brief Draw button label (if any)
      /// @remarks - Use 'bindGraphicsPipeline' (for control labels) before call.
      ///          - It's recommended to draw all labels using the same pipeline/uniform before using the other draw calls.
      void drawLabel(RendererContext& context, RendererStateBuffers& buffers, bool isActive);

    private:
      void init(RendererContext& context, const char16_t* label, int32_t x, int32_t labelY, const ButtonStyleProperties& style);
      static constexpr inline uint32_t iconMarginRight() noexcept { return 4u; }

    private:
      display::controls::ControlMesh controlMesh;
      display::controls::IconMesh iconMesh;
      display::controls::TextMesh labelMesh;
      const bool* enabler = nullptr;

      std::function<void(uint32_t)> onClick;
      uint32_t operationId = 0;
      uint32_t paddingX = 0;
      uint32_t paddingY = 0;
    };
  }
}
