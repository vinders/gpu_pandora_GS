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
#include <video/window_events.h>
#include <display/controls/control_mesh.h>
#include <display/controls/icon_mesh.h>
#include <display/controls/text_mesh.h>
#include "menu/renderer_state_buffers.h"
#include "menu/controls/control.h"

namespace menu {
  namespace controls {
    enum class KeyBindingType : uint32_t { ///< Key-binding control value type(s)
      keyboard = 0x1,   ///< Keyboard key only
      controller = 0x2, ///< Controller key only
      both = 0x3        ///< Keyboard + controller key only
    };

    /// @brief UI key-binding control
    class KeyBinding final : public Control {
    public:
      /// @brief Create key-binding control
      /// @param keyboardValue  Keyboard virtual key code -or- mouse key code (button type converted to code with 'toMouseKeyCode')
      /// @param enabler     Optional data/config value to which the key-binding box state should be bound
      KeyBinding(RendererContext& context, const char16_t* label, int32_t x, int32_t labelY, uint32_t minLabelWidth,
                 uint32_t fixedWidth, const float backgroundColor[4], const KeyboardKeyColors& keyColors,
                 KeyBindingType bindingType, uint32_t keyboardValue, uint32_t controllerValue,
                 bool isUnbindable, const bool* enabler = nullptr)
        : bindingType(bindingType),
          keyboardValue_(((uint32_t)bindingType & (uint32_t)KeyBindingType::keyboard) ? keyboardValue : emptyKeyValue()),
          controllerValue_(((uint32_t)bindingType & (uint32_t)KeyBindingType::controller) ? controllerValue : emptyKeyValue()),
          enabler(enabler),
          minLabelWidth(minLabelWidth) {
        init(context, label, x, labelY, fixedWidth, backgroundColor, keyColors, isUnbindable);
      }

      KeyBinding() = default;
      KeyBinding(const KeyBinding&) = delete;
      KeyBinding(KeyBinding&&) noexcept = default;
      KeyBinding& operator=(const KeyBinding&) = delete;
      KeyBinding& operator=(KeyBinding&&) noexcept = default;
      ~KeyBinding() noexcept { release(); }

      inline void release() noexcept {
        controlMesh.release();
        unbindMesh.release();
        labelMesh.release();
        keyboardMesh.release();
        keyboardValueMesh.release();
        controllerValueMesh.release();
      }
      ControlType type() const noexcept override;

      // -- accessors --

      inline int32_t x() const noexcept { return labelMesh.x(); }
      inline int32_t y() const noexcept { return controlMesh.y() + 1; }
      inline int32_t controlX() const noexcept { return controlMesh.x(); }
      inline int32_t rightX() const noexcept { return (controlMesh.x() + (int32_t)controlMesh.width()); }
      inline int32_t labelY() const noexcept { return labelMesh.y(); }
      inline int32_t hoverMarginY() const noexcept { return 0; }

      inline uint32_t width() const noexcept { return static_cast<uint32_t>(rightX() - x()); }
      inline uint32_t height() const noexcept { return controlMesh.height(); }

      inline bool isEnabled() const noexcept { return (enabler == nullptr || *enabler); } ///< Verify if control is enabled
      inline bool isHover(int32_t mouseX, int32_t mouseY) const noexcept { ///< Verify mouse hover
        return (mouseY >= y() && mouseX >= x() && mouseY < y() + (int32_t)height() && mouseX < rightX());
      }
      /// @brief Get control status, based on mouse location (hover, disabled...)
      ControlStatus getStatus(int32_t mouseX, int32_t mouseY) const noexcept override;
      inline bool isEditMode() const noexcept { return isEditing; } ///< Verify if control is currently in edit mode

