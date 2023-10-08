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
#include <cstring>
#include "menu/controls/ruler.h"

#define THUMB_CIRCLE_VERTICES 16

using namespace video_api;
using namespace display;
using namespace display::controls;
using namespace menu::controls;


static inline void setControlVertex(ControlVertex& outVertex, const float rgba[4], float x, float y) {
  float* position = outVertex.position;
  *position = x;
  *(++position) = y;
  *(++position) = 0.f; // z
  *(++position) = 1.f; // w
  memcpy(outVertex.color, rgba, 4*sizeof(float));
}
static void generateRulerThumbVertices(uint32_t thumbHeight, double offset, double radius,
                                       const float color[4], ControlVertex* topIt)  {
  ControlVertex* bottomIt = topIt + (intptr_t)(THUMB_CIRCLE_VERTICES+2 - 1);
  const double midpointBottom = offset - (double)thumbHeight;

  // top/bottom vertices
  setControlVertex(*topIt,    color, (float)offset, (float)(radius - offset));
  setControlVertex(*bottomIt, color, (float)offset, (float)(midpointBottom - radius));
  
  // 1/4-circle calculation ]PI;0] -> generate top and bottom 1/2-circles
  for (double i = M_PI_2 - 2*M_PI/THUMB_CIRCLE_VERTICES; topIt <= bottomIt; i -= 2*M_PI/THUMB_CIRCLE_VERTICES, ++topIt, --bottomIt) {
    double coordX = cos(i)*radius;
    double coordY = sin(i)*radius;
    if (i <= 0.01) { // last vertices -> round to closest pixels
      uint32_t roundX = static_cast<uint32_t>(coordX + 0.5);
      coordX = static_cast<double>(roundX);
      uint32_t roundY = static_cast<uint32_t>(coordY + 0.5);
      coordY = static_cast<double>(roundY);
    }
    setControlVertex(*topIt,        color, (float)(offset + coordX), (float)(coordY - offset));
    setControlVertex(*(++topIt),    color, (float)(offset - coordX), (float)(coordY - offset));
    setControlVertex(*bottomIt,     color, (float)(offset - coordX), (float)(midpointBottom - coordY));
    setControlVertex(*(--bottomIt), color, (float)(offset + coordX), (float)(midpointBottom - coordY));
  }
}
static void generateRulerThumbIndices(uint32_t* indexIt, uint32_t baseIndex) {
  //{ 0,1,2, 2,1,3,2,3,4, 4,3,5,4,5,6, 6,5,7,6,7,8, ..., 16,15,17 }
  uint32_t currentIndex = baseIndex;
  *indexIt = currentIndex;
  *(++indexIt) = currentIndex + 1u;
  *(++indexIt) = currentIndex + 2u;

  for (currentIndex += 2u; currentIndex < baseIndex + (uint32_t)THUMB_CIRCLE_VERTICES; currentIndex += 2u) {
    *(++indexIt) = currentIndex;
    *(++indexIt) = currentIndex - 1u;
    *(++indexIt) = currentIndex + 1u;
    *(++indexIt) = currentIndex;
    *(++indexIt) = currentIndex + 1u;
    *(++indexIt) = currentIndex + 2u;
  }

  *(++indexIt) = currentIndex;
  *(++indexIt) = currentIndex - 1u;
  *(++indexIt) = currentIndex + 1u;
}

// ---

