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
#include "menu/controls/geometry_generator.h"
#include "menu/controls/slider.h"

using namespace video_api;
using namespace display;
using namespace display::controls;
using namespace menu::controls;
using namespace menu;

ControlType Slider::Type() const noexcept { return ControlType::slider; }


// -- init/resize geometry -- --------------------------------------------------

void Slider::init(RendererContext& context, const char32_t* label, int32_t x, int32_t labelY,
                  const float arrowColor[4], ComboBoxOption* values, size_t valueCount) {
  auto& font = context.getFont(FontType::labels);
  const uint32_t sliderHeight = font.XHeight() + (paddingY << 1); // width==height
  const int32_t y = labelY - (int32_t)paddingY;

  // create label
  labelMesh = TextMesh(context.renderer(), font, label, context.pixelSizeX(), context.pixelSizeY(), x, labelY);
  uint32_t labelWidthWithMargin = (minLabelWidth >= labelMesh.width()) ? minLabelWidth : labelMesh.width();
  if (labelWidthWithMargin)
    labelWidthWithMargin += labelMargin();
  
  // create options
  const uint32_t optionCenterX = x + (int32_t)labelWidthWithMargin + (int32_t)(fixedSliderWidth >> 1) - 2;
  selectableValues.reserve(valueCount);
  for (size_t remainingOptions = valueCount; remainingOptions; --remainingOptions, ++values) {
    selectableValues.emplace_back(context, font, values->name.get(), optionCenterX, labelY, values->value);
  }

  // create arrows
  const uint32_t arrowWidth = (sliderHeight >> 1) - 1;
  const uint32_t arrowNeckTop = (sliderHeight >> 1) - paddingY;
  const uint32_t arrowNeckBottom = (sliderHeight >> 1) + paddingY - 2;
  const float shadowColor[4]{ arrowColor[0]*0.65f,arrowColor[1]*0.65f,arrowColor[2]*0.65f,1.f };

  std::vector<ControlVertex> vertices(static_cast<size_t>(15u));
  ControlVertex* vertexIt = vertices.data();
  GeometryGenerator::fillObliqueRectangleVertices(vertexIt, shadowColor, 0.f, (float)(arrowWidth + 1), // left arrow shadow
                                                  -(float)((sliderHeight >> 1) - 1), -(float)(sliderHeight >> 1),
                                                  -(float)((sliderHeight >> 1) - 1));
  vertexIt += 4;
  GeometryGenerator::fillControlVertex(*vertexIt,     shadowColor, 0.f, -(float)((sliderHeight >> 1) - 1)); // left arrow
  GeometryGenerator::fillControlVertex(*(++vertexIt), arrowColor, (float)(arrowWidth + 1), 0.f);
  GeometryGenerator::fillControlVertex(*(++vertexIt), arrowColor, (float)(arrowWidth + 1), -(float)(sliderHeight - 2));

  GeometryGenerator::fillRectangleVertices(++vertexIt, shadowColor, (float)(arrowWidth + 1), (float)(sliderHeight - 1),
                                           -(float)arrowNeckBottom, -(float)(arrowNeckBottom + 1)); // left arrow neck shadow
  vertexIt += 4;
  GeometryGenerator::fillRectangleVertices(vertexIt, arrowColor, (float)(arrowWidth + 1), (float)(sliderHeight - 1),
                                           -(float)arrowNeckTop, -(float)arrowNeckBottom); // left arrow neck
  std::vector<uint32_t> indices{ 0,1,2, 2,1,3,  4,5,6,  7,8,9, 9,8,10,  11,12,13, 13,12,14 };
  arrowLeftMesh = ControlMesh(context.renderer(), std::move(vertices), indices,
                              context.pixelSizeX(), context.pixelSizeY(), x + labelWidthWithMargin, y, sliderHeight, sliderHeight);

  vertices = arrowLeftMesh.relativeVertices(); // right arrow = mirrored version of left arrow
  const auto* endIt = vertices.data() + (intptr_t)vertices.size();
  for (ControlVertex* it = vertices.data(); it < endIt; ++it) { 
    it->position[0] = (float)sliderHeight - it->position[0];
  }
  indices = { 0,2,1, 1,2,3,  4,6,5,  8,7,9, 8,9,10,  12,11,13, 12,13,14 };
  arrowRightMesh = ControlMesh(context.renderer(), std::move(vertices), indices, context.pixelSizeX(), context.pixelSizeY(),
                                x + labelWidthWithMargin + fixedSliderWidth - sliderHeight, y, sliderHeight, sliderHeight);
}

