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
#include <system/align.h>
#include "display/video_api.h"
#include "display/geometry.h"

namespace display {
  namespace controls {
    /// @brief UI control vertex point
    __align_type(16, // force 16-byte memory alignment
    struct ControlVertex final {
      float position[4]; // x,y,z,w
      float color[4];    // r,g,b,a
    });
    
    // ---
    
    /// @brief UI control triangles
    /// @remarks Use uniform buffer to use a color multiplier (for hover/pressed/disabled effects)
    class ControlMesh final {
    public:
      /// @brief Create control mesh
      /// @param verticesRelPos  Vertices positionned relatively to each other, in pixels (no frame position)
      /// @param pxSizeX  Expected: ToPixelSize(frameWidth)
      /// @param pxSizeY  Expected: ToPixelSize(frameHeight)
      ControlMesh(video_api::Renderer& renderer, std::vector<ControlVertex>&& verticesRelPos,
                  const std::vector<uint32_t>& indices, const float pxSizeX, const float pxSizeY,
                  int32_t x, int32_t y, uint32_t width, uint32_t height);
      ControlMesh() = default;
      ControlMesh(ControlMesh&&) = default;
      ControlMesh& operator=(ControlMesh&&) = default;
      ~ControlMesh() noexcept { release(); }

      inline void release() noexcept { ///< Destroy mesh
        vertexBuffer.release();
        indexBuffer.release();
        verticesRelPos.clear();
      }
      
      // -- accessors --
      
      inline int32_t x() const noexcept { return x_; } ///< Left X coord
      inline int32_t y() const noexcept { return y_; } ///< Top Y coord
      inline uint32_t width() const noexcept { return width_; } ///< Total width
      inline uint32_t height() const noexcept { return height_; } ///< Total height
      inline const std::vector<ControlVertex>& relativeVertices() const noexcept { return verticesRelPos; } ///< Unpositionned geometry

      // -- operations --
      
      /// @brief Update relative vertices (same count as before required!)
      void update(video_api::Renderer& renderer, std::vector<ControlVertex>&& verticesRelPos,
                  const float pxSizeX, const float pxSizeY, int32_t x, int32_t y, uint32_t width, uint32_t height);
      /// @brief Change mesh position
      void move(video_api::Renderer& renderer, const float pxSizeX, const float pxSizeY, int32_t x, int32_t y);
      /// @brief Render mesh
      /// @warning A rendering pipeline for control rendering should be bound before call
      void draw(video_api::Renderer& renderer);
      
    private:
      video_api::Buffer<video_api::ResourceUsage::staticGpu> vertexBuffer;
      video_api::Buffer<video_api::ResourceUsage::staticGpu> indexBuffer;
      std::vector<ControlVertex> verticesRelPos;
      uint32_t indexCount = 0;
      int32_t x_ = 0;
      int32_t y_ = 0;
      uint32_t width_ = 0;
      uint32_t height_ = 0;
    };
  }
}
