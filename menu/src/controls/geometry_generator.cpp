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
#define _USE_MATH_DEFINES
#include <cmath>
#include "menu/controls/geometry_generator.h"

using namespace display::controls;
using namespace menu::controls;


// -- triangle generation -- ---------------------------------------------------

void GeometryGenerator::fillTriangleVertices(ControlVertex* outVertexIt, const float rgba[4],
                                             float x, float y, float width, float height) noexcept {
  fillControlVertex(*outVertexIt,     rgba, x,              y - height);
  fillControlVertex(*(++outVertexIt), rgba, x + width*0.5f, y);
  fillControlVertex(*(++outVertexIt), rgba, x + width,      y - height);
}

void GeometryGenerator::fillInvertedTriangleVertices(ControlVertex* outVertexIt, const float rgba[4],
                                                     float x, float y, float width, float height) noexcept {
  fillControlVertex(*outVertexIt,     rgba, x,              y);
  fillControlVertex(*(++outVertexIt), rgba, x + width,      y);
  fillControlVertex(*(++outVertexIt), rgba, x + width*0.5f, y - height);
}


// -- rectangle generation -- --------------------------------------------------

void GeometryGenerator::fillRectangleVertices(ControlVertex* outVertexIt, const float rgba[4],
                                              float x1, float x2, float y1, float y2) noexcept {
  fillControlVertex(*outVertexIt,     rgba, x1, y1);
  fillControlVertex(*(++outVertexIt), rgba, x2, y1);
  fillControlVertex(*(++outVertexIt), rgba, x1, y2);
  fillControlVertex(*(++outVertexIt), rgba, x2, y2);
}

void GeometryGenerator::fillHorizontalRectangleVertices(ControlVertex* outVertexIt, const float rgba1[4], const float rgba2[4],
                                              float x1, float x2, float y1, float y2) noexcept {
  fillControlVertex(*outVertexIt,     rgba1, x1, y1);
  fillControlVertex(*(++outVertexIt), rgba2, x2, y1);
  fillControlVertex(*(++outVertexIt), rgba1, x1, y2);
  fillControlVertex(*(++outVertexIt), rgba2, x2, y2);
}

void GeometryGenerator::fillVerticalRectangleVertices(ControlVertex* outVertexIt, const float rgba1[4], const float rgba2[4],
                                                      float x1, float x2, float y1, float y2) noexcept {
  fillControlVertex(*outVertexIt,     rgba1, x1, y1);
  fillControlVertex(*(++outVertexIt), rgba1, x2, y1);
  fillControlVertex(*(++outVertexIt), rgba2, x1, y2);
  fillControlVertex(*(++outVertexIt), rgba2, x2, y2);
}

void GeometryGenerator::fillDoubleGradientRectangleVertices(ControlVertex* outVertexIt, const float rgba[3][4],
                                                            float x1, float x2, float y1, float y2, float topGradHeight) noexcept {
  const float yLine = y1 - topGradHeight - 2.f;
  fillControlVertex(*outVertexIt,     rgba[1], x1, y1);
  fillControlVertex(*(++outVertexIt), rgba[1], x2, y1);
  fillControlVertex(*(++outVertexIt), rgba[0], x1, y1 - topGradHeight);
  fillControlVertex(*(++outVertexIt), rgba[0], x2, y1 - topGradHeight);
  fillControlVertex(*(++outVertexIt), rgba[0], x1, yLine);
  fillControlVertex(*(++outVertexIt), rgba[0], x2, yLine);
  fillControlVertex(*(++outVertexIt), rgba[2], x1, yLine);
  fillControlVertex(*(++outVertexIt), rgba[2], x2, yLine);
  fillControlVertex(*(++outVertexIt), rgba[0], x1, y2);
  fillControlVertex(*(++outVertexIt), rgba[0], x2, y2);
}

void GeometryGenerator::fillObliqueRectangleVertices(ControlVertex* outVertexIt, const float rgba[4],
                                                     float x1, float x2, float y1, float y2, float yOffset) noexcept {
  fillControlVertex(*outVertexIt,     rgba, x1, y1);
  fillControlVertex(*(++outVertexIt), rgba, x2, y1 + yOffset);
  fillControlVertex(*(++outVertexIt), rgba, x1, y2);
  fillControlVertex(*(++outVertexIt), rgba, x2, y2 + yOffset);
}

