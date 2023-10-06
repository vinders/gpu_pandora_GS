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
#include "menu/controls/combo_box.h"

using namespace video_api;
using namespace display;
using namespace display::controls;
using namespace menu::controls;

#define ARROW_WIDTH  9
#define ARROW_HEIGHT 5


static inline void setControlVertex(ControlVertex& outVertex, const float rgba[4], float x, float y) {
  float* position = outVertex.position;
  *position = x;
  *(++position) = y;
  *(++position) = 0.f; // z
  *(++position) = 1.f; // w
  memcpy(outVertex.color, rgba, 4*sizeof(float));
}
static inline void fillHoverColor(const float dropdownColor[4], float outColor[4]) {
  float gray = (dropdownColor[0] + dropdownColor[1] + dropdownColor[2]) * 0.333f;
  gray *= (gray >= 0.5f) ? 0.8f : 1.25f;
  *outColor = gray;     // r
  *(++outColor) = gray; // g
  *(++outColor) = gray; // b
  *(++outColor) = 1.f; // a
}

// ---

void ComboBox::init(RendererContext& context, const char32_t* label, int32_t x, int32_t y,
                    const float color[4], const float dropdownColor[4], ComboBoxOption* values, size_t valueCount) {
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

  // create options
  uint32_t longestOptionNameWidth = 0;
  {
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
  }

  // create background
  uint32_t boxWidth = longestOptionNameWidth + (paddingX << 2);
  if (boxWidth < minBoxWidth)
    boxWidth = minBoxWidth;
  {
    const float semiFactor = 1.f - (paddingY / boxHeight) * 0.2f;
    const float colorSemi[4]{ color[0]*semiFactor, color[1]*semiFactor, color[2]*semiFactor, color[3] };
    const float colorDarker[4]{ color[0]*0.8f, color[1]*0.8f, color[2]*0.8f, color[3] };
    const uint32_t cornerCutWidth = boxWidth - (paddingY << 1);
    std::vector<ControlVertex> vertices;
    vertices.resize(24);
    ControlVertex* vertexIt = vertices.data();
    setControlVertex(*vertexIt,     color,       0.f,                   0.f); // box background
    setControlVertex(*(++vertexIt), color,       (float)cornerCutWidth, 0.f);
    setControlVertex(*(++vertexIt), colorDarker, 0.f,                   -(float)boxHeight);
    setControlVertex(*(++vertexIt), colorSemi,   (float)boxWidth,       -(float)paddingY);
    setControlVertex(*(++vertexIt), colorDarker, (float)boxWidth,       -(float)boxHeight);
    setControlVertex(*(++vertexIt), colorDarker, 0.f, 0.f); // border left
    setControlVertex(*(++vertexIt), colorDarker, 1.f, 0.f);
    setControlVertex(*(++vertexIt), colorDarker, 0.f, -(float)boxHeight);
    setControlVertex(*(++vertexIt), colorDarker, 1.f, -(float)boxHeight);
    setControlVertex(*(++vertexIt), colorDarker, (float)(boxWidth-1), -(float)paddingY); // border right
    setControlVertex(*(++vertexIt), colorDarker, (float)boxWidth,     -(float)paddingY);
    setControlVertex(*(++vertexIt), colorDarker, (float)(boxWidth-1), -(float)boxHeight);
    setControlVertex(*(++vertexIt), colorDarker, (float)boxWidth,     -(float)boxHeight);
    setControlVertex(*(++vertexIt), colorDarker, 1.f,                   0.f); // border top
    setControlVertex(*(++vertexIt), colorDarker, (float)cornerCutWidth, 0.f);
    setControlVertex(*(++vertexIt), colorDarker, 1.f,                   -1.f);
    setControlVertex(*(++vertexIt), colorDarker, (float)cornerCutWidth, -1.f);
    setControlVertex(*(++vertexIt), colorDarker, (float)cornerCutWidth, 0.f); // border corner
    setControlVertex(*(++vertexIt), colorDarker, (float)boxWidth,       -(float)paddingY);
    setControlVertex(*(++vertexIt), colorDarker, (float)cornerCutWidth, -1.f);
    setControlVertex(*(++vertexIt), colorDarker, (float)(boxWidth-1),   -(float)(paddingY+1));
    const float colorArrow[4]{ color[0]*0.15f, color[1]*0.15f, color[2]*0.15f, 0.75f };
    const int32_t arrowX = static_cast<int32_t>(boxWidth - (paddingX << 1));
    const int32_t arrowY = y + static_cast<int32_t>((boxHeight - ARROW_HEIGHT) >> 1);
    setControlVertex(*(++vertexIt), colorArrow, (float)arrowX,                     -(float)arrowY); // down arrow
    setControlVertex(*(++vertexIt), colorArrow, (float)(arrowX + ARROW_WIDTH),     -(float)arrowY);
    setControlVertex(*(++vertexIt), colorArrow, (float)arrowX + ARROW_WIDTH*0.5f, -(float)(arrowY + ARROW_HEIGHT));
    std::vector<uint32_t> indices{ 0,1,2, 1,3,2, 2,3,4,  5,6,7, 7,6,8,  9,10,11, 11,10,12,
                                   13,14,15, 15,14,16,  17,18,19, 19,18,20,  21,22,23 };

    controlMesh = ControlMesh(*context.renderer, std::move(vertices), indices, context.pixelSizeX, context.pixelSizeY,
                              x + (int32_t)labelWidthWithMargin, y, boxWidth, boxHeight);
  }

  // create drop-down area
  const uint32_t dropdownHeight = (!selectableValues.empty()) ? boxHeight*(uint32_t)selectableValues.size() : paddingY;
  std::vector<ControlVertex> vertices;
  vertices.resize(4);
  ControlVertex* vertexIt = vertices.data();
  setControlVertex(*vertexIt,     dropdownColor, 0.f,             0.f);
  setControlVertex(*(++vertexIt), dropdownColor, (float)boxWidth, 0.f);
  setControlVertex(*(++vertexIt), dropdownColor, 0.f,             -(float)dropdownHeight);
  setControlVertex(*(++vertexIt), dropdownColor, (float)boxWidth, -(float)dropdownHeight);
  std::vector<uint32_t> indices = { 0,1,2, 2,1,3 };

  dropdownMesh = ControlMesh(*context.renderer, std::move(vertices), indices, context.pixelSizeX, context.pixelSizeY,
                              x + (int32_t)labelWidthWithMargin, y + (int32_t)boxHeight, boxWidth, dropdownHeight);
  
  float colorHover[4];
  fillHoverColor(dropdownColor, colorHover);
  vertices.resize(4);
  vertexIt = vertices.data();
  setControlVertex(*vertexIt,     colorHover, 0.f,             0.f);
  setControlVertex(*(++vertexIt), colorHover, (float)boxWidth, 0.f);
  setControlVertex(*(++vertexIt), colorHover, 0.f,             -(float)boxHeight);
  setControlVertex(*(++vertexIt), colorHover, (float)boxWidth, -(float)boxHeight);

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
  if (isListOpen && !selectableValues.empty()) {
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
  if (hoverIndex + 1 < (int32_t)selectableValues.size())
    moveDropdownHover(context, ++hoverIndex);
}

// ---

void ComboBox::drawBackground(RendererContext& context) {
  if (isListOpen) {
    if (isEnabled()) {
      dropdownMesh.draw(*context.renderer);
      if (hoverIndex >= 0) // hover entry background
        dropdownHoverMesh.draw(*context.renderer);
    }
    else isListOpen = false;
  }
  controlMesh.draw(*context.renderer);
}
