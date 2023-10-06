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
#include <cstring>
#include "menu/controls/slider.h"

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

// ---

void Slider::init(RendererContext& context, const char32_t* label, int32_t x, int32_t y,
                  const float arrowColor[4], ComboBoxOption* values, size_t valueCount) {
  auto& font = context.getFont(FontType::labels);
  const uint32_t sliderHeight = font.XHeight() + (paddingY << 1); // width==height
  const uint32_t labelNamesY = y + (int32_t)paddingY;

  // create label
  uint32_t labelWidthWithMargin = 0;
  if (label != nullptr && *label != (char32_t)0) {
    labelMesh = TextMesh(*context.renderer, font, label, context.pixelSizeX, context.pixelSizeY, x, labelNamesY);
    labelWidthWithMargin = (minLabelWidth >= labelMesh.width()) ? minLabelWidth + labelMargin() : labelMesh.width() + labelMargin();
  }
  // create options
  const uint32_t optionCenterX = x + (int32_t)labelWidthWithMargin + (int32_t)(fixedSliderWidth << 1);
  selectableValues.reserve(valueCount);
  for (size_t remainingOptions = valueCount; remainingOptions; --remainingOptions, ++values) {
    selectableValues.emplace_back(context, font, values->name.get(), optionCenterX, labelNamesY, values->value);
  }

  // create arrows
  const uint32_t arrowHeight = font.XHeight() + paddingY;
  const uint32_t arrowWidth = (arrowHeight >> 2);
  const uint32_t arrowMargin = (paddingY >> 1);
  const float shadowColor[4]{ 0.f,0.f,0.f,0.15f };

  std::vector<ControlVertex> vertices;
  vertices.resize(8);
  ControlVertex* vertexIt = vertices.data();
  float arrowEndX = (float)arrowMargin + (float)arrowWidth*2.5f;
  setControlVertex(*vertexIt,     shadowColor, arrowEndX,                                    -(float)(arrowMargin - 1)); // drop shadow
  setControlVertex(*(++vertexIt), shadowColor, (float)(arrowMargin + arrowWidth - 1),        -(float)(sliderHeight >> 1));
  setControlVertex(*(++vertexIt), shadowColor, (float)(arrowMargin + (arrowWidth << 1) + 1), -(float)(sliderHeight >> 1));
  setControlVertex(*(++vertexIt), shadowColor, arrowEndX,                                    -(float)(sliderHeight - arrowMargin + 1));
  setControlVertex(*vertexIt,     arrowColor, arrowEndX,                                -(float)arrowMargin); // arrow
  setControlVertex(*(++vertexIt), arrowColor, (float)(arrowMargin + arrowWidth),        -(float)(sliderHeight >> 1));
  setControlVertex(*(++vertexIt), arrowColor, (float)(arrowMargin + (arrowWidth << 1)), -(float)(sliderHeight >> 1));
  setControlVertex(*(++vertexIt), arrowColor, arrowEndX,                                -(float)(sliderHeight - arrowMargin));
  std::vector<ControlVertex> verticesArrowRight = vertices;
  std::vector<uint32_t> indices{ 0,2,1, 1,2,3,  4,6,5, 5,6,7 };
  arrowLeftMesh = ControlMesh(*context.renderer, std::move(vertices), indices,
                              context.pixelSizeX, context.pixelSizeY, x + labelWidthWithMargin, y, sliderHeight, sliderHeight);

  vertexIt = verticesArrowRight.data(); // right arrow = mirrored version of left arrow
  vertexIt->position[0] = (float)sliderHeight - vertexIt->position[0];
  ++vertexIt;
  vertexIt->position[0] = (float)sliderHeight - vertexIt->position[0];
  ++vertexIt;
  vertexIt->position[0] = (float)sliderHeight - vertexIt->position[0];
  ++vertexIt;
  vertexIt->position[0] = (float)sliderHeight - vertexIt->position[0];
  ++vertexIt;
  vertexIt->position[0] = (float)sliderHeight - vertexIt->position[0];
  ++vertexIt;
  vertexIt->position[0] = (float)sliderHeight - vertexIt->position[0];
  ++vertexIt;
  vertexIt->position[0] = (float)sliderHeight - vertexIt->position[0];
  ++vertexIt;
  vertexIt->position[0] = (float)sliderHeight - vertexIt->position[0];
  arrowRightMesh = ControlMesh(*context.renderer, std::move(verticesArrowRight), indices, context.pixelSizeX, context.pixelSizeY,
                                x + labelWidthWithMargin + fixedSliderWidth - sliderHeight, y, sliderHeight, sliderHeight);
}

void Slider::move(RendererContext& context, int32_t x, int32_t y) {
  const uint32_t oldOriginX = labelMesh.x();
  const uint32_t labelNamesY = y + (int32_t)paddingY;

  labelMesh.move(*context.renderer, context.pixelSizeX, context.pixelSizeY, x, labelNamesY);
  for (auto& option : selectableValues) {
    option.nameMesh.move(*context.renderer, context.pixelSizeX, context.pixelSizeY, x + option.nameMesh.x() - oldOriginX, labelNamesY);
  }
}

// ---

void Slider::click(int32_t mouseX) {
  if (mouseX >= arrowLeftMesh.x() && mouseX < arrowLeftMesh.x() + (int32_t)arrowLeftMesh.width()) {
    selectPrevious();
  }
  else if (mouseX >= arrowRightMesh.x() && mouseX < arrowRightMesh.x() + (int32_t)arrowRightMesh.width()) {
    selectNext();
  }
}

void Slider::selectPrevious() {
  if (isEnabled() && selectedIndex > 0) {
    --selectedIndex;
    onChange(operationId, selectableValues[selectedIndex].value);
  }
}

void Slider::selectNext() {
  if (isEnabled() && selectedIndex + 1 < (int32_t)selectableValues.size()) {
    ++selectedIndex;
    onChange(operationId, selectableValues[selectedIndex].value);
  }
}

// ---

bool Slider::drawBackground(RendererContext& context, int32_t mouseX, int32_t mouseY,
                            Buffer<ResourceUsage::staticGpu>& hoverPressedVertexUniform) {
  if (isEnabled() && mouseY >= arrowLeftMesh.y() && mouseY < arrowLeftMesh.y() + (int32_t)arrowLeftMesh.height()) { // hover
    if (mouseX >= arrowLeftMesh.x() && mouseX < arrowLeftMesh.x() + (int32_t)arrowLeftMesh.width()) {
      arrowRightMesh.draw(*context.renderer);
      context.renderer->bindVertexUniforms(0, hoverPressedVertexUniform.handlePtr(), 1);
      arrowLeftMesh.draw(*context.renderer);
      return true;
    }
    else if (mouseX >= arrowRightMesh.x() && mouseX < arrowRightMesh.x() + (int32_t)arrowRightMesh.width()) {
      arrowLeftMesh.draw(*context.renderer);
      context.renderer->bindVertexUniforms(0, hoverPressedVertexUniform.handlePtr(), 1);
      arrowRightMesh.draw(*context.renderer);
      return true;
    }
  }
  arrowLeftMesh.draw(*context.renderer);
  arrowRightMesh.draw(*context.renderer);
  return false;
}