void GeometryGenerator::fillRectangleBorderVertices(ControlVertex* outVertexIt, const float rgba[4],
                                                    float x1, float x2, float y1, float y2) noexcept {
  fillControlVertex(*outVertexIt,     rgba, x1, y1);
  fillControlVertex(*(++outVertexIt), rgba, x2, y1);
  fillControlVertex(*(++outVertexIt), rgba, x1, y1 - 1.f);
  fillControlVertex(*(++outVertexIt), rgba, x2, y1 - 1.f);
  fillControlVertex(*(++outVertexIt), rgba, x1, y2 + 1.f);
  fillControlVertex(*(++outVertexIt), rgba, x2, y2 + 1.f);
  fillControlVertex(*(++outVertexIt), rgba, x1, y2);
  fillControlVertex(*(++outVertexIt), rgba, x2, y2);
  y1 -= 1.f;
  y2 += 1.f;
  fillControlVertex(*(++outVertexIt), rgba, x1,       y1);
  fillControlVertex(*(++outVertexIt), rgba, x1 + 1.f, y1);
  fillControlVertex(*(++outVertexIt), rgba, x1,       y2);
  fillControlVertex(*(++outVertexIt), rgba, x1 + 1.f, y2);
  fillControlVertex(*(++outVertexIt), rgba, x2 - 1.f, y1);
  fillControlVertex(*(++outVertexIt), rgba, x2,       y1);
  fillControlVertex(*(++outVertexIt), rgba, x2 - 1.f, y2);
  fillControlVertex(*(++outVertexIt), rgba, x2,       y2);
}

// ---

void GeometryGenerator::fillLeftRoundedRectangleVertices(ControlVertex* outVertexIt, const float rgba[4],
                                                         float x1, float x2, float y1, float y2) noexcept {
  double radius = fabs((double)y2 - (double)y1)*0.5;
  float centerX = x1 + (float)radius;
  float centerY = y1 - (float)radius;
  fillControlVertex(*outVertexIt,     rgba, x1, centerY);

  constexpr const double sector = 2.0*M_PI / 12.0;
  for (double i = M_PI - sector; i > M_PI_2 + 0.001; i -= sector) {
    double coordX = cos(i)*radius;
    double coordY = sin(i)*radius;
    fillControlVertex(*(++outVertexIt), rgba, centerX + (float)coordX, centerY + (float)coordY);
    fillControlVertex(*(++outVertexIt), rgba, centerX + (float)coordX, centerY - (float)coordY);
  }

  fillControlVertex(*(++outVertexIt), rgba, centerX, y1);
  fillControlVertex(*(++outVertexIt), rgba, centerX, y2);
  fillControlVertex(*(++outVertexIt), rgba, x2, y1);
  fillControlVertex(*(++outVertexIt), rgba, x2, y2);
}

void GeometryGenerator::fillRightRoundedRectangleVertices(ControlVertex* outVertexIt, const float rgba[4],
                                                          float x1, float x2, float y1, float y2) noexcept {
  double radius = fabs((double)y2 - (double)y1)*0.5;
  float centerX = x2 - (float)radius;
  float centerY = y1 - (float)radius;

  fillControlVertex(*outVertexIt,     rgba, x1, y1);
  fillControlVertex(*(++outVertexIt), rgba, x1, y2);
  fillControlVertex(*(++outVertexIt), rgba, centerX, y1);
  fillControlVertex(*(++outVertexIt), rgba, centerX, y2);

  constexpr const double sector = 2.0*M_PI / 12.0;
  for (double i = M_PI_2 - sector; i > 0.001; i -= sector) {
    double coordX = cos(i)*radius;
    double coordY = sin(i)*radius;
    fillControlVertex(*(++outVertexIt), rgba, centerX + (float)coordX, centerY + (float)coordY);
    fillControlVertex(*(++outVertexIt), rgba, centerX + (float)coordX, centerY - (float)coordY);
  }
  fillControlVertex(*(++outVertexIt), rgba, x2, centerY);
}

