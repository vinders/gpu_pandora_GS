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
#include "menu/controls/combo_box.h"

#if defined(_CPP_REVISION) && _CPP_REVISION == 14
# define EMPLACE_AND_GET(collection, result, ...)  collection.emplace_back(__VA_ARGS__); \
                                                   const auto& result = selectableValues.back()
#else
# define EMPLACE_AND_GET(collection, result, ...)  const auto& result = collection.emplace_back(__VA_ARGS__)
#endif

using namespace video_api;
using namespace display;
using namespace display::controls;
using namespace menu::controls;
using namespace menu;

ControlType ComboBox::Type() const noexcept { return ControlType::comboBox; }


// -- init/resize geometry -- --------------------------------------------------

#define ARROW_WIDTH  9
#define ARROW_HEIGHT 5

static inline void generateBackground(float width, uint32_t height, ComboBoxStyle style,
                                      const float primaryColor[4], const float topColor[4],
                                      std::vector<ControlVertex>& outVertices, std::vector<uint32_t>& outIndices) {
  const float gradientColors[3][4]{
    { primaryColor[0], primaryColor[1], primaryColor[2], primaryColor[3] },
    { topColor[0], topColor[1], topColor[2], topColor[3] },
    { primaryColor[0]*0.8f, primaryColor[1]*0.8f, primaryColor[2]*0.8f, primaryColor[3] },
  };
  const float colorBorder[4]{ primaryColor[0]*0.75f, primaryColor[1]*0.75f, primaryColor[2]*0.75f, primaryColor[3] };
  const float colorArrow[4] { primaryColor[0]*0.3f,  primaryColor[1]*0.3f,  primaryColor[2]*0.3f,  0.525f };

  ControlVertex* vertexIt;
  if (style == ComboBoxStyle::cutCorner) { // rectangle with cut corner
    outVertices.resize(static_cast<size_t>(10 + 20 + 3));
    vertexIt = outVertices.data();
    GeometryGenerator::fillTopRightCutRectangleVertices(vertexIt, gradientColors, 0.f, width, // background (cut corner)
                                                        0.f, -(float)height, (float)Control::comboBoxPaddingY());
    vertexIt += 10;
    GeometryGenerator::fillTopRightCutBorderVertices(vertexIt, colorBorder, 0.f, width, // borders (cut corner)
                                                     0.f, -(float)height, (float)Control::comboBoxPaddingY());
    vertexIt += 20;
    outIndices = { 0,1,2,2,1,3, 2,3,4,4,3,5, 6,7,8,8,7,9,  10,11,12,12,11,13,  14,15,16,16,15,17,
                   18,19,20,20,19,21,  22,23,24,24,23,25,  26,27,28,28,27,29,  30,31,32 };
  }
  else { // ComboBoxStyle::classic -> simple rectangle
    outVertices.resize(static_cast<size_t>(10 + 16 + 3));
    vertexIt = outVertices.data();
    GeometryGenerator::fillDoubleGradientRectangleVertices(vertexIt, gradientColors, 0.f, width, // background
                                                           0.f, -(float)height, (float)Control::comboBoxPaddingY());
    vertexIt += 10;
    GeometryGenerator::fillRectangleBorderVertices(vertexIt, colorBorder, 0.f, width, 0.f, -(float)height); // borders
    vertexIt += 16;
    outIndices = { 0,1,2,2,1,3, 2,3,4,4,3,5, 6,7,8,8,7,9,  10,11,12,12,11,13,  14,15,16,16,15,17,
                   18,19,20,20,19,21,  22,23,24,24,23,25,  26,27,28 };
  }
  // down arrow
  GeometryGenerator::fillInvertedTriangleVertices(vertexIt, colorArrow,
                                                  width - static_cast<float>(Control::comboBoxPaddingY() + ARROW_WIDTH + 2),
                                                  -static_cast<float>((height + 2 - ARROW_HEIGHT) >> 1),
                                                  (float)ARROW_WIDTH, (float)ARROW_HEIGHT);
}