// ---

void Slider::move(RendererContext& context, int32_t x, int32_t labelY) {
  const uint32_t oldOriginX = labelMesh.x();
  const int32_t y = labelY - (int32_t)paddingY;

  labelMesh.move(context.renderer(), context.pixelSizeX(), context.pixelSizeY(), x, labelY);
  uint32_t labelWidthWithMargin = (minLabelWidth >= labelMesh.width()) ? minLabelWidth : labelMesh.width();
  if (labelWidthWithMargin)
    labelWidthWithMargin += labelMargin();
  
  for (auto& option : selectableValues) {
    option.nameMesh.move(context.renderer(), context.pixelSizeX(), context.pixelSizeY(),
                         x + option.nameMesh.x() - oldOriginX, labelY);
  }

  const uint32_t sliderHeight = labelMesh.height() + (paddingY << 1);
  arrowLeftMesh.move(context.renderer(), context.pixelSizeX(), context.pixelSizeY(), x + labelWidthWithMargin, y);
  arrowRightMesh.move(context.renderer(), context.pixelSizeX(), context.pixelSizeY(),
                      x + labelWidthWithMargin + fixedSliderWidth - sliderHeight, y);
}


// -- accessors/operations -- --------------------------------------------------

ControlStatus Slider::getStatus(int32_t mouseX, int32_t mouseY) const noexcept {
  return isEnabled() ? (isHover(mouseX, mouseY) ? ControlStatus::hover : ControlStatus::regular) : ControlStatus::disabled;
}

// ---

bool Slider::click(RendererContext&, int32_t mouseX) {
  int32_t extraMargin = (int32_t)(arrowLeftMesh.width() >> 2);
  if (mouseX >= arrowLeftMesh.x() - extraMargin && mouseX < arrowLeftMesh.x() + (int32_t)arrowLeftMesh.width() + extraMargin)
    selectPrevious();
  else if (mouseX >= arrowRightMesh.x() - extraMargin && mouseX < arrowRightMesh.x() + (int32_t)arrowRightMesh.width() + extraMargin)
    selectNext();
  return false;
}

void Slider::selectPrevious() {
  if (isEnabled() && selectedIndex > 0) {
    --selectedIndex;
    if (onChange != nullptr)
      onChange(operationId, (uint32_t)selectableValues[selectedIndex].value);
  }
}

void Slider::selectNext() {
  if (isEnabled() && selectedIndex + 1 < (int32_t)selectableValues.size()) {
    ++selectedIndex;
    if (onChange != nullptr)
      onChange(operationId, (uint32_t)selectableValues[selectedIndex].value);
  }
}


// -- rendering -- -------------------------------------------------------------

void Slider::drawBackground(RendererContext& context, int32_t mouseX, RendererStateBuffers& buffers, bool isActive) {
  ControlBufferType leftBuffer = ControlBufferType::disabled;
  ControlBufferType rightBuffer = ControlBufferType::disabled;
  if (isEnabled()) {
    if (selectedIndex > 0)
      leftBuffer = (isActive && mouseX >= arrowLeftMesh.x() && mouseX < arrowLeftMesh.x() + (int32_t)arrowLeftMesh.width())
                 ? ControlBufferType::active : ControlBufferType::regular;
    if (selectedIndex + 1 < (int32_t)selectableValues.size())
      rightBuffer = (isActive && mouseX >= arrowRightMesh.x() && mouseX < arrowRightMesh.x() + (int32_t)arrowRightMesh.width())
                  ? ControlBufferType::active : ControlBufferType::regular;
  }

  buffers.bindControlBuffer(context.renderer(), leftBuffer);
  arrowLeftMesh.draw(context.renderer());
  buffers.bindControlBuffer(context.renderer(), rightBuffer);
  arrowRightMesh.draw(context.renderer());
}

void Slider::drawLabels(RendererContext& context, RendererStateBuffers& buffers, bool isActive) {
  buffers.bindLabelBuffer(context.renderer(), isEnabled()
                                              ? (isActive ? LabelBufferType::active : LabelBufferType::regular)
                                              : LabelBufferType::disabled);
  labelMesh.draw(context.renderer());
  if (selectedIndex >= 0)
    selectableValues[selectedIndex].nameMesh.draw(context.renderer());
}
