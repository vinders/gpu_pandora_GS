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
#include <cstring>
#include <display/controls/control_mesh.h>

namespace menu {
  namespace controls {
    /// @brief Geometry generation helpers
    class GeometryGenerator final {
    public:
      GeometryGenerator() = delete;
    
      /// @brief Fill values of a menu control vertex
      static inline void fillControlVertex(display::controls::ControlVertex& outVertex,
                                           const float rgba[4], float x, float y) noexcept {
        float* position = outVertex.position;
        *position = x;
        *(++position) = y;
        *(++position) = 0.f; // z
        *(++position) = 1.f; // w
        memcpy(outVertex.color, rgba, 4*sizeof(float));
      }

      // -- triangle generation --

      /// @brief Generate triangle (up arrow) in 3 menu control vertices
      /// @param outVertexIt  Must point to an array of at least 3 control vertices
      static void fillTriangleVertices(display::controls::ControlVertex* outVertexIt, const float rgba[4],
                                       float x, float y, float width, float height) noexcept;
      /// @brief Generate inverted triangle (down arrow) in 3 menu control vertices
      /// @param outVertexIt  Must point to an array of at least 3 control vertices
      static void fillInvertedTriangleVertices(display::controls::ControlVertex* outVertexIt, const float rgba[4],
                                               float x, float y, float width, float height) noexcept;

      // -- rectangle generation --
      
      /// @brief Generate rectangle (in 4 menu control vertices)
      /// @param outVertexIt  Must point to an array of at least 4 control vertices
      /// @warning Required: x1 < x2 (positive) and y1 > y2 (negative: Y-axis goes up)
      static void fillRectangleVertices(display::controls::ControlVertex* outVertexIt, const float rgba[4],
                                        float x1, float x2, float y1, float y2) noexcept;
      /// @brief Generate rectangle with horizontal gradient (in 4 menu control vertices)
      /// @param outVertexIt  Must point to an array of at least 4 control vertices
      /// @warning Required: x1 < x2 (positive) and y1 > y2 (negative: Y-axis goes up)
      static void fillHorizontalRectangleVertices(display::controls::ControlVertex* outVertexIt,
                                                  const float rgba1[4], const float rgba2[4],
                                                  float x1, float x2, float y1, float y2) noexcept;
      /// @brief Generate rectangle with vertical gradient (in 4 menu control vertices)
      /// @param outVertexIt  Must point to an array of at least 4 control vertices
      /// @warning Required: x1 < x2 (positive) and y1 > y2 (negative: Y-axis goes up)
      static void fillVerticalRectangleVertices(display::controls::ControlVertex* outVertexIt,
                                               const float rgba1[4], const float rgba2[4],
                                               float x1, float x2, float y1, float y2) noexcept;
      /// @brief Generate rectangle with double gradient (in 10 menu control vertices)
      ///        (expected indices: 0,1,2,2,1,3, 2,3,4,4,3,5, 6,7,8,8,7,9)
      /// @param outVertexIt  Must point to an array of at least 10 control vertices
      /// @param rgba         [0]: primary color / [1]: top lighter color / [2]: middle darker color
      /// @warning Required: x1 < x2 (positive) and y1 > y2 (negative: Y-axis goes up)
      static void fillDoubleGradientRectangleVertices(display::controls::ControlVertex* outVertexIt, const float rgba[3][4],
                                                      float x1, float x2, float y1, float y2, float topGradHeight) noexcept;
      /// @brief Generate rectangle (in 13 menu control vertices)
      ///        (expected indices: 0,1,3,0,3,5,  1,2,4,2,6,4,    3,1,7,1,4,7,  3,7,5,4,6,7,
      ///                           5,7,8,7,11,8, 5,8,10,8,11,10, 7,6,9,7,9,11, 6,12,9,9,12,11)
      /// @param outVertexIt  Must point to an array of at least 13 control vertices
      /// @warning Required: x1 < x2 (positive) and y1 > y2 (negative: Y-axis goes up)
      static void fillRadialGradientRectangleVertices(display::controls::ControlVertex* outVertexIt,
                                                      const float rgba1[4], const float rgba2[4],
                                                      float x1, float x2, float y1, float y2) noexcept;

