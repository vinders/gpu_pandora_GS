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
#include "display/font.h"
#include "display/geometry.h"

namespace display {
  namespace controls {
    /// @brief Text glyph vertex point
    __align_type(16, // force 16-byte memory alignment
    struct TextVertex final {
      float position[2]; // x,y
      float coords[2];   // u,v
    });
    enum class TextAlignment : uint32_t { ///< Text alignment with x during TextMesh construction
      left = 0,
      center = 1,
      right = 2
    };
    
    // ---
    
    ///< Text glyphs triangles
    /// @remarks Use uniform buffer to set text color
    class TextMesh final {
    public:
      /// @brief Create text mesh
      /// @param pxSizeX  Expected: ToPixelSize(frameWidth)
      /// @param pxSizeY  Expected: ToPixelSize(frameHeight)
      TextMesh(video_api::Renderer& renderer, Font& font, const char32_t* text,
               const float pxSizeX, const float pxSizeY, int32_t x, int32_t y,
               TextAlignment align = TextAlignment::left);
      TextMesh() = default;
      TextMesh(TextMesh&&) = default;
      TextMesh& operator=(TextMesh&&) = default;
      ~TextMesh() noexcept { release(); }

      inline void release() noexcept { ///< Destroy mesh
        vertexBuffer.release();
        indexBuffer.release();
        glyphs.clear();
        vertices.clear();
        indices.clear();
      }

      // -- accessors --

      inline int32_t x() const noexcept { return x_; } ///< Left X coord
      inline int32_t y() const noexcept { return y_; } ///< Top Y coord
      inline uint32_t width() const noexcept { return width_; } ///< Total width
      inline uint32_t height() const noexcept { return height_; } ///< X-height
      inline const std::vector<std::shared_ptr<FontGlyph> >& meshGlyphs() const noexcept { return glyphs; } ///< Current mesh glyphs

      // -- operations --
      
      /// @brief Change mesh position
      void move(video_api::Renderer& renderer, const float pxSizeX, const float pxSizeY, int32_t x, int32_t y);
      /// @brief Create a clone of the mesh at a different location
      void cloneAtLocation(video_api::Renderer& renderer, const float pxSizeX,
                           const float pxSizeY, int32_t x, int32_t y, TextMesh& outClone);
      /// @brief Render mesh
      /// @warning A rendering pipeline for text rendering should be bound before call
      void draw(video_api::Renderer& renderer);

      /// @brief Append character to the mesh
      /// @returns true if the code was a valid character
      bool push(video_api::Renderer& renderer, Font& font, const float pxSizeX, const float pxSizeY, char32_t code);
      /// @brief Insert character before another character of the mesh
      /// @returns true if the code was a valid character + if index exists
      bool insertBefore(video_api::Renderer& renderer, Font& font, const float pxSizeX,
                        const float pxSizeY, char32_t code, uint32_t index);

      /// @brief Remove last character from the mesh (if any)
      void pop(video_api::Renderer& renderer);
      /// @brief Remove character at specified index from the mesh
      void removeAt(video_api::Renderer& renderer, const float pxSizeX, uint32_t index);

      // -- helpers --

      /// @brief Store unicode string
      static std::unique_ptr<char32_t[]> toString(const char32_t* text);
      
    private:
      video_api::Buffer<video_api::ResourceUsage::staticGpu> vertexBuffer;
      video_api::Buffer<video_api::ResourceUsage::staticGpu> indexBuffer;
      std::vector<std::shared_ptr<FontGlyph> > glyphs;
      std::vector<TextVertex> vertices;
      std::vector<uint32_t> indices;
      int32_t x_ = 0;
      int32_t y_ = 0;
      uint32_t width_ = 0;
      uint32_t height_ = 0;
    };
  }
}
