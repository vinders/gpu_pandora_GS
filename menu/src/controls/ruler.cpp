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
#include "menu/controls/geometry_generator.h"
#include "menu/controls/ruler.h"

using namespace video_api;
using namespace display;
using namespace display::controls;
using namespace menu::controls;

ControlType Ruler::Type() const noexcept { return ControlType::ruler; }

static inline void clampColorComponents(float color[4]) {
  if (color[0] > 1.f)
    color[0] = 1.f;
  if (color[1] > 1.f)
    color[1] = 1.f;
  if (color[2] > 1.f)
    color[2] = 1.f;
}


// -- init/resize geometry -- --------------------------------------------------

#define THUMB_CIRCLE_VERTICES 24

void Ruler::init(RendererContext& context, const char32_t* label, const char32_t* suffix,
                 display::controls::TextAlignment labelAlign, int32_t x, int32_t labelY, const ControlStyle& style,
                 uint32_t fixedRulerWidth, const float borderColor[4], const float thumbColor[4], float leftFillColor[4]) {
  // create label
  auto& labelFont = context.getFont(FontType::labels);
  labelMesh = TextMesh(context.renderer(), labelFont, label, context.pixelSizeX(), context.pixelSizeY(), x, labelY, labelAlign);
  uint32_t labelWidthWithMargin = (minLabelWidth >= labelMesh.width()) ? minLabelWidth : labelMesh.width();
  if (labelWidthWithMargin) {
    labelWidthWithMargin += labelMargin();
    x = labelMesh.x(); // in case labelAlign != left
  }

  // create suffix (if any)
  if (suffix != nullptr && *suffix != (char32_t)0) {
    suffixMesh = TextMesh(context.renderer(), labelFont, label, context.pixelSizeX(), context.pixelSizeY(),
                          x + (int32_t)labelWidthWithMargin + (int32_t)fixedRulerWidth + (int32_t)labelMargin(), labelY);
  }

  // specify ruler positions
  const uint32_t thumbWidth = (labelFont.XHeight() << 1) + (paddingY & ~(uint32_t)0x1);
  const int32_t y = labelY + (int32_t)(labelFont.XHeight() >> 1) - (int32_t)(thumbWidth >> 1);
  uint32_t thumbOffset;
  if (minValue != maxValue) {
    const uint32_t valueCount = maxValue - minValue;
    stepWidth = (fixedRulerWidth - (style.paddingX << 1) - 1u) * step / valueCount;
    firstStepOffset = (fixedRulerWidth - (stepWidth * valueCount)) >> 1;
    thumbOffset = firstStepOffset + (*boundValue - minValue)*stepWidth - (thumbWidth >> 1);
  }
  else {
    stepWidth = firstStepOffset = (fixedRulerWidth >> 1);
    thumbOffset = firstStepOffset - (thumbWidth >> 1);
  }

  // create ruler (sliding bar + marks)
  std::vector<ControlVertex> vertices(static_cast<size_t>(9u + 4u*((maxValue - minValue)+1u)));
  ControlVertex* vertexIt = vertices.data();

  const uint32_t rulerHeight = (thumbWidth+2) >> 2;
  GeometryGenerator::fillRightRoundedRectangleVertices(vertexIt, style.color, (float)(rulerHeight >> 1),
                                                       (float)fixedRulerWidth, 0.f, -(float)rulerHeight);
  vertexIt += 9;
  std::vector<uint32_t> indices{ 0,2,1, 1,2,3, 3,2,4, 3,4,5, 5,4,6, 5,6,7, 7,6,8 };

  const float marksColor[4]{ 0.5f, 0.5f, 0.5f, 0.65f };
  indices.reserve(indices.size() + (maxValue - minValue + 1u)*4u);
  for (uint32_t markX = firstStepOffset, index = 9; markX < fixedRulerWidth; markX += stepWidth, index += 4) {
    GeometryGenerator::fillRectangleVertices(vertexIt, marksColor, // ruler marks
                                             (float)(markX-1), (float)markX, -(float)(rulerHeight+1), -(float)(rulerHeight+6));
    vertexIt += 4;
    indices.emplace_back(index);
    indices.emplace_back(index + 1);
    indices.emplace_back(index + 2);
    indices.emplace_back(index + 2);
    indices.emplace_back(index + 1);
    indices.emplace_back(index + 3);
  }

  controlMesh = ControlMesh(context.renderer(), std::move(vertices), indices, context.pixelSizeX(), context.pixelSizeY(),
                            x + (int32_t)labelWidthWithMargin, y + ((int32_t)thumbWidth - (int32_t)rulerHeight)/2,
                            fixedRulerWidth, rulerHeight);

  // create sliding bar filler
  vertices = std::vector<ControlVertex>(static_cast<size_t>(9u));
  GeometryGenerator::fillLeftRoundedRectangleVertices(vertices.data(), leftFillColor, 0.f,
                                                      (float)(thumbOffset + (thumbWidth >> 1)), 0.f, -(float)rulerHeight);
  indices = { 0,1,2, 2,1,3, 2,3,4, 4,3,5, 4,5,6, 6,5,7, 6,7,8 };

  fillerMesh = ControlMesh(context.renderer(), std::move(vertices), indices, context.pixelSizeX(), context.pixelSizeY(),
                           x + (int32_t)labelWidthWithMargin, y + ((int32_t)thumbWidth - (int32_t)rulerHeight)/2,
                           fixedRulerWidth, rulerHeight);

  // create thumb (circle)
  vertices = std::vector<ControlVertex>(static_cast<size_t>((THUMB_CIRCLE_VERTICES) * 3));
  const double radius = (double)(thumbWidth >> 1);

  GeometryGenerator::fillCircleVertices(vertices.data(), borderColor,                                       // border (outline)
                                        THUMB_CIRCLE_VERTICES, radius, (float)radius, -(float)radius);
  float innerBorderColor[4]{ thumbColor[0]*1.25f, thumbColor[2]*1.25f, thumbColor[3]*1.25f, thumbColor[3] };
  clampColorComponents(innerBorderColor);
  GeometryGenerator::fillCircleVertices(vertices.data() + (intptr_t)THUMB_CIRCLE_VERTICES, innerBorderColor,// border (inner)
                                        THUMB_CIRCLE_VERTICES, radius-1.0, (float)radius, -(float)radius);
  GeometryGenerator::fillCircleVertices(vertices.data() + (intptr_t)(THUMB_CIRCLE_VERTICES*2), thumbColor,  // background
                                        THUMB_CIRCLE_VERTICES, radius-2.0, (float)radius, -(float)radius);

  indices = std::vector<uint32_t>(static_cast<size_t>((THUMB_CIRCLE_VERTICES-2)*3 * 3));
  GeometryGenerator::fillCircleIndices(indices.data(), 0, THUMB_CIRCLE_VERTICES);
  GeometryGenerator::fillCircleIndices(indices.data() + (intptr_t)((THUMB_CIRCLE_VERTICES-2)*3),
                                       THUMB_CIRCLE_VERTICES, THUMB_CIRCLE_VERTICES);
  GeometryGenerator::fillCircleIndices(indices.data() + (intptr_t)((THUMB_CIRCLE_VERTICES-2)*3 * 2),
                                       THUMB_CIRCLE_VERTICES*2, THUMB_CIRCLE_VERTICES);

  thumbMesh = ControlMesh(context.renderer(), std::move(vertices), indices, context.pixelSizeX(), context.pixelSizeY(),
                          controlMesh.x() + (int32_t)thumbOffset, y, thumbWidth, thumbWidth);
}