void Ruler::init(RendererContext& context, const char32_t* label, TextAlignment labelAlign,
                 float rulerColor[4], float borderColor[4], float thumbColor[4],
                 int32_t x, int32_t y, uint32_t fixedRulerWidth, const char* suffix) {
  auto& labelFont = context.getFont(FontType::labels);
  const uint32_t thumbHeight = (labelFont.XHeight() << 1);
  
  // create label
  uint32_t labelWidthWithMargin = 0;
  const int32_t labelY = y + paddingY - ((int32_t)labelFont.XHeight() - (int32_t)thumbHeight)/2;
  if (label != nullptr && *label != (char32_t)0) {
    labelMesh = TextMesh(*context.renderer, labelFont, label, context.pixelSizeX, context.pixelSizeY, x, labelY, labelAlign);
    labelWidthWithMargin = (minLabelWidth >= labelMesh.width()) ? minLabelWidth + labelMargin() : labelMesh.width() + labelMargin();
    x = labelMesh.x(); // in case labelAlign != left
  }
  if (suffix != nullptr && *suffix != (char32_t)0) {
    suffixMesh = TextMesh(*context.renderer, labelFont, label, context.pixelSizeX, context.pixelSizeY,
                          x + (int32_t)labelWidthWithMargin + (int32_t)fixedRulerWidth + (int32_t)labelMargin(), labelY);
  }

  // specify ruler positions
  uint32_t thumbOffset, stepWidth;
  if (minValue != maxValue) {
    const uint32_t valueCount = maxValue - minValue;
    stepWidth = (fixedRulerWidth - 1u)* step / valueCount;
    firstStepOffset = (fixedRulerWidth - (stepWidth * valueCount)) >> 1;
    thumbOffset = firstStepOffset + (*boundValue - minValue)*stepWidth - (thumbHeight >> 1);
  }
  else {
    stepWidth = firstStepOffset = (fixedRulerWidth >> 1);
    thumbOffset = firstStepOffset - (thumbHeight >> 1);
  }

  // create ruler
  { std::vector<ControlVertex> vertices;
    vertices.resize(8u + 4u*((maxValue - minValue)+1u));
    const float rulerHeight = (float)(thumbHeight >> 2);
    ControlVertex* vertexIt = vertices.data();
    setControlVertex(*vertexIt,     rulerColor, 0.f,                    0.f); // border
    setControlVertex(*(++vertexIt), rulerColor, (float)fixedRulerWidth, 0.f);
    setControlVertex(*(++vertexIt), rulerColor, 0.f,                    -rulerHeight);
    setControlVertex(*(++vertexIt), rulerColor, (float)fixedRulerWidth, -rulerHeight);
    setControlVertex(*(++vertexIt), rulerColor, 1.f,                        0.f); // ruler
    setControlVertex(*(++vertexIt), rulerColor, (float)(fixedRulerWidth-2), 0.f);
    setControlVertex(*(++vertexIt), rulerColor, 1.f,                        -rulerHeight+2.f);
    setControlVertex(*(++vertexIt), rulerColor, (float)(fixedRulerWidth-2), -rulerHeight+2.f);
    std::vector<uint32_t> indices{ 0,1,2, 2,1,3,  4,5,6, 6,5,7 };

    for (uint32_t markX = firstStepOffset, index = 8; markX < fixedRulerWidth; markX += stepWidth, index += 6) { // ruler marks
      setControlVertex(*(++vertexIt), thumbColor, (float)(markX-1), -rulerHeight-2.f);
      setControlVertex(*(++vertexIt), thumbColor, (float)markX,     -rulerHeight-2.f);
      setControlVertex(*(++vertexIt), thumbColor, (float)(markX-1), -rulerHeight-5.f);
      setControlVertex(*(++vertexIt), thumbColor, (float)markX,     -rulerHeight-5.f);
      indices.emplace_back(index);
      indices.emplace_back(index + 1);
      indices.emplace_back(index + 2);
      indices.emplace_back(index + 2);
      indices.emplace_back(index + 1);
      indices.emplace_back(index + 3);
    }

    controlMesh = ControlMesh(*context.renderer, std::move(vertices), indices, context.pixelSizeX, context.pixelSizeY,
                              x + (int32_t)labelWidthWithMargin, y + (int32_t)((thumbHeight*3) >> 3), fixedRulerWidth, (thumbHeight >> 2));
  }

  // create thumb
  std::vector<ControlVertex> vertices;
  vertices.resize((THUMB_CIRCLE_VERTICES+2)*2);
  const float thumbBorderColor[4]{ (borderColor[0]+thumbColor[0])*0.5f, (borderColor[1]+thumbColor[1])*0.5f,
                                   (borderColor[2]+thumbColor[2])*0.5f, 0.75f };
  const double radius = (double)(thumbHeight >> 2); // width=height/2 ; radius=height/4;
  generateRulerThumbVertices(thumbHeight, radius, radius, thumbBorderColor, vertices.data());
  generateRulerThumbVertices(thumbHeight, radius, (double)((thumbHeight>>2)-1), thumbColor,
                             vertices.data() + (intptr_t)(THUMB_CIRCLE_VERTICES+2));
  std::vector<uint32_t> indices;
  indices.resize(THUMB_CIRCLE_VERTICES*3);
  generateRulerThumbIndices(indices.data(), 0);
  generateRulerThumbIndices(indices.data() + (intptr_t)(THUMB_CIRCLE_VERTICES+2), THUMB_CIRCLE_VERTICES+2);
  
  thumbMesh = ControlMesh(*context.renderer, std::move(vertices), indices, context.pixelSizeX, context.pixelSizeY,
                          controlMesh.x() + (int32_t)thumbOffset, y, fixedRulerWidth, thumbHeight);
}