// ---

void ComboBox::init(RendererContext& context, const char32_t* label, int32_t x, int32_t labelY,
                    ComboBoxStyle style, const ControlColors<3>& colors, ComboBoxOption* values, size_t valueCount) {
  // create label
  auto& labelFont = context.getFont(FontType::labels);
  labelMesh = TextMesh(context.renderer(), labelFont, label, context.pixelSizeX(), context.pixelSizeY(), x, labelY);
  uint32_t labelWidthWithMargin = (minLabelWidth >= labelMesh.width()) ? minLabelWidth : labelMesh.width();
  if (labelWidthWithMargin)
    labelWidthWithMargin += labelMargin();

  // create drop-down options
  auto& optionFont = context.getFont(FontType::inputText);
  const uint32_t boxHeight = optionFont.XHeight() + (comboBoxPaddingY() << 1);
  const int32_t y = labelY - comboBoxPaddingY() + ((int32_t)labelFont.XHeight() - (int32_t)optionFont.XHeight())/2;

  uint32_t longestOptionNameWidth = 0;
  {
    const int32_t optionNameX = x + (int32_t)labelWidthWithMargin + (int32_t)comboBoxPaddingX();
    int32_t optionNameY = y + (int32_t)boxHeight + (int32_t)comboBoxPaddingY() + 1;
    selectableValues.reserve(valueCount);
    for (size_t remainingOptions = valueCount; remainingOptions; --remainingOptions, ++values, optionNameY += (int32_t)boxHeight) {
      EMPLACE_AND_GET(selectableValues, result,
                      context, optionFont, values->name.get(), optionNameX, optionNameY, values->value);
      if (result.nameMesh.width() > longestOptionNameWidth)
        longestOptionNameWidth = result.nameMesh.width();
    }
    if (selectedIndex >= 0) { // copy selected option as combo-box value
      selectableValues[selectedIndex].nameMesh.cloneAtLocation(context.renderer(), context.pixelSizeX(), context.pixelSizeY(),
                                                               optionNameX, y + (int32_t)comboBoxPaddingY() + 1, selectedNameMesh);
    }
  }
  uint32_t boxWidth = longestOptionNameWidth + (comboBoxPaddingX() << 2);
  if (boxWidth < minBoxWidth)
    boxWidth = minBoxWidth;

  // create background
  std::vector<ControlVertex> vertices;
  std::vector<uint32_t> indices;
  generateBackground((float)boxWidth, boxHeight, style, colors.colors[0], colors.colors[1], vertices, indices);
  controlMesh = ControlMesh(context.renderer(), std::move(vertices), indices, context.pixelSizeX(), context.pixelSizeY(),
                            x + (int32_t)labelWidthWithMargin, y, boxWidth, boxHeight);

  // create drop-down area
  vertices = std::vector<ControlVertex>(static_cast<size_t>(4 + 3*4));
  ControlVertex* vertexIt = vertices.data();
  const uint32_t dropdownHeight = (!selectableValues.empty()) ? boxHeight*(uint32_t)selectableValues.size() : comboBoxPaddingY();
  const float* dropdownColor = colors.colors[2];
  GeometryGenerator::fillRectangleVertices(vertexIt, dropdownColor,   // drop-down background
                                           0.f, (float)boxWidth, 0.f, -(float)dropdownHeight);
  vertexIt += 4;
  const float dropdownBorder[4]{ dropdownColor[0]*0.8f, dropdownColor[1]*0.8f, dropdownColor[2]*0.8f, dropdownColor[3] };
  GeometryGenerator::fillRectangleVertices(vertexIt, dropdownBorder,  // drop-down border bottom
                                           0.f, (float)boxWidth, -(float)(dropdownHeight-1), -(float)dropdownHeight);
  vertexIt += 4;
  GeometryGenerator::fillRectangleVertices(vertexIt, dropdownBorder,  // drop-down border left
                                           0.f, 1.f, 0.f, -(float)(dropdownHeight-1));
  vertexIt += 4;
  GeometryGenerator::fillRectangleVertices(vertexIt, dropdownBorder,  // drop-down border right
                                           (float)(boxWidth-1), (float)boxWidth, 0.f, -(float)(dropdownHeight-1));
  indices = { 0,1,2,2,1,3,  4,5,6,6,5,7,  8,9,10,10,9,11,  12,13,14,14,13,15 };

  dropdownMesh = ControlMesh(context.renderer(), std::move(vertices), indices, context.pixelSizeX(), context.pixelSizeY(),
                              x + (int32_t)labelWidthWithMargin, y + (int32_t)boxHeight, boxWidth, dropdownHeight);
  
  // create drop-down hover area
  vertices = std::vector<ControlVertex>(static_cast<size_t>(4));
  GeometryGenerator::fillRectangleVertices(vertices.data(), dropdownBorder,  // drop-down hover background
                                           0.f, (float)boxWidth, 0.f, -(float)boxHeight);
  indices = { 0,1,2,2,1,3 };

  const uint32_t dropdownHoverY = (selectedIndex >= 0) ? (y + (int32_t)boxHeight*(selectedIndex+1)) : (y + (int32_t)boxHeight);
  dropdownHoverMesh = ControlMesh(context.renderer(), std::move(vertices), indices, context.pixelSizeX(), context.pixelSizeY(),
                                  x + (int32_t)labelWidthWithMargin, dropdownHoverY, boxWidth, boxHeight);
}

