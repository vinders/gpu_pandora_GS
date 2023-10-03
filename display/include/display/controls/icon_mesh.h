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
#include <memory>
#include <vector>
#include <system/align.h>
#include "display/video_api.h"
#include "display/geometry.h"

namespace display {
  namespace controls {
    /// @brief Icon/image vertex point
    __align_type(16, // force 16-byte memory alignment
    struct IconVertex final {
      float position[2]; // x,y
      float coords[2];   // u,v
    });
    
    // ---
    
    /// @brief UI icon triangles
    class IconMesh final {
    public:
      /// @brief Create control mesh
      /// @param texture  Icon to display entirely
      /// @param pxSizeX  Expected: ToPixelSize(frameWidth)
      /// @param pxSizeY  Expected: ToPixelSize(frameHeight)
      IconMesh(video_api::Renderer& renderer, std::shared_ptr<video_api::Texture2D> texture,
               const float pxSizeX, const float pxSizeY, int32_t x, int32_t y);
      /// @brief Create control mesh
      /// @param texture  Spritesheet containing the icon
      /// @param pxSizeX  Expected: ToPixelSize(frameWidth)
      /// @param pxSizeY  Expected: ToPixelSize(frameHeight)
      IconMesh(video_api::Renderer& renderer, std::shared_ptr<video_api::Texture2D> texture,
               const float pxSizeX, const float pxSizeY, int32_t x, int32_t y,
               uint32_t txOffsetX, uint32_t txOffsetY, uint32_t width, uint32_t height);
      IconMesh() = default;
      IconMesh(IconMesh&&) = default;
      IconMesh& operator=(IconMesh&&) = default;
      ~IconMesh() noexcept { release(); }

      inline void release() noexcept { ///< Destroy mesh
        vertexBuffer.release();
        indexBuffer.release();
        texture.reset();
      }
      
      // -- accessors --
      
      inline int32_t x() const noexcept { return x_; } ///< Left X coord
      inline int32_t y() const noexcept { return y_; } ///< Top Y coord
      inline uint32_t width() const noexcept { return width_; } ///< Total width
      inline uint32_t height() const noexcept { return height_; } ///< Total height

      // -- operations --
      
      /// @brief Change mesh position
      void move(video_api::Renderer& renderer, const float pxSizeX, const float pxSizeY, int32_t x_, int32_t y_);
      /// @brief Render mesh
      /// @warning A rendering pipeline for control rendering should be bound before call
      void draw(video_api::Renderer& renderer);
    
    private:
      video_api::Buffer<video_api::ResourceUsage::staticGpu> vertexBuffer;
      video_api::Buffer<video_api::ResourceUsage::staticGpu> indexBuffer;
      std::shared_ptr<video_api::Texture2D> texture = nullptr;
      IconVertex vertices[4]{};
      int32_t x_ = 0;
      int32_t y_ = 0;
      uint32_t width_ = 0;
      uint32_t height_ = 0;
    };
  }
}