void Ruler::move(RendererContext& context, int32_t x, int32_t y, TextAlignment labelAlign) {
  uint32_t labelWidthWithMargin = 0;
  const int32_t labelY = y + ((int32_t)labelMesh.y() - (int32_t)controlMesh.y());
  if (labelMesh.width()) {
    if (labelAlign != TextAlignment::left)
      x -= (labelAlign == TextAlignment::right) ? labelMesh.width() : (labelMesh.width() >> 1);
    labelMesh.move(*context.renderer, context.pixelSizeX, context.pixelSizeY, x, labelY);
    labelWidthWithMargin = (minLabelWidth >= labelMesh.width()) ? minLabelWidth + labelMargin() : labelMesh.width() + labelMargin();
  }
  if (suffixMesh.width()) {
    suffixMesh.move(*context.renderer, context.pixelSizeX, context.pixelSizeY,
                    x + (int32_t)labelWidthWithMargin + (int32_t)controlMesh.width() + (int32_t)labelMargin(), labelY);
  }

  controlMesh.move(*context.renderer, context.pixelSizeX, context.pixelSizeY,
                   x + (int32_t)labelWidthWithMargin, y + (int32_t)((thumbMesh.height()*3) >> 3));

  uint32_t thumbOffset = (minValue != maxValue)
                       ? firstStepOffset + (*boundValue - minValue)*interStepWidth() - (thumbMesh.height() >> 1)
                       : firstStepOffset - (thumbMesh.height() >> 1);
  thumbMesh.move(*context.renderer, context.pixelSizeX, context.pixelSizeY,
                 controlMesh.x() + (int32_t)thumbOffset, y);
}

void Ruler::updateThumbPosition(RendererContext& context, uint32_t value) {
  int32_t thumbX = controlMesh.x() + (int32_t)firstStepOffset - (int32_t)(thumbMesh.width() >> 1);
  if (minValue != maxValue)
    thumbX += (int32_t)((value - minValue) * interStepWidth());

  thumbMesh.move(*context.renderer, context.pixelSizeX, context.pixelSizeY, thumbX, thumbMesh.y());
}

void Ruler::mouseMove(RendererContext& context, int32_t mouseX) {
  if (isDragged() && minValue != maxValue) {
    mouseX -= (controlMesh.x() + (int32_t)firstStepOffset);
    if (mouseX < 0)
      mouseX = 0;
    else if (mouseX >= (int32_t)controlMesh.width() - (int32_t)firstStepOffset)
      mouseX = (int32_t)controlMesh.width() - (int32_t)firstStepOffset - 1;

    const uint32_t stepWidth = interStepWidth();
    *boundValue = lastValue = minValue + (((uint32_t)mouseX + (stepWidth >> 1)) / stepWidth);
    updateThumbPosition(context, lastValue);
  }
}

// ---

void Ruler::selectPrevious(RendererContext& context) {
  if (isEnabled() && *boundValue > minValue) {
    lastValue = --(*boundValue);
    updateThumbPosition(context, lastValue);
  }
}
void Ruler::selectNext(RendererContext& context) {
  if (isEnabled() && *boundValue < maxValue) {
    lastValue = ++(*boundValue);
    updateThumbPosition(context, lastValue);
  }
}

// ---

void Ruler::drawBackground(RendererContext& context) {
  if (*boundValue != lastValue) {
    lastValue = *boundValue;
    updateThumbPosition(context, lastValue);
  }
  controlMesh.draw(*context.renderer);
  thumbMesh.draw(*context.renderer);
}