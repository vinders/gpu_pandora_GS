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
                                             float x, float y, float width, float height) {
  fillControlVertex(*outVertexIt,     rgba, x,              y - height);
  fillControlVertex(*(++outVertexIt), rgba, x + width*0.5f, y);
  fillControlVertex(*(++outVertexIt), rgba, x + width,      y - height);
}

void GeometryGenerator::fillInvertedTriangleVertices(ControlVertex* outVertexIt, const float rgba[4],
                                                     float x, float y, float width, float height) {
  fillControlVertex(*outVertexIt,     rgba, x,              y);
  fillControlVertex(*(++outVertexIt), rgba, x + width,      y);
  fillControlVertex(*(++outVertexIt), rgba, x + width*0.5f, y - height);
}


// -- rectangle generation -- --------------------------------------------------

void GeometryGenerator::fillRectangleVertices(ControlVertex* outVertexIt, const float rgba[4],
                                              float x1, float x2, float y1, float y2) {
  fillControlVertex(*outVertexIt,     rgba, x1, y1);
  fillControlVertex(*(++outVertexIt), rgba, x2, y1);
  fillControlVertex(*(++outVertexIt), rgba, x1, y2);
  fillControlVertex(*(++outVertexIt), rgba, x2, y2);
}

void GeometryGenerator::fillHorizontalRectangleVertices(ControlVertex* outVertexIt, const float rgba1[4], const float rgba2[4],
                                              float x1, float x2, float y1, float y2) {
  fillControlVertex(*outVertexIt,     rgba1, x1, y1);
  fillControlVertex(*(++outVertexIt), rgba2, x2, y1);
  fillControlVertex(*(++outVertexIt), rgba1, x1, y2);
  fillControlVertex(*(++outVertexIt), rgba2, x2, y2);
}

void GeometryGenerator::fillVerticalRectangleVertices(ControlVertex* outVertexIt, const float rgba1[4], const float rgba2[4],
  float x1, float x2, float y1, float y2) {
  fillControlVertex(*outVertexIt,     rgba1, x1, y1);
  fillControlVertex(*(++outVertexIt), rgba1, x2, y1);
  fillControlVertex(*(++outVertexIt), rgba2, x1, y2);
  fillControlVertex(*(++outVertexIt), rgba2, x2, y2);
}

void GeometryGenerator::fillObliqueRectangleVertices(ControlVertex* outVertexIt, const float rgba[4],
                                                     float x1, float x2, float y1, float y2, float yOffset) {
  fillControlVertex(*outVertexIt,     rgba, x1, y1);
  fillControlVertex(*(++outVertexIt), rgba, x2, y1 + yOffset);
  fillControlVertex(*(++outVertexIt), rgba, x1, y2);
  fillControlVertex(*(++outVertexIt), rgba, x2, y2 + yOffset);
}

// ---