void GeometryGenerator::fillRoundedRectangleVertices(display::controls::ControlVertex* outVertexIt, const float rgba[4],
                                                     float x1, float x2, float y1, float y2, float radius) noexcept {
  const uint32_t vertexCount = getRoundedRectangleVertexCount(radius);
  const double sector = 2.0*M_PI / (double)(vertexCount - 4u);

  const float centerX1 = x1 + radius;
  const float centerX2 = x2 - radius;
  float centerY = y1 - radius;
  fillControlVertex(*outVertexIt,     rgba, centerX1, y1);
  fillControlVertex(*(++outVertexIt), rgba, centerX2, y1);
  for (double i = M_PI_2 - sector; i > 0.001; i -= sector) {
    double coordX = cos(i)*radius;
    double coordY = sin(i)*radius;
    fillControlVertex(*(++outVertexIt), rgba, centerX1 - (float)coordX, centerY + (float)coordY);
    fillControlVertex(*(++outVertexIt), rgba, centerX2 + (float)coordX, centerY + (float)coordY);
  }
  fillControlVertex(*(++outVertexIt), rgba, x1, centerY);
  fillControlVertex(*(++outVertexIt), rgba, x2, centerY);

  centerY = y2 + radius;
  fillControlVertex(*(++outVertexIt), rgba, x1, centerY);
  fillControlVertex(*(++outVertexIt), rgba, x2, centerY);
  for (double i = sector; i < M_PI_2 - 0.001; i += sector) {
    double coordX = cos(i)*radius;
    double coordY = sin(i)*radius;
    fillControlVertex(*(++outVertexIt), rgba, centerX1 - (float)coordX, centerY - (float)coordY);
    fillControlVertex(*(++outVertexIt), rgba, centerX2 + (float)coordX, centerY - (float)coordY);
  }
  fillControlVertex(*(++outVertexIt), rgba, centerX1, y2);
  fillControlVertex(*(++outVertexIt), rgba, centerX2, y2);
}

void GeometryGenerator::fillRoundedRectangleIndices(uint32_t* outIndexIt, uint32_t firstVertexIndex, float radius) noexcept {
  const uint32_t lastVertexIndex = firstVertexIndex + getRoundedRectangleVertexCount(radius) - 4u;
  for (; firstVertexIndex <= lastVertexIndex; firstVertexIndex += 2u, ++outIndexIt) {
    *outIndexIt     = firstVertexIndex;
    *(++outIndexIt) = firstVertexIndex + 1u;
    *(++outIndexIt) = firstVertexIndex + 2u;
    *(++outIndexIt) = firstVertexIndex + 2u;
    *(++outIndexIt) = firstVertexIndex + 1u;
    *(++outIndexIt) = firstVertexIndex + 3u;
  }
}

// ---

void GeometryGenerator::fillTRCutDoubleGradientRectangleVertices(ControlVertex* outVertexIt, const float rgba[3][4],
                                                           float x1, float x2, float y1, float y2, float cornerSize) noexcept {
  const float yLine = y1 - cornerSize - 2.f;
  fillControlVertex(*outVertexIt,     rgba[1], x1,              y1);
  fillControlVertex(*(++outVertexIt), rgba[1], x2 - cornerSize, y1);
  fillControlVertex(*(++outVertexIt), rgba[0], x1,              y1 - cornerSize);
  fillControlVertex(*(++outVertexIt), rgba[0], x2,              y1 - cornerSize);
  fillControlVertex(*(++outVertexIt), rgba[0], x1,              yLine);
  fillControlVertex(*(++outVertexIt), rgba[0], x2,              yLine);
  fillControlVertex(*(++outVertexIt), rgba[2], x1,              yLine);
  fillControlVertex(*(++outVertexIt), rgba[2], x2,              yLine);
  fillControlVertex(*(++outVertexIt), rgba[0], x1,              y2);
  fillControlVertex(*(++outVertexIt), rgba[0], x2,              y2);
}

void GeometryGenerator::fillTRCutBorderVertices(ControlVertex* outVertexIt, const float rgba[4],
                                                      float x1, float x2, float y1, float y2, float cornerSize) noexcept {
  GeometryGenerator::fillRectangleVertices(outVertexIt, rgba, x1, x2-cornerSize, y1, y1-1.f); // border top
  outVertexIt += 4;
  GeometryGenerator::fillRectangleVertices(outVertexIt, rgba, x1, x2, y2+1.f, y2);            // border bottom
  outVertexIt += 4;
  GeometryGenerator::fillRectangleVertices(outVertexIt, rgba, x1, x1+1.f, y1-1.f, y2+1.f);        // border left
  outVertexIt += 4;
  GeometryGenerator::fillRectangleVertices(outVertexIt, rgba, x2-1.f, x2, y1-cornerSize, y2+1.f); // border right
  outVertexIt += 4;
  GeometryGenerator::fillObliqueRectangleVertices(outVertexIt, rgba, x2-cornerSize, x2, y1, y1-1.f, -cornerSize); // corner
}

// ---

void GeometryGenerator::fillTLBRCutRectangleVertices(ControlVertex* outVertexIt, const float rgba[4],
                                                     float x1, float x2, float y1, float y2, float cornerSize) noexcept {
  fillControlVertex(*outVertexIt,     rgba, x1,              y1 - cornerSize);
  fillControlVertex(*(++outVertexIt), rgba, x1 + cornerSize, y1);
  fillControlVertex(*(++outVertexIt), rgba, x1,              y2);
  fillControlVertex(*(++outVertexIt), rgba, x2,              y1);
  fillControlVertex(*(++outVertexIt), rgba, x2 - cornerSize, y2);
  fillControlVertex(*(++outVertexIt), rgba, x2,              y2 + cornerSize);
}