      /// @brief Generate oblique rectangle (in 4 menu control vertices)
      /// @param outVertexIt  Must point to an array of at least 4 control vertices
      /// @param yOffset      Offset at x2 vertices added to y1 & y2
      /// @warning Required: x1 < x2 (positive) and y1 > y2 (negative: Y-axis goes up)
      static void fillObliqueRectangleVertices(display::controls::ControlVertex* outVertexIt, const float rgba[4],
                                               float x1, float x2, float y1, float y2, float yOffset) noexcept;

      /// @brief Generate rectangle with left edge rounded (in 9 menu control vertices)
      ///        (expected indices: 0,1,2, 2,1,3, 2,3,4, 4,3,5, 4,5,6, 6,5,7, 6,7,8)
      /// @param outVertexIt  Must point to an array of at least 9 control vertices
      /// @warning Required: x1 < x2 (positive) and y1 > y2 (negative: Y-axis goes up)
      static void fillLeftRoundedRectangleVertices(display::controls::ControlVertex* outVertexIt, const float rgba[4],
                                                   float x1, float x2, float y1, float y2) noexcept;
      /// @brief Generate rectangle with right edge rounded (in 9 menu control vertices)
      ///        (expected indices: 0,2,1, 1,2,3, 3,2,4, 3,4,5, 5,4,6, 5,6,7, 7,6,8)
      /// @param outVertexIt  Must point to an array of at least 9 control vertices
      /// @warning Required: x1 < x2 (positive) and y1 > y2 (negative: Y-axis goes up)
      static void fillRightRoundedRectangleVertices(display::controls::ControlVertex* outVertexIt, const float rgba[4],
                                                    float x1, float x2, float y1, float y2) noexcept;
      
      /// @brief Generate rectangle with rounded edges (in <getRoundedRectangleVertexCount(radius)> menu control vertices)
      /// @param outVertexIt  Must point to an array of at least <getRoundedRectangleVertexCount(radius)> control vertices
      /// @warning Required: x1 < x2 (positive) and y1 > y2 (negative: Y-axis goes up)
      static void fillRoundedRectangleVertices(display::controls::ControlVertex* outVertexIt, const float rgba[4],
                                               float x1, float x2, float y1, float y2, float radius) noexcept;
      /// @brief Generate rectangle with rounded edges vertex indices (in <getRoundedRectangleVertexIndexCount(radius)> vertex indices)
      /// @param outIndexIt  Must point to an array of at least <getRoundedRectangleVertexIndexCount(radius)> indices
      static void fillRoundedRectangleIndices(uint32_t* outIndexIt, uint32_t firstVertexIndex, float radius) noexcept;
      /// @brief Count vertices required to draw a rectangle with rounded edges
      static inline uint32_t getRoundedRectangleVertexCount(float radius) noexcept {
        return 4u + (static_cast<uint32_t>(radius + 0.5f) << 3);
      }
      /// @brief Count vertex indices required to draw a rectangle with rounded edges
      static inline uint32_t getRoundedRectangleVertexIndexCount(float radius) noexcept {
        return ((getRoundedRectangleVertexCount(radius) >> 1) - 1u)*6u;
      }

