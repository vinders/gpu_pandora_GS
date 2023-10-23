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
                                           const float rgba[4], float x, float y) {
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
                                       float x, float y, float width, float height);
      /// @brief Generate inverted triangle (down arrow) in 3 menu control vertices
      /// @param outVertexIt  Must point to an array of at least 3 control vertices
      static void fillInvertedTriangleVertices(display::controls::ControlVertex* outVertexIt, const float rgba[4],
                                               float x, float y, float width, float height);

      // -- rectangle generation --
      
      /// @brief Generate rectangle (in 4 menu control vertices)
      /// @param outVertexIt  Must point to an array of at least 4 control vertices
      /// @warning Required: x1 < x2 (positive) and y1 > y2 (negative: Y-axis goes up)
      static void fillRectangleVertices(display::controls::ControlVertex* outVertexIt, const float rgba[4],
                                        float x1, float x2, float y1, float y2);
      /// @brief Generate oblique rectangle (in 4 menu control vertices)
      /// @param outVertexIt  Must point to an array of at least 4 control vertices
      /// @param yOffset      Offset at x2 vertices added to y1 & y2
      /// @warning Required: x1 < x2 (positive) and y1 > y2 (negative: Y-axis goes up)
      static void fillObliqueRectangleVertices(display::controls::ControlVertex* outVertexIt, const float rgba[4],
                                               float x1, float x2, float y1, float y2, float yOffset);

      /// @brief Generate rectangle with left edge rounded (in 9 menu control vertices)
      ///        (expected indices: 0,1,2, 2,1,3, 2,3,4, 4,3,5, 4,5,6, 6,5,7, 6,7,8)
      /// @param outVertexIt  Must point to an array of at least 9 control vertices
      /// @warning Required: x1 < x2 (positive) and y1 > y2 (negative: Y-axis goes up)
      static void fillLeftRoundedRectangleVertices(display::controls::ControlVertex* outVertexIt, const float rgba[4],
                                                   float x1, float x2, float y1, float y2);
      /// @brief Generate rectangle with right edge rounded (in 9 menu control vertices)
      ///        (expected indices: 0,2,1, 1,2,3, 3,2,4, 3,4,5, 5,4,6, 5,6,7, 7,6,8)
      /// @param outVertexIt  Must point to an array of at least 9 control vertices
      /// @warning Required: x1 < x2 (positive) and y1 > y2 (negative: Y-axis goes up)
      static void fillRightRoundedRectangleVertices(display::controls::ControlVertex* outVertexIt, const float rgba[4],
                                                    float x1, float x2, float y1, float y2);

      /// @brief Generate rectangle with top-right corner cut (in 5 menu control vertices)
      ///        (expected indices: 0,1,2, 1,3,2, 2,3,4)
      /// @param outVertexIt  Must point to an array of at least 5 control vertices
      /// @warning Required: x1 < x2 (positive) and y1 > y2 (negative: Y-axis goes up)
      static void fillTopRightCutRectangleVertices(display::controls::ControlVertex* outVertexIt, const float rgba[4],
                                                   float x1, float x2, float y1, float y2, float cornerSize);
      /// @brief Generate rectangle borders with top-right corner cut (in 20 menu control vertices)
      /// @param outVertexIt  Must point to an array of at least 20 control vertices
      /// @warning Required: x1 < x2 (positive) and y1 > y2 (negative: Y-axis goes up)
      static void fillTopRightCutBorderVertices(display::controls::ControlVertex* outVertexIt, const float rgba[4],
                                                float x1, float x2, float y1, float y2, float cornerSize);

      /// @brief Generate rectangle with two cut corners (in 6 menu control vertices)
      ///        (expected indices: 0,1,2, 2,1,3, 2,3,4, 4,3,5)
      /// @param outVertexIt  Must point to an array of at least 6 control vertices
      /// @warning Required: x1 < x2 (positive) and y1 > y2 (negative: Y-axis goes up)
      static void fillDoubleCutRectangleVertices(display::controls::ControlVertex* outVertexIt, const float rgba[4],
                                             float x1, float x2, float y1, float y2, float cornerSize);
      /// @brief Generate rectangle borders with two cut corners (in 24 menu control vertices)
      /// @param outVertexIt  Must point to an array of at least 24 control vertices
      /// @warning Required: x1 < x2 (positive) and y1 > y2 (negative: Y-axis goes up)
      static void fillDoubleCutBorderVertices(display::controls::ControlVertex* outVertexIt, const float rgba[4],
                                              float x1, float x2, float y1, float y2, float cornerSize);

      /// @brief Resize existing rectangle horizontally (update 4 menu control vertices)
      static inline void resizeRectangleVerticesX(display::controls::ControlVertex* vertexIt, float x2) {
        (vertexIt + 1)->position[0] = x2;
        (vertexIt + 3)->position[0] = x2;
      }
      /// @brief Resize existing rectangle vertically (update 4 menu control vertices)
      static inline void resizeRectangleVerticesY(display::controls::ControlVertex* vertexIt, float y2) {
        (vertexIt + 2)->position[1] = y2;
        (vertexIt + 3)->position[1] = y2;
      }
      /// @brief Resize horizontally existing rectangle with right edge rounded (update 4 menu control vertices)
      static inline void resizeRightRoundedRectangleVerticesX(display::controls::ControlVertex* vertexIt,
                                                              float x2, float height) {
        x2 -= height*0.5f;
        (vertexIt + 7)->position[0] = x2;
        (vertexIt + 8)->position[0] = x2;
      }

      /// @brief Move existing rectangle vertically (update 4 menu control vertices)
      static inline void moveRectangleVerticesX(display::controls::ControlVertex* vertexIt, float x1, float x2) {
        vertexIt->position[0] = x1;     (++vertexIt)->position[0] = x2;
        (++vertexIt)->position[0] = x1; (++vertexIt)->position[0] = x2;
      }
      /// @brief Move existing rectangle vertically (update 4 menu control vertices)
      static inline void moveRectangleVerticesY(display::controls::ControlVertex* vertexIt, float y1, float y2) {
        vertexIt->position[1] = y1;     (++vertexIt)->position[1] = y1;
        (++vertexIt)->position[1] = y2; (++vertexIt)->position[1] = y2;
      }

      // -- circle generation --

      /// @brief Generate filled circle (in <circleVertexCount> menu control vertices)
      /// @param outVertexIt       Must point to an array of at least <circleVertexCount> control vertices
      /// @param circleVertexCount Number of vertices around the circle
      ///                          (note: total vertex count = circleVertexCount)
      static void fillCircleVertices(display::controls::ControlVertex* outVertexIt, const float rgba[4],
                                     uint32_t circleVertexCount, double radius, float centerX, float centerY);

      /// @brief Generate filled circle vertex indices (in <circleVertexCount*3> vertex indices)
      /// @param outIndexIt        Must point to an array of at least <(circleVertexCount-2)*3> indices
      /// @param circleVertexCount Number of vertices around the circle
      ///                          (note: total vertex count = circleVertexCount)
      static void fillCircleIndices(uint32_t* outIndexIt, uint32_t firstIndex, uint32_t circleVertexCount);
    };
  }
}