// ---

void ComboBox::move(RendererContext& context, int32_t x, int32_t labelY) {
  const int32_t y = labelY - (labelMesh.y() - controlMesh.y());
  
  labelMesh.move(context.renderer(), context.pixelSizeX(), context.pixelSizeY(), x, labelY);
  uint32_t labelWidthWithMargin = (minLabelWidth >= labelMesh.width()) ? minLabelWidth : labelMesh.width();
  if (labelWidthWithMargin)
    labelWidthWithMargin += labelMargin();

  const int32_t optionNameX = x + labelWidthWithMargin + (int32_t)comboBoxPaddingX();
  int32_t optionNameY = y + (int32_t)controlMesh.height() + (int32_t)comboBoxPaddingY();
  for (auto& value : selectableValues) {
    value.nameMesh.move(context.renderer(), context.pixelSizeX(), context.pixelSizeY(), optionNameX, optionNameY);
    optionNameY += (int32_t)controlMesh.height();
  }
  if (selectedNameMesh.width())
    selectedNameMesh.move(context.renderer(), context.pixelSizeX(), context.pixelSizeY(), optionNameX, y + (int32_t)comboBoxPaddingY() + 1);

  controlMesh.move(context.renderer(), context.pixelSizeX(), context.pixelSizeY(), x + (int32_t)labelWidthWithMargin, y);

  const uint32_t dropdownHoverY = (selectedIndex >= 0) ? (y + (int32_t)controlMesh.height()*(selectedIndex+1)) : (y + (int32_t)controlMesh.height());
  dropdownMesh.move(context.renderer(), context.pixelSizeX(), context.pixelSizeY(), x + (int32_t)labelWidthWithMargin, y + (int32_t)controlMesh.height());
  dropdownHoverMesh.move(context.renderer(), context.pixelSizeX(), context.pixelSizeY(), x + (int32_t)labelWidthWithMargin, dropdownHoverY);
}

void ComboBox::moveDropdownHover(RendererContext& context, int32_t valueIndex) {
  if (valueIndex < 0 || valueIndex >= (int32_t)selectableValues.size())
    return;
  const int32_t hoverY = selectableValues[valueIndex].nameMesh.y() - (int32_t)comboBoxPaddingY() - 1;
  dropdownHoverMesh.move(context.renderer(), context.pixelSizeX(), context.pixelSizeY(), dropdownHoverMesh.x(), hoverY);
}