      /// @brief Generate rectangle with top-right corner cut and double gradient (in 10 menu control vertices)
      ///        (expected indices: 0,1,2,2,1,3, 2,3,4,4,3,5, 6,7,8,8,7,9)
      /// @param outVertexIt  Must point to an array of at least 10 control vertices
      /// @param rgba         [0]: primary color / [1]: top lighter color / [2]: middle darker color
      /// @warning Required: x1 < x2 (positive) and y1 > y2 (negative: Y-axis goes up)
      static void fillTopRightCutRectangleVertices(display::controls::ControlVertex* outVertexIt, const float rgba[3][4],
                                                   float x1, float x2, float y1, float y2, float cornerSize) noexcept;
      /// @brief Generate rectangle with two cut corners (in 6 menu control vertices)
      ///        (expected indices: 0,1,2, 2,1,3, 2,3,4, 4,3,5)
      /// @param outVertexIt  Must point to an array of at least 6 control vertices
      /// @warning Required: x1 < x2 (positive) and y1 > y2 (negative: Y-axis goes up)
      static void fillDoubleCutRectangleVertices(display::controls::ControlVertex* outVertexIt, const float rgba[4],
                                                 float x1, float x2, float y1, float y2, float cornerSize) noexcept;
      /// @brief Generate rectangle with all corners cut (in 8 menu control vertices)
      ///        (expected indices: 0,1,2, 2,1,3,  2,3,4, 4,3,5,  4,5,6, 6,5,7)
      /// @param outVertexIt  Must point to an array of at least 8 control vertices
      /// @warning Required: x1 < x2 (positive) and y1 > y2 (negative: Y-axis goes up)
      static void fillCornerCutRectangleVertices(display::controls::ControlVertex* outVertexIt, const float rgba[4],
                                                 float x1, float x2, float y1, float y2, float cornerSize) noexcept;

      /// @brief Generate rectangle borders (in 16 menu control vertices)
      /// @param outVertexIt  Must point to an array of at least 16 control vertices
      /// @warning Required: x1 < x2 (positive) and y1 > y2 (negative: Y-axis goes up)
      static void fillRectangleBorderVertices(display::controls::ControlVertex* outVertexIt, const float rgba[4],
                                              float x1, float x2, float y1, float y2) noexcept;
      /// @brief Generate rectangle borders with top-right corner cut (in 20 menu control vertices)
      /// @param outVertexIt  Must point to an array of at least 20 control vertices
      /// @warning Required: x1 < x2 (positive) and y1 > y2 (negative: Y-axis goes up)
      static void fillTopRightCutBorderVertices(display::controls::ControlVertex* outVertexIt, const float rgba[4],
                                                float x1, float x2, float y1, float y2, float cornerSize) noexcept;
      /// @brief Generate rectangle borders with two cut corners (in 24 menu control vertices)
      /// @param outVertexIt  Must point to an array of at least 24 control vertices
      /// @warning Required: x1 < x2 (positive) and y1 > y2 (negative: Y-axis goes up)
      static void fillDoubleCutBorderVertices(display::controls::ControlVertex* outVertexIt, const float rgba[4],
                                              float x1, float x2, float y1, float y2, float cornerSize) noexcept;

      // -- rectangle resize --

