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
#include "menu/controls/combo_box.h"

using namespace video_api;
using namespace display;
using namespace display::controls;
using namespace menu::controls;

#define ARROW_WIDTH  9
#define ARROW_HEIGHT 5


static inline void setVertexPosition(float* position, float x, float y) {
  *position = x;
  *(++position) = y;
  *(++position) = 0.f; // z
  *(++position) = 1.f; // w
}

// ---

void ComboBox::init(RendererContext& context, const char32_t* label, int32_t x, int32_t y,
                    ComboBoxEntry* values, size_t valueCount) {
  auto& optionFont = context.getFont(FontType::inputText);
  const uint32_t boxHeight = optionFont.XHeight() + (paddingY << 1);

  // create label
  uint32_t labelWidthWithMargin = 0;
  if (label != nullptr && *label != (char32_t)0) {
    auto& labelFont = context.getFont(FontType::labels);

    const int32_t labelY = y + paddingY - ((int32_t)labelFont.XHeight() - (int32_t)optionFont.XHeight())/2;
    labelMesh = TextMesh(*context.renderer, labelFont, label, context.pixelSizeX, context.pixelSizeY, x, labelY);
    labelWidthWithMargin = (minLabelWidth >= labelMesh.width()) ? minLabelWidth + labelMargin() : labelMesh.width() + labelMargin();
  }

  // create option names
  uint32_t longestOptionNameWidth = 0;
  const int32_t optionNameX = x + labelWidthWithMargin + paddingX;
  int32_t optionNameY = y + (int32_t)boxHeight;
  selectableValues.reserve(valueCount);
  for (size_t remainingOptions = valueCount; remainingOptions; --remainingOptions, ++values, optionNameY += (int32_t)boxHeight) {
    const auto& entry = selectableValues.emplace_back(context, optionFont, values->name.get(),
                                                      optionNameX, optionNameY, values->value);
    if (entry.nameMesh.width() > longestOptionNameWidth)
      longestOptionNameWidth = entry.nameMesh.width();
  }
  if (selectedIndex >= 0) { // copy selected option in combo-box
    selectableValues[selectedIndex].nameMesh.cloneAtLocation(*context.renderer, context.pixelSizeX, context.pixelSizeY,
                                                             optionNameX, y + (int32_t)paddingY, selectedNameMesh);
  }

  // create background
  uint32_t boxWidth = longestOptionNameWidth + (paddingX << 2);
  if (boxWidth < minBoxWidth)
    boxWidth = minBoxWidth;

  std::vector<ControlVertex> vertices;
  vertices.resize(5);
  ControlVertex* vertexIt = vertices.data();
  setVertexPosition(vertexIt->position,     0.f,             0.f);
  setVertexPosition((++vertexIt)->position, static_cast<float>(boxWidth - (paddingY << 1)), 0.f);
  setVertexPosition((++vertexIt)->position, 0.f,             -(float)boxHeight);
  setVertexPosition((++vertexIt)->position, (float)boxWidth, -(float)paddingY);
  setVertexPosition((++vertexIt)->position, (float)boxWidth, -(float)boxHeight);
  std::vector<uint32_t> indices{ 0,1,2, 1,3,2, 2,3,4 };

  controlMesh = ControlMesh(*context.renderer, std::move(vertices), indices, context.pixelSizeX, context.pixelSizeY,
                            x + (int32_t)labelWidthWithMargin, y, boxWidth, boxHeight);
  
  // create arrow
  vertices.resize(3);
  vertexIt = vertices.data();
  setVertexPosition(vertexIt->position,     0.f,                     0.f);
  setVertexPosition((++vertexIt)->position, (float)ARROW_WIDTH,      0.f);
  setVertexPosition((++vertexIt)->position, (float)ARROW_WIDTH*0.5f, -(float)ARROW_HEIGHT);
  indices.resize(3);

  const int32_t arrowY = y + static_cast<int32_t>((boxHeight - ARROW_HEIGHT) >> 1);
  arrowMesh = ControlMesh(*context.renderer, std::move(vertices), indices, context.pixelSizeX, context.pixelSizeY,
                          controlMesh.x() + static_cast<int32_t>(boxWidth - (paddingX << 1)), arrowY, ARROW_WIDTH, ARROW_HEIGHT);

  // create drop-down area
  const uint32_t dropdownHeight = (!selectableValues.empty()) ? boxHeight*(uint32_t)selectableValues.size() : paddingY;
  vertices.resize(4);
  vertexIt = vertices.data();
  setVertexPosition(vertexIt->position,     0.f,             0.f);
  setVertexPosition((++vertexIt)->position, (float)boxWidth, 0.f);
  setVertexPosition((++vertexIt)->position, 0.f,             -(float)dropdownHeight);
  setVertexPosition((++vertexIt)->position, (float)boxWidth, -(float)dropdownHeight);
  indices = { 0,1,2, 2,1,3 };

  dropdownMesh = ControlMesh(*context.renderer, std::move(vertices), indices, context.pixelSizeX, context.pixelSizeY,
                             x + (int32_t)labelWidthWithMargin, y + (int32_t)boxHeight, boxWidth, dropdownHeight);

  vertices.resize(4);
  vertexIt = vertices.data();
  setVertexPosition(vertexIt->position,     0.f,             0.f);
  setVertexPosition((++vertexIt)->position, (float)boxWidth, 0.f);
  setVertexPosition((++vertexIt)->position, 0.f,             -(float)boxHeight);
  setVertexPosition((++vertexIt)->position, (float)boxWidth, -(float)boxHeight);
  indices = { 0,1,2, 2,1,3 };

  const uint32_t dropdownHoverY = (selectedIndex >= 0) ? (y + (int32_t)boxHeight*(selectedIndex+1)) : (y + (int32_t)boxHeight);
  dropdownHoverMesh = ControlMesh(*context.renderer, std::move(vertices), indices, context.pixelSizeX, context.pixelSizeY,
                                  x + (int32_t)labelWidthWithMargin, dropdownHoverY, boxWidth, boxHeight);
}