void GeometryGenerator::fillBLTRCutRectangleVertices(ControlVertex* outVertexIt, const float rgba[4],
                                                     float x1, float x2, float y1, float y2, float cornerSize) noexcept {
  fillControlVertex(*outVertexIt,     rgba, x1,              y1);
  fillControlVertex(*(++outVertexIt), rgba, x2 - cornerSize, y1);
  fillControlVertex(*(++outVertexIt), rgba, x1,              y2 + cornerSize);
  fillControlVertex(*(++outVertexIt), rgba, x2,              y1 - cornerSize);
  fillControlVertex(*(++outVertexIt), rgba, x1 + cornerSize, y2);
  fillControlVertex(*(++outVertexIt), rgba, x2,              y2);
}

// ---

void GeometryGenerator::fillCornerCutRectangleVertices(ControlVertex* outVertexIt, const float rgba[4],
                                                       float x1, float x2, float y1, float y2, float cornerSize) noexcept {
  fillControlVertex(*outVertexIt,     rgba, x1 + cornerSize, y1);
  fillControlVertex(*(++outVertexIt), rgba, x2 - cornerSize, y1);
  fillControlVertex(*(++outVertexIt), rgba, x1,              y1 - cornerSize);
  fillControlVertex(*(++outVertexIt), rgba, x2,              y1 - cornerSize);
  fillControlVertex(*(++outVertexIt), rgba, x1,              y2 + cornerSize);
  fillControlVertex(*(++outVertexIt), rgba, x2,              y2 + cornerSize);
  fillControlVertex(*(++outVertexIt), rgba, x1 + cornerSize, y2);
  fillControlVertex(*(++outVertexIt), rgba, x2 - cornerSize, y2);
}


// -- rectangle resize -- ------------------------------------------------------

void GeometryGenerator::resizeRoundedRectangleVerticesX(ControlVertex* vertexIt, float x2, float radius) noexcept {
  const uint32_t vertexCount = getRoundedRectangleVertexCount(radius);
  const float offset = x2 - vertexIt[(vertexCount >> 1) + 1u].position[0];
  const display::controls::ControlVertex* endIt = vertexIt + (intptr_t)vertexCount;

  ++vertexIt;
  while (vertexIt < endIt) {
    vertexIt->position[0] += offset;
    vertexIt += 2;
  }
}

void GeometryGenerator::resizeTLBRCutRectangleVerticesX(ControlVertex* vertexIt, float x2) noexcept {
  const float cornerSize = (float)static_cast<int32_t>(vertexIt[1].position[0] - vertexIt[0].position[0] + 0.5f);
  vertexIt += 3;
  vertexIt->position[0] = x2;
  (++vertexIt)->position[0] = x2 - cornerSize;
  (++vertexIt)->position[0] = x2;
}

void GeometryGenerator::resizeBLTRCutRectangleVerticesX(ControlVertex* vertexIt, float x2) noexcept {
  const float cornerSize = (float)static_cast<int32_t>(vertexIt[4].position[0] - vertexIt[0].position[0] + 0.5f);
  ++vertexIt;
  vertexIt->position[0] = x2 - cornerSize;
  vertexIt += 2;
  vertexIt->position[0] = x2;
  vertexIt += 2;
  vertexIt->position[0] = x2;
}


// -- other shape generation -- ------------------------------------------------------

void GeometryGenerator::fillDiamondVertices(ControlVertex* outVertexIt, const float rgba[4],
                                            float x1, float x2, float y1, float y2) noexcept {
  const float centerX = (x1 + x2) * 0.5f;
  const float centerY = (y1 + y2) * 0.5f;
  fillControlVertex(*outVertexIt,     rgba, centerX, y1);
  fillControlVertex(*(++outVertexIt), rgba, x2, centerY);
  fillControlVertex(*(++outVertexIt), rgba, x1, centerY);
  fillControlVertex(*(++outVertexIt), rgba, centerX, y2);
}