// ---

void ComboBox::updateLabel(RendererContext& context, const char32_t* label) {
  labelMesh = TextMesh(context.renderer(), context.getFont(FontType::labels), label,
                       context.pixelSizeX(), context.pixelSizeY(), labelMesh.x(), labelMesh.y());
  uint32_t labelWidthWithMargin = (minLabelWidth >= labelMesh.width()) ? minLabelWidth : labelMesh.width();
  if (labelWidthWithMargin)
    labelWidthWithMargin += labelMargin();

  if (labelMesh.x() + (int32_t)labelWidthWithMargin != controlMesh.x())
    move(context, controlMesh.x(), labelMesh.y());
}

void ComboBox::replaceValues(RendererContext& context, ComboBoxOption* values, size_t valueCount, int32_t selectedIndex_) {
  if (controlMesh.width() == 0)
    return;
  this->selectedIndex = (selectedIndex_ < (int32_t)valueCount) ? selectedIndex_ : -1;

  // re-create options
  selectableValues.clear();
  {
    auto& optionFont = context.getFont(FontType::inputText);
    const int32_t optionNameX = controlMesh.x() + (int32_t)comboBoxPaddingX();
    int32_t optionNameY = controlMesh.y() + (int32_t)controlMesh.height() + (int32_t)comboBoxPaddingY() + 1;
    selectableValues.reserve(valueCount);

    for (size_t remainingOptions = valueCount; remainingOptions; --remainingOptions, ++values, optionNameY += (int32_t)controlMesh.height()) {
      selectableValues.emplace_back(context, optionFont, values->name.get(), optionNameX, optionNameY, values->value);
    }
    if (selectedIndex >= 0) { // copy selected option in combo-box
      selectableValues[selectedIndex].nameMesh.cloneAtLocation(context.renderer(), context.pixelSizeX(), context.pixelSizeY(),
                                                               optionNameX, controlMesh.y() + (int32_t)comboBoxPaddingY() + 1, selectedNameMesh);
    }
  }

  // resize drop-down area
  const uint32_t dropdownHeight = (!selectableValues.empty()) ? controlMesh.height()*(uint32_t)selectableValues.size() : comboBoxPaddingY();
  if (dropdownHeight != dropdownMesh.height()) {
    std::vector<ControlVertex> vertices = dropdownMesh.relativeVertices();
    ControlVertex* vertexIt = vertices.data();
    GeometryGenerator::resizeRectangleVerticesY(vertexIt, -(float)dropdownHeight); // drop-down background
    vertexIt += 4;
    GeometryGenerator::moveRectangleVerticesY(vertexIt, -(float)(dropdownHeight-1), -(float)dropdownHeight); // drop-down border bottom
    vertexIt += 4;
    GeometryGenerator::resizeRectangleVerticesY(vertexIt, -(float)(dropdownHeight-1)); // drop-down border left
    vertexIt += 4;
    GeometryGenerator::resizeRectangleVerticesY(vertexIt, -(float)(dropdownHeight-1)); // drop-down border right
    dropdownMesh.update(context.renderer(), std::move(vertices), context.pixelSizeX(), context.pixelSizeY(),
                        dropdownMesh.x(), dropdownMesh.y(), dropdownMesh.width(), dropdownHeight);
  }
  moveDropdownHover(context, selectedIndex);
}


// -- accessors -- -------------------------------------------------------------

bool ComboBox::isHover(int32_t mouseX, int32_t mouseY) const noexcept {
  return (isListOpen && mouseX >= controlMesh.x())
          ? (mouseY >= y() && mouseY < y() + (int32_t)(controlMesh.height() + dropdownMesh.height())
                           && mouseX < controlMesh.x() + (int32_t)controlMesh.width())
          : (mouseY >= y() && mouseY < y() + (int32_t)controlMesh.height() && mouseX >= x()
                           && mouseX < controlMesh.x() + (int32_t)controlMesh.width());
}