      inline uint32_t keyboardValue() const noexcept { return keyboardValue_; } ///< Get keyboard binding value (or emptyKeyValue())
      inline bool isMouseValue() const noexcept { ///< Verify if keyboardValue actually contains a mouse button (use 'fromMouseKeyCode' to convert it)
        return (keyboardValue_ > leftMouseKey() && keyboardValue_ != emptyKeyValue());
      }
      inline uint32_t controllerValue() const noexcept { return controllerValue_; } ///< Get controller binding value (or emptyKeyValue())

      // -- operations --

      /// @brief Report click to the control (on mouse click with hover -or- on keyboard/pad action)
      /// @returns True if the control is now open (virtual key listening mode)
      bool click(RendererContext& context, int32_t mouseX = noMouseCoord(), int32_t = noMouseCoord()) override;
      /// @brief Set keyboard virtual key (or emptyKeyValue() to disable)
      /// @param Keyboard  keyboard virtual key code -or- mouse key code if not left click (converted with 'toMouseKeyCode')
      /// @returns True if the control is still open (virtual key listening mode)
      bool setKeyboardValue(RendererContext& context, uint32_t virtualKeyCode);
      /// @brief Set controller virtual key (or emptyKeyValue() to disable)
      /// @returns True if the control is still open (virtual key listening mode)
      bool setControllerValue(RendererContext& context, uint32_t virtualKeyCode);
      void close() override; ///< Force-stop edit mode (on click elsewhere)

      void move(RendererContext& context, int32_t x, int32_t labelY); ///< Change control location (on window resize)

      // -- rendering --

      /// @brief Draw key-binding box background
      /// @remarks - Use 'bindGraphicsPipeline' (for control backgrounds) before call.
      ///          - It's recommended to draw all controls using the same pipeline/uniform before using the other draw calls.
      void drawBackground(RendererContext& context, int32_t mouseX, RendererStateBuffers& buffers, bool isActive);
      /// @brief Draw controller binding value
      /// @remarks - Use 'bindGraphicsPipeline' (for control icons) before call.
      ///          - It's recommended to draw all icons using the same pipeline/uniform before using the other draw calls.
      void drawIcon(RendererContext& context, RendererStateBuffers& buffers);
      /// @brief Draw label + keyboard binding value
      /// @remarks - Use 'bindGraphicsPipeline' (for control labels) before call.
      ///          - It's recommended to draw all labels using the same pipeline/uniform before using the other draw calls.
      void drawLabels(RendererContext& context, RendererStateBuffers& buffers, bool isActive);

      static constexpr inline uint32_t emptyKeyValue() noexcept { return 0x7FFFFFFF; } ///< Empty/disabled key value
      static constexpr inline int32_t noMouseCoord() noexcept { return 0x7FFFFFFF; } ///< Click coord for key/pad

      static constexpr inline uint32_t leftMouseKey() noexcept { return 0x7FFFFFF0; } ///< First mouse button key code
      static constexpr inline uint32_t toMouseKeyCode(pandora::video::MouseButton button) noexcept { ///< Mouse button to key value
        return leftMouseKey() + (uint32_t)button;
      }
      static constexpr inline pandora::video::MouseButton fromMouseKeyCode(uint32_t virtualKeyCode) noexcept { ///< Key value to mouse button
        return (pandora::video::MouseButton)(virtualKeyCode - leftMouseKey());
      }

    private:
      void init(RendererContext& context, const char16_t* label, int32_t x, int32_t labelY,
                uint32_t fixedWidth, const float color[4], const KeyboardKeyColors& keyColors, bool isUnbindable);

    private:
      display::controls::ControlMesh controlMesh;
      display::controls::ControlMesh unbindMesh;
      display::controls::TextMesh labelMesh;
      display::controls::ControlMesh keyboardMesh;
      display::controls::TextMesh keyboardValueMesh;
      display::controls::IconMesh controllerValueMesh;
      
      KeyBindingType bindingType = KeyBindingType::both;
      uint32_t keyboardValue_ = 0;
      uint32_t controllerValue_ = 0;
      bool isEditing = false;
      
      const bool* enabler = nullptr;
      uint32_t minLabelWidth = 0;
    };
  }
}