// ---

void Ruler::move(RendererContext& context, int32_t x, int32_t labelY, TextAlignment labelAlign) {
  const int32_t y = labelY - ((int32_t)labelMesh.y() - (int32_t)controlMesh.y());

  if (labelAlign != TextAlignment::left)
    x -= (labelAlign == TextAlignment::right) ? (int32_t)labelMesh.width() : (int32_t)(labelMesh.width() >> 1);
  labelMesh.move(context.renderer(), context.pixelSizeX(), context.pixelSizeY(), x, labelY);
  uint32_t labelWidthWithMargin = (minLabelWidth >= labelMesh.width()) ? minLabelWidth : labelMesh.width();
  if (labelWidthWithMargin)
    labelWidthWithMargin += labelMargin();
  
  if (suffixMesh.width()) {
    suffixMesh.move(context.renderer(), context.pixelSizeX(), context.pixelSizeY(),
                    x + (int32_t)labelWidthWithMargin + (int32_t)controlMesh.width() + (int32_t)labelMargin(), labelY);
  }

  controlMesh.move(context.renderer(), context.pixelSizeX(), context.pixelSizeY(),
                   x + (int32_t)labelWidthWithMargin, y + ((int32_t)thumbMesh.height() - (int32_t)controlMesh.height())/2);
  fillerMesh.move(context.renderer(), context.pixelSizeX(), context.pixelSizeY(),
                  x + (int32_t)labelWidthWithMargin, y + ((int32_t)thumbMesh.height() - (int32_t)controlMesh.height())/2);

  uint32_t thumbOffset = (minValue != maxValue)
                       ? firstStepOffset + (*boundValue - minValue)*stepWidth - (thumbMesh.width() >> 1)
                       : firstStepOffset - (thumbMesh.width() >> 1);
  thumbMesh.move(context.renderer(), context.pixelSizeX(), context.pixelSizeY(),
                 controlMesh.x() + (int32_t)thumbOffset, y);
}