ControlStatus ComboBox::getStatus(int32_t mouseX, int32_t mouseY) const noexcept {
  return isEnabled() ? (isHover(mouseX, mouseY) ? ControlStatus::hover : ControlStatus::regular) : ControlStatus::disabled;
}


// -- operations -- ------------------------------------------------------------

bool ComboBox::click(RendererContext& context, int32_t mouseX) {
  if (isEnabled()) {
    if (isListOpen) {
      if (mouseX >= controlX() && hoverIndex > -1 && selectedIndex != hoverIndex && hoverIndex < (int32_t)selectableValues.size()) {
        selectedIndex = hoverIndex; // change selected index + update selected text
        selectableValues[hoverIndex].nameMesh.cloneAtLocation(context.renderer(), context.pixelSizeX(), context.pixelSizeY(),
                                                              selectedNameMesh.x(), selectedNameMesh.y(), selectedNameMesh);

        if (onChange != nullptr)
          onChange(operationId, (uint32_t)selectableValues[selectedIndex].value); // report value change
      }
      isListOpen = false;
    }
    else {
      hoverIndex = (selectedIndex >= 0) ? selectedIndex : 0;
      moveDropdownHover(context, hoverIndex);
      isListOpen = true;
    }
  }
  return isListOpen;
}

void ComboBox::mouseMove(RendererContext& context, int32_t, int32_t mouseY) {
  if (isListOpen && !selectableValues.empty()) {
    const auto& firstMesh = selectableValues[0].nameMesh;
    if (mouseY >= firstMesh.y()) {
      mouseY -= (firstMesh.y() - (int32_t)comboBoxPaddingY()); // absolute to relative
      mouseY /= static_cast<int32_t>(firstMesh.height() + (comboBoxPaddingY() << 1)); // height to index (divide by entry height)
    }
    else mouseY = selectedIndex; // if mouse on selected name, reset hover position to selected index

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

void ComboBox::close() {
  isListOpen = false;
}


// -- rendering -- -------------------------------------------------------------

void ComboBox::drawDropdown(RendererContext& context, RendererStateBuffers& buffers) {
  if (isListOpen) {
    if (isEnabled()) {
      buffers.bindControlBuffer(context.renderer(), ControlBufferType::regular);
      dropdownMesh.draw(context.renderer());
      if (hoverIndex >= 0) // hover entry background
        dropdownHoverMesh.draw(context.renderer());
    }
    else isListOpen = false;
  }
}

void ComboBox::drawLabels(RendererContext& context, RendererStateBuffers& buffers, bool isActive) {
  LabelBufferType labelBuffer, valueBuffer;
  if (isEnabled()) {
    labelBuffer = isActive ? LabelBufferType::active : LabelBufferType::regular;
    valueBuffer = LabelBufferType::comboBoxValue;
  }
  else {
    labelBuffer = LabelBufferType::disabled;
    valueBuffer = LabelBufferType::comboBoxValueDisabled;
  }

  if (labelMesh.width()) {
    buffers.bindLabelBuffer(context.renderer(), labelBuffer);
    labelMesh.draw(context.renderer());
  }
  buffers.bindLabelBuffer(context.renderer(), valueBuffer);
  selectedNameMesh.draw(context.renderer());
}

void ComboBox::drawOptions(RendererContext& context, RendererStateBuffers& buffers) {
  if (isListOpen) {
    buffers.bindLabelBuffer(context.renderer(), LabelBufferType::dropdownValue);
    const auto* endEntries = &selectableValues[0] + (intptr_t)selectableValues.size();
    for (auto* entry = &selectableValues[0]; entry < endEntries; ++entry)
      entry->nameMesh.draw(context.renderer());
  }
}