void GeometryGenerator::fillLeftRoundedRectangleVertices(ControlVertex* outVertexIt, const float rgba[4],
                                                         float x1, float x2, float y1, float y2) {
  double radius = fabs((double)y2 - (double)y1)*0.5;
  float centerX = x1 + (float)radius;
  float centerY = y1 - (float)radius;
  fillControlVertex(*outVertexIt,     rgba, x1, centerY);

  constexpr const double sector = 2.0*M_PI / 12.0;
  for (double i = M_PI - sector; i > M_PI_2 + 0.01; i -= sector) {
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
                                                          float x1, float x2, float y1, float y2) {
  double radius = fabs((double)y2 - (double)y1)*0.5;
  float centerX = x2 - (float)radius;
  float centerY = y1 - (float)radius;

  fillControlVertex(*outVertexIt,     rgba, x1, y1);
  fillControlVertex(*(++outVertexIt), rgba, x1, y2);
  fillControlVertex(*(++outVertexIt), rgba, centerX, y1);
  fillControlVertex(*(++outVertexIt), rgba, centerX, y2);

  constexpr const double sector = 2.0*M_PI / 12.0;
  for (double i = M_PI_2 - sector; i > 0.01; i -= sector) {
    double coordX = cos(i)*radius;
    double coordY = sin(i)*radius;
    fillControlVertex(*(++outVertexIt), rgba, centerX + (float)coordX, centerY + (float)coordY);
    fillControlVertex(*(++outVertexIt), rgba, centerX + (float)coordX, centerY - (float)coordY);
  }
  fillControlVertex(*(++outVertexIt), rgba, x2, centerY);
}

// ---

void GeometryGenerator::fillTopRightCutRectangleVertices(ControlVertex* outVertexIt, const float rgba[4],
                                                         float x1, float x2, float y1, float y2, float cornerSize) {
  constexpr const float darkMultiplier = 0.85f;
  const float cornerFactor = 1.f - (cornerSize / (y2 - y1)) * (1.f - darkMultiplier);
  const float cornerRgba[4]{ rgba[0]*cornerFactor, rgba[1]*cornerFactor, rgba[2]*cornerFactor, rgba[3] };
  const float darkerRgba[4]{ rgba[0]*darkMultiplier, rgba[1]*darkMultiplier, rgba[2]*darkMultiplier, rgba[3] };

  fillControlVertex(*outVertexIt,     rgba,       x1,              y1);
  fillControlVertex(*(++outVertexIt), rgba,       x2 - cornerSize, y1);
  fillControlVertex(*(++outVertexIt), darkerRgba, x1,              y2);
  fillControlVertex(*(++outVertexIt), cornerRgba, x2,              y1 - cornerSize);
  fillControlVertex(*(++outVertexIt), darkerRgba, x2,              y2);
}

void GeometryGenerator::fillTopRightCutBorderVertices(ControlVertex* outVertexIt, const float rgba[4],
                                                      float x1, float x2, float y1, float y2, float cornerSize) {
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

void GeometryGenerator::fillDoubleCutRectangleVertices(ControlVertex* outVertexIt, const float rgba[4],
                                                   float x1, float x2, float y1, float y2, float cornerSize) {
  fillControlVertex(*outVertexIt,     rgba, x1,              y1 - cornerSize);
  fillControlVertex(*(++outVertexIt), rgba, x1 + cornerSize, y1);
  fillControlVertex(*(++outVertexIt), rgba, x1,              y2);
  fillControlVertex(*(++outVertexIt), rgba, x2,              y1);
  fillControlVertex(*(++outVertexIt), rgba, x2 - cornerSize, y2);
  fillControlVertex(*(++outVertexIt), rgba, x2,              y2 + cornerSize);
}

void GeometryGenerator::fillDoubleCutBorderVertices(ControlVertex* outVertexIt, const float rgba[4],
                                                float x1, float x2, float y1, float y2, float cornerSize) {
  GeometryGenerator::fillRectangleVertices(outVertexIt, rgba, x1+cornerSize, x2, y1, y1-1.f);  // border top
  outVertexIt += 4;
  GeometryGenerator::fillRectangleVertices(outVertexIt, rgba, x1, x2-cornerSize, y2+1.f, y2);  // border bottom
  outVertexIt += 4;
  GeometryGenerator::fillRectangleVertices(outVertexIt, rgba, x1, x1+1.f, y1-cornerSize, y2+1.f);  // border left
  outVertexIt += 4;
  GeometryGenerator::fillRectangleVertices(outVertexIt, rgba, x2-1.f, x2, y1-1.f, y2+cornerSize);  // border right
  outVertexIt += 4;
  GeometryGenerator::fillObliqueRectangleVertices(outVertexIt, rgba, // corner left
                                                  x1, x1+cornerSize, y1-cornerSize, y1-cornerSize-1.f, cornerSize);
  outVertexIt += 4;
  GeometryGenerator::fillObliqueRectangleVertices(outVertexIt, rgba, // corner right
                                                  x2-cornerSize, x2, y2+1.f, y2, cornerSize);
}

// ---

void GeometryGenerator::fillCornerCutRectangleVertices(ControlVertex* outVertexIt, const float rgba[4],
                                                       float x1, float x2, float y1, float y2, float cornerSize) {
  fillControlVertex(*outVertexIt,     rgba, x1 + cornerSize, y1);
  fillControlVertex(*(++outVertexIt), rgba, x2 - cornerSize, y1);
  fillControlVertex(*(++outVertexIt), rgba, x1,              y1 - cornerSize);
  fillControlVertex(*(++outVertexIt), rgba, x2,              y1 - cornerSize);
  fillControlVertex(*(++outVertexIt), rgba, x1,              y2 + cornerSize);
  fillControlVertex(*(++outVertexIt), rgba, x2,              y2 + cornerSize);
  fillControlVertex(*(++outVertexIt), rgba, x1 + cornerSize, y2);
  fillControlVertex(*(++outVertexIt), rgba, x2 - cornerSize, y2);
}


// -- circle generation -- -----------------------------------------------------

void GeometryGenerator::fillCircleVertices(display::controls::ControlVertex* topIt, const float rgba[4],
                                           uint32_t circleVertexCount, double radius, float centerX, float centerY) {
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

void GeometryGenerator::fillCircleIndices(uint32_t* outIndexIt, uint32_t firstIndex, uint32_t circleVertexCount) {
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
