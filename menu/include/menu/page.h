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
#include "menu/controls/types.h"

namespace menu {
  /// @brief UI color modifier buffer
  __align_type(16, // force 16-byte memory alignment
  struct ColorUniform final {
    float modifier[4]; // r,g,b,a
  });
  /// @brief UI text color buffer
  __align_type(16, // force 16-byte memory alignment
  struct TextColorUniform final {
    float color[4]; // r,g,b,a
  });
  /// @brief UI scroll position buffer
  __align_type(16, // force 16-byte memory alignment
  struct ScrollUniform final {
    float offset[4]; // x,y,z,w
  });

  struct StateBuffers final { ///< UI state uniform buffers
    ~StateBuffers() noexcept { clear(); }

    // vertex slot 0
    video_api::Buffer<video_api::ResourceUsage::staticGpu> regularControl; ///< neutral vertex color
    video_api::Buffer<video_api::ResourceUsage::staticGpu> disabledControl;///< color modifier for disabled controls
    video_api::Buffer<video_api::ResourceUsage::staticGpu> activeControl;  ///< color modifier for active/hover controls
    video_api::Buffer<video_api::ResourceUsage::staticGpu> activeSpecialControl; ///< color modifier for special active/hover controls
    // vertex slot 1
    video_api::Buffer<video_api::ResourceUsage::staticGpu> fixedPosition;  ///< neutral scroll for fixed geometry
    //fragment slot 0
    video_api::Buffer<video_api::ResourceUsage::staticGpu> regularIcon;    ///< neutral icon opacity
    video_api::Buffer<video_api::ResourceUsage::staticGpu> disabledIcon;   ///< disabled icon opacity
    video_api::Buffer<video_api::ResourceUsage::staticGpu> fieldsetLabel;  ///< fieldset group label text color
    video_api::Buffer<video_api::ResourceUsage::staticGpu> regularLabel;   ///< neutral label text color
    video_api::Buffer<video_api::ResourceUsage::staticGpu> activeLabel;    ///< active/hover label text color
    video_api::Buffer<video_api::ResourceUsage::staticGpu> textInput;      ///< text input color
    video_api::Buffer<video_api::ResourceUsage::staticGpu> selectedValue;  ///< selected control text value
    video_api::Buffer<video_api::ResourceUsage::staticGpu> dropdownValue;  ///< drop-down control text option

    void clear() noexcept {
      regularControl.release();
      disabledControl.release();
      activeControl.release();
      activeSpecialControl.release();

      fixedPosition.release();

      regularIcon.release();
      disabledIcon.release();
      fieldsetLabel.release();
      regularLabel.release();
      activeLabel.release();
      textInput.release();
      selectedValue.release();
      dropdownValue.release();
    }
  };

  // ---

  /// @brief UI page or tab page
  class Page {
  public:
    // -- window event --
    
    /// @brief Report page resize event
    virtual void move(int32_t x, int32_t y, uint32_t width, uint32_t height) = 0;
  
    // -- user interactions --
  
    /// @brief Report mouse down (click) -- coords relative to window
    virtual void mouseDown(int32_t mouseX, int32_t mouseY) = 0;
    /// @brief Report mouse move -- coords relative to window
    virtual void mouseMove(int32_t mouseX, int32_t mouseY) = 0;
    /// @brief Report mouse up (end of click) -- coords relative to window
    virtual void mouseUp(int32_t mouseX, int32_t mouseY) = 0;
    /// @brief Report mouse wheel delta
    virtual void mouseScroll(int32_t deltaY) = 0;
    
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
    virtual bool drawBackgrounds(StateBuffers& buffers, int32_t mouseX, int32_t mouseY) = 0;
    /// @brief Draw page control foregrounds (if any)
    /// @remarks Use 'bindGraphicsPipeline' (for control backgrounds) before call.
    virtual void drawForegrounds(StateBuffers& buffers, int32_t mouseX, int32_t mouseY) = 0;
    /// @brief Draw page control icons
    /// @remarks Use 'bindGraphicsPipeline' (for flat-shaded images) before call.
    virtual void drawIcons(StateBuffers& buffers, int32_t mouseX, int32_t mouseY) = 0;
    /// @brief Draw page control labels
    /// @remarks Use 'bindGraphicsPipeline' (for control labels) before call.
    virtual void drawLabels(StateBuffers& buffers, int32_t mouseX, int32_t mouseY) = 0;
    /// @brief Draw page control foreground labels (if any)
    /// @remarks Use 'bindGraphicsPipeline' (for control labels) before call.
    virtual void drawForegroundLabels(StateBuffers& buffers, int32_t mouseX, int32_t mouseY) = 0;
  };
}
