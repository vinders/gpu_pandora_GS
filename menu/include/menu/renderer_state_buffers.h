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
#include <array>
#include <system/align.h>
#include <display/video_api.h>
#include "menu/color_theme.h"

namespace menu {
  enum class ControlBufferType : uint32_t { ///< Control/icon buffer type, based on control status
    regular = 0, ///< neutral control/icon
    selectedTile,///< selected tile control
    disabled,    ///< disabled control
    active,      ///< active/hover control
    activeScroll,///< active/hover scroll arrow/thumb
    coloredIcon, ///< theme-colored icon (use "regular" for neutral)
    disabledIcon,///< disabled icon
    activeIcon,  ///< active/hover icon
    regularTabIcon, ///< neutral tab icon
    activeTabIcon,  ///< active/hover tab icon
    COUNT
  };
  enum class LabelBufferType : uint32_t { ///< State buffer type, based on control type and status
    regular = 0,          ///< neutral label text color
    disabled,             ///< disabled label text color
    active,               ///< active/hover label text color
    tile,                 ///< profile tile text color
    activeTile,           ///< active/hover tile text color
    selectedTile,         ///< selected/current tile text color
    title,                ///< title label text color
    fieldset,             ///< fieldset group title text color
    tab,                  ///< neutral tab text color
    tabActive,            ///< active/hover tab text color
    verticalTab,          ///< neutral vertical tab text color
    verticalTabActive,    ///< active/hover vertical tab text color
    button,               ///< neutral button text color
    buttonDisabled,       ///< disabled button text color
    buttonActive,         ///< active/hover button text color
    textInput,            ///< neutral text input color
    textInputDisabled,    ///< disabled text input color
    comboBoxValue,        ///< selected combo-box value color
    comboBoxValueDisabled,///< disabled selected combo-box value color
    dropdownValue,        ///< combo-box drop-down option color
    keyboardKey,          ///< keyboard key text color
    keyboardKeyDisabled,          ///< keyboard key text color
    COUNT
  };
  
  // ---

  /// @brief Menu rendering - control/icon/text state uniform buffers (shared by all menu pages)
  class RendererStateBuffers final {
  public:
    RendererStateBuffers(video_api::Renderer& renderer, const ColorTheme& theme, uint32_t scaling = 1);
    
    RendererStateBuffers() noexcept = default;
    RendererStateBuffers(const RendererStateBuffers&) = delete;
    RendererStateBuffers(RendererStateBuffers&&) noexcept = default;
    RendererStateBuffers& operator=(const RendererStateBuffers&) = delete;
    RendererStateBuffers& operator=(RendererStateBuffers&&) noexcept = default;
    inline ~RendererStateBuffers() noexcept { release(); }
    
    void release() noexcept;  ///< Release all buffers (required before closing renderer)
    
    // -- buffer binding --
    
    /// @brief Unbind all uniform buffers (recommended before clear())
    void unbind(video_api::Renderer& renderer) noexcept;
    
    /// @brief Bind vertex uniform buffer for world position - fixed geometry
    inline void bindFixedLocationBuffer(video_api::Renderer& renderer,
                                        const video_api::ScissorRectangle& fullWindowArea) {
      if (scaling == 1u)
        renderer.setScissorRectangle(fullWindowArea);
      else {
        video_api::ScissorRectangle scaledArea(fullWindowArea.x()*scaling, fullWindowArea.y()*scaling,
                                               fullWindowArea.width()*scaling, fullWindowArea.height()*scaling);
        renderer.setScissorRectangle(scaledArea);
      }
      renderer.bindVertexUniforms(1, fixedPosition.handlePtr(), 1);
      isFixedPosition = true;
    }
    /// @brief Bind vertex uniform buffer for world position - scrollable geometry
    inline void bindScrollLocationBuffer(video_api::Renderer& renderer,
                                         const video_api::ScissorRectangle& scrollableArea) {
      if (scaling == 1u)
        renderer.setScissorRectangle(scrollableArea);
      else {
        video_api::ScissorRectangle scaledArea(scrollableArea.x()*scaling, scrollableArea.y()*scaling,
                                               scrollableArea.width()*scaling, scrollableArea.height()*scaling);
        renderer.setScissorRectangle(scaledArea);
      }
      renderer.bindVertexUniforms(1, scrollPosition.handlePtr(), 1);
      isFixedPosition = false;
    }
    inline bool isFixedLocationBuffer() const noexcept { return isFixedPosition; } ///< Verify if current position buffer bound
    
    /// @brief Bind vertex uniform buffer for control meshes
    void bindControlBuffer(video_api::Renderer& renderer, ControlBufferType type);
    /// @brief Bind fragment uniform buffer for icon meshes
    void bindIconBuffer(video_api::Renderer& renderer, ControlBufferType type);
    /// @brief Bind fragment uniform buffer for label meshes
    void bindLabelBuffer(video_api::Renderer& renderer, LabelBufferType type);
    
    // -- updates --
    
    /// @brief Update page scaling (on size change)
    inline void updateScaling(uint32_t scaling_) noexcept { this->scaling = scaling_; }
    /// @brief Update page scroll position in scroll uniform buffer (on scroll event)
    /// @param scrollLevelY  Scroll offset from top (pixels)
    void updateScrollBuffer(float pixelSizeY, uint32_t scrollLevelY);
    /// @brief Update buffer colors in control/icon/label uniform buffers (on theme change)
    void updateColorBuffers(video_api::Renderer& renderer, const ColorTheme& theme);

  private:
    ControlBufferType boundControlType = ControlBufferType::COUNT;
    ControlBufferType boundIconType = ControlBufferType::COUNT;
    LabelBufferType boundLabelType = LabelBufferType::COUNT;
    bool isFixedPosition = false;
    uint32_t scaling = 1;
  
    // vertex slot 1 - scroll position
    video_api::Buffer<video_api::ResourceUsage::staticGpu> fixedPosition;
    video_api::Buffer<video_api::ResourceUsage::staticGpu> scrollPosition;
    video_api::Buffer<video_api::ResourceUsage::staging> scrollPositionStaging;
    
    // vertex slot 0 - control color modifier / fragment slot 0 - icon color modifier
    std::array<video_api::Buffer<video_api::ResourceUsage::immutable>,
                                 (size_t)ControlBufferType::COUNT> controlBuffers;
    //fragment slot 0 - text background color
    std::array<video_api::Buffer<video_api::ResourceUsage::immutable>,
                                 (size_t)LabelBufferType::COUNT> labelBuffers;
  };
}