// ---

void ComboBox::move(RendererContext& context, int32_t x, int32_t y) {
  uint32_t labelWidthWithMargin = 0;
  if (labelMesh.width()) {
    const int32_t labelY = y + ((int32_t)labelMesh.y() - (int32_t)controlMesh.y());
    labelMesh.move(*context.renderer, context.pixelSizeX, context.pixelSizeY, x, labelY);
    labelWidthWithMargin = (minLabelWidth >= labelMesh.width()) ? minLabelWidth + labelMargin() : labelMesh.width() + labelMargin();
  }

  const int32_t optionNameX = x + labelWidthWithMargin + paddingX;
  int32_t optionNameY = y + (int32_t)controlMesh.height();
  for (auto& value : selectableValues) {
    value.nameMesh.move(*context.renderer, context.pixelSizeX, context.pixelSizeY, optionNameX, optionNameY);
    optionNameY += (int32_t)controlMesh.height();
  }
  if (selectedNameMesh.width())
    selectedNameMesh.move(*context.renderer, context.pixelSizeX, context.pixelSizeY, optionNameX, y + (int32_t)paddingY);

  controlMesh.move(*context.renderer, context.pixelSizeX, context.pixelSizeY, x + (int32_t)labelWidthWithMargin, y);

  const int32_t arrowY = y + static_cast<int32_t>((controlMesh.height() - ARROW_HEIGHT) >> 1);
  arrowMesh.move(*context.renderer, context.pixelSizeX, context.pixelSizeY,
                 controlMesh.x() + static_cast<int32_t>(controlMesh.width() - (paddingX << 1)), arrowY);

  const uint32_t dropdownHoverY = (selectedIndex >= 0) ? (y + (int32_t)controlMesh.height()*(selectedIndex+1)) : (y + (int32_t)controlMesh.height());
  dropdownMesh.move(*context.renderer, context.pixelSizeX, context.pixelSizeY, x + (int32_t)labelWidthWithMargin, y + (int32_t)controlMesh.height());
  dropdownHoverMesh.move(*context.renderer, context.pixelSizeX, context.pixelSizeY, x + (int32_t)labelWidthWithMargin, dropdownHoverY);
}

void ComboBox::moveDropdownHover(RendererContext& context, int32_t valueIndex) {
  if (valueIndex < 0 || valueIndex >= (int32_t)selectableValues.size())
    return;
  const int32_t hoverY = selectableValues[valueIndex].nameMesh.y() - (int32_t)paddingY;
  dropdownHoverMesh.move(*context.renderer, context.pixelSizeX, context.pixelSizeY, dropdownHoverMesh.x(), hoverY);
}

// ---

void ComboBox::click(RendererContext& context) {
  if (isEnabled()) {
    if (isListOpen) {
      if (hoverIndex > -1 && selectedIndex != hoverIndex && hoverIndex < (int32_t)selectableValues.size()) {
        selectedIndex = hoverIndex; // change selected index + update selected text
        selectableValues[hoverIndex].nameMesh.cloneAtLocation(*context.renderer, context.pixelSizeX, context.pixelSizeY,
                                                              selectedNameMesh.x(), selectedNameMesh.y(), selectedNameMesh);

        onChange(operationId, selectableValues[selectedIndex].value); // report value change
      }
      isListOpen = false;
    }
    else {
      hoverIndex = (selectedIndex >= 0) ? selectedIndex : 0;
      moveDropdownHover(context, hoverIndex);
      isListOpen = true;
    }
  }
}

void ComboBox::mouseMove(RendererContext& context, int32_t mouseY) {
  if (!selectableValues.empty()) {
    const auto& firstMesh = selectableValues[0].nameMesh;
    mouseY -= firstMesh.y() - (int32_t)paddingY; // absolute to relative
    mouseY /= static_cast<int32_t>(firstMesh.height() + (paddingY << 1)); // height to index (divide by entry height)

    if (mouseY != hoverIndex && mouseY >= 0 && mouseY < (int32_t)selectableValues.size()) {
      hoverIndex = mouseY;
      moveDropdownHover(context, hoverIndex);
    }
  }
}

void ComboBox::selectPrevious(RendererContext& context) {
  if (hoverIndex > 0)
    moveDropdownHover(context, --hoverIndex);
}
void ComboBox::selectNext(RendererContext& context) {
  if (hoverIndex < (int32_t)selectableValues.size())
    moveDropdownHover(context, ++hoverIndex);
}

// ---

void ComboBox::drawDropdown(RendererContext& context, const BufferHandle& backUniform, const BufferHandle& hoverUniform) {
  if (isListOpen) {
    if (isEnabled()) {
      context.renderer->bindFragmentUniforms(0, &backUniform, 1); // dropdown background
      dropdownMesh.draw(*context.renderer);

      if (hoverIndex >= 0) { // hover entry background
        context.renderer->bindFragmentUniforms(0, &hoverUniform, 1);
        dropdownHoverMesh.draw(*context.renderer);
      }
      const BufferHandle empty = nullptr; // unbind uniform
      context.renderer->bindFragmentUniforms(0, &empty, 1);
    }
    else isListOpen = false;
  }
}