      /// @brief Resize existing rectangle horizontally (update 4 menu control vertices)
      static inline void resizeRectangleVerticesX(display::controls::ControlVertex* vertexIt, float x2) noexcept {
        (vertexIt + 1)->position[0] = x2;
        (vertexIt + 3)->position[0] = x2;
      }
      /// @brief Resize existing rectangle vertically (update 4 menu control vertices)
      static inline void resizeRectangleVerticesY(display::controls::ControlVertex* vertexIt, float y2) noexcept {
        (vertexIt + 2)->position[1] = y2;
        (vertexIt + 3)->position[1] = y2;
      }
      /// @brief Resize existing rectangle horizontally and vertically (update 4 menu control vertices)
      static inline void resizeRectangleVertices(display::controls::ControlVertex* vertexIt, float x2, float y2) noexcept {
        (vertexIt + 1)->position[0] = x2;
        (vertexIt + 2)->position[1] = y2;
        auto* bottomRightPosition = (vertexIt + 3)->position;
        bottomRightPosition[0] = x2;
        bottomRightPosition[1] = y2;
      }
      /// @brief Resize horizontally existing rectangle with right edge rounded (update 4 menu control vertices)
      static inline void resizeRightRoundedRectangleVerticesX(display::controls::ControlVertex* vertexIt,
                                                              float x2, float height) noexcept {
        x2 -= height*0.5f;
        (vertexIt + 7)->position[0] = x2;
        (vertexIt + 8)->position[0] = x2;
      }
      /// @brief Resize horizontally existing rectangle with all edges rounded (update <4 + 8*radius> menu control vertices)
      static void resizeRoundedRectangleVerticesX(display::controls::ControlVertex* vertexIt, float x2, float radius) noexcept;
      /// @brief Resize horizontally existing rectangle with all corners cut (update 8 menu control vertices)
      static inline void resizeCornerCutRectangleVerticesX(display::controls::ControlVertex* vertexIt,
                                                           float x2, float cornerSize) noexcept {
        (vertexIt + 1)->position[0] = x2 - cornerSize;
        (vertexIt + 3)->position[0] = x2;
        (vertexIt + 5)->position[0] = x2;
        (vertexIt + 7)->position[0] = x2 - cornerSize;
      }
      /// @brief Resize existing rectangle with radial gradient (update 13 menu control vertices)
      static void resizeRadialGradientRectangleVertices(display::controls::ControlVertex* vertexIt,
                                                        float x2, float y2) noexcept;

      /// @brief Move existing rectangle vertically (update 4 menu control vertices)
      static inline void moveRectangleVerticesX(display::controls::ControlVertex* vertexIt, float x1, float x2) noexcept {
        vertexIt->position[0] = x1;     (++vertexIt)->position[0] = x2;
        (++vertexIt)->position[0] = x1; (++vertexIt)->position[0] = x2;
      }
      /// @brief Move existing rectangle vertically (update 4 menu control vertices)
      static inline void moveRectangleVerticesY(display::controls::ControlVertex* vertexIt, float y1, float y2) noexcept {
        vertexIt->position[1] = y1;     (++vertexIt)->position[1] = y1;
        (++vertexIt)->position[1] = y2; (++vertexIt)->position[1] = y2;
      }

      // -- cross generation --
      
      /// @brief Generate filled cross (in 8 menu control vertices)
      ///        (expected indices: 0,1,2,2,1,3, 4,5,6,6,5,7)
      /// @param outVertexIt       Must point to an array of at least 8 control vertices
      /// @warning Required: x1 < x2 (positive) and y1 > y2 (negative: Y-axis goes up)
      static void fillCrossVertices(display::controls::ControlVertex* outVertexIt, const float rgba[4],
                                    float x1, float x2, float y1, float y2) noexcept;

      // -- circle generation --

      /// @brief Generate filled circle (in <circleVertexCount> menu control vertices)
      /// @param outVertexIt       Must point to an array of at least <circleVertexCount> control vertices
      /// @param circleVertexCount Number of vertices around the circle
      ///                          (note: total vertex count = circleVertexCount)
      static void fillCircleVertices(display::controls::ControlVertex* outVertexIt, const float rgba[4],
                                     uint32_t circleVertexCount, double radius, float centerX, float centerY) noexcept;
      /// @brief Generate filled circle vertex indices (in <getCircleVertexIndexCount(circleVertexCount)> vertex indices)
      /// @param outIndexIt        Must point to an array of at least <getCircleVertexIndexCount(circleVertexCount)> indices
      /// @param circleVertexCount Number of vertices around the circle
      ///                          (note: total vertex count = circleVertexCount)
      static void fillCircleIndices(uint32_t* outIndexIt, uint32_t firstVertexIndex, uint32_t circleVertexCount) noexcept;
      /// @brief Count vertex indices required to draw a circle
      static inline uint32_t getCircleVertexIndexCount(uint32_t circleVertexCount) noexcept {
        return (circleVertexCount-2u)*3u;
      }
    };
  }
}