void Ruler::updateThumbPosition(RendererContext& context, uint32_t value) {
  int32_t thumbX = controlMesh.x() + (int32_t)firstStepOffset - (int32_t)(thumbMesh.width() >> 1);
  if (minValue != maxValue)
    thumbX += (int32_t)((value - minValue) * stepWidth);

  const uint32_t thumbOffset = static_cast<uint32_t>(thumbX - controlMesh.x());
  std::vector<ControlVertex> vertices = fillerMesh.relativeVertices();
  GeometryGenerator::resizeRightRoundedRectangleVerticesX(vertices.data(), (float)(thumbOffset + (thumbMesh.width() >> 1)),
                                                          (float)fillerMesh.height());
  fillerMesh.update(context.renderer(), std::move(vertices), context.pixelSizeX(), context.pixelSizeY(),
                    fillerMesh.x(), fillerMesh.y(), fillerMesh.width(), fillerMesh.height());

  thumbMesh.move(context.renderer(), context.pixelSizeX(), context.pixelSizeY(), thumbX, thumbMesh.y());
}


// -- operations -- ------------------------------------------------------------

void Ruler::mouseMove(RendererContext& context, int32_t mouseX) {
  if (isDragged() && minValue != maxValue) {
    mouseX -= (controlMesh.x() + (int32_t)firstStepOffset);
    if (mouseX < 0)
      mouseX = 0;
    else if (mouseX >= (int32_t)controlMesh.width() - (int32_t)firstStepOffset)
      mouseX = (int32_t)controlMesh.width() - (int32_t)firstStepOffset - 1;

    uint32_t newValue = minValue + (((uint32_t)mouseX + (stepWidth >> 1)) / stepWidth);
    if (*boundValue != newValue) {
      *boundValue = lastValue = newValue;
      updateThumbPosition(context, lastValue);
    }
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


// -- rendering -- -------------------------------------------------------------

void Ruler::drawBackground(RendererContext& context, RendererStateBuffers& buffers) {
  if (*boundValue != lastValue) { // bound value changed elsewhere -> refresh display
    lastValue = *boundValue;
    updateThumbPosition(context, lastValue);
  }

  buffers.bindControlBuffer(context.renderer(), ControlBufferType::regular);
  controlMesh.draw(context.renderer());
  fillerMesh.draw(context.renderer());
  thumbMesh.draw(context.renderer());
}

void Ruler::drawLabels(RendererContext& context, RendererStateBuffers& buffers, bool isActive) {
  if (labelMesh.width() || suffixMesh.width()) {
    buffers.bindLabelBuffer(context.renderer(), isEnabled()
                                                ? (isActive ? LabelBufferType::active : LabelBufferType::regular)
                                                : LabelBufferType::disabled);
    labelMesh.draw(context.renderer());
    suffixMesh.draw(context.renderer());
  }
}