void GeometryGenerator::fillCrossVertices(ControlVertex* outVertexIt, const float rgba[4],
                                          float x1, float x2, float y1, float y2) noexcept {
  fillControlVertex(*outVertexIt,     rgba, x1 + 1.f, y1 + 1.f);
  fillControlVertex(*(++outVertexIt), rgba, x2 + 1.f, y2 + 1.f);
  fillControlVertex(*(++outVertexIt), rgba, x1 - 1.f, y1 - 1.f);
  fillControlVertex(*(++outVertexIt), rgba, x2 - 1.f, y2 - 1.f);

  fillControlVertex(*(++outVertexIt), rgba, x1 - 1.f, y2 + 1.f);
  fillControlVertex(*(++outVertexIt), rgba, x2 - 1.f, y1 + 1.f);
  fillControlVertex(*(++outVertexIt), rgba, x1 + 1.f, y2 - 1.f);
  fillControlVertex(*(++outVertexIt), rgba, x2 + 1.f, y1 - 1.f);
}

void GeometryGenerator::fillPencilVertices(ControlVertex* outVertexIt, const float rgba[4],
                                           float x1, float x2, float y1, float y2) noexcept {
  // top
  fillControlVertex(*outVertexIt,     rgba, x2 - 2.f, y1 + 2.f);
  fillControlVertex(*(++outVertexIt), rgba, x2 + 2.f, y1 - 2.f);
  fillControlVertex(*(++outVertexIt), rgba, x2 - 4.f, y1);
  fillControlVertex(*(++outVertexIt), rgba, x2,       y1 - 4.f);

  // body
  fillControlVertex(*(++outVertexIt), rgba, x2 - 5.f, y1 - 1.f);
  fillControlVertex(*(++outVertexIt), rgba, x2 - 1.f, y1 - 5.f);
  fillControlVertex(*(++outVertexIt), rgba, x1 + 1.f, y2 + 5.f);
  fillControlVertex(*(++outVertexIt), rgba, x1 + 5.f, y2 + 1.f);

  // tip
  fillControlVertex(*(++outVertexIt), rgba, x1,       y2 + 4.f);
  fillControlVertex(*(++outVertexIt), rgba, x1 + 4.f, y2);
  fillControlVertex(*(++outVertexIt), rgba, x1 - 1.f, y2 - 1.f);
}


// -- circle generation -- -----------------------------------------------------

void GeometryGenerator::fillCircleVertices(ControlVertex* topIt, const float rgba[4],
                                           uint32_t circleVertexCount, double radius, float centerX, float centerY) noexcept {
  ControlVertex* bottomIt = topIt + ((intptr_t)circleVertexCount - 1);

  // top/bottom vertices
  fillControlVertex(*topIt,    rgba, centerX, centerY + (float)radius);
  fillControlVertex(*bottomIt, rgba, centerX, centerY - (float)radius);
  ++topIt;
  --bottomIt;

  // 1/4-circle calculation ]PI;0] -> generate top and bottom 1/2-circles
  const double sector = 2.0*M_PI / (double)circleVertexCount;
  for (double i = M_PI_2 - sector; topIt <= bottomIt; i -= sector, ++topIt, --bottomIt) {
    double coordX = cos(i)*radius;
    double coordY = sin(i)*radius;
    fillControlVertex(*topIt,          rgba, centerX + (float)coordX, centerY + (float)coordY);
    fillControlVertex(*(++topIt),      rgba, centerX - (float)coordX, centerY + (float)coordY);
    if (topIt != bottomIt) {
      fillControlVertex(*bottomIt,     rgba, centerX - (float)coordX, centerY - (float)coordY);
      fillControlVertex(*(--bottomIt), rgba, centerX + (float)coordX, centerY - (float)coordY);
    }
  }
}

void GeometryGenerator::fillCircleIndices(uint32_t* outIndexIt, uint32_t firstIndex, uint32_t circleVertexCount) noexcept {
  //{ 0,1,2,  2,1,3,2,3,4,  4,3,5,4,5,6,  6,5,7,6,7,8,  ...,  14,13,15 }
  uint32_t currentIndex = firstIndex;
  *outIndexIt = currentIndex;
  *(++outIndexIt) = currentIndex + 1u;
  *(++outIndexIt) = currentIndex + 2u;

  const uint32_t endIndex = firstIndex + circleVertexCount - 2u;
  for (currentIndex += 2u; currentIndex < endIndex; currentIndex += 2u) {
    *(++outIndexIt) = currentIndex;
    *(++outIndexIt) = currentIndex - 1u;
    *(++outIndexIt) = currentIndex + 1u;
    *(++outIndexIt) = currentIndex;
    *(++outIndexIt) = currentIndex + 1u;
    *(++outIndexIt) = currentIndex + 2u;
  }

  *(++outIndexIt) = currentIndex;
  *(++outIndexIt) = currentIndex - 1u;
  *(++outIndexIt) = currentIndex + 1u;
}
