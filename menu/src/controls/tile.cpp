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
#include <display/image_loader.h>
#include "menu/controls/geometry_generator.h"
#include "menu/controls/combo_box_option.h"
#include "menu/controls/tile.h"

using namespace display;
using namespace display::controls;
using namespace menu::controls;
using namespace menu;

ControlType Tile::type() const noexcept { return ControlType::tile; }

#define MAX_NAME_LENGTH ComboBoxOption::maxLength
#define MAX_LINE_LENGTH ((MAX_NAME_LENGTH >> 1) + (MAX_NAME_LENGTH >> 3) + 1u)


// -- init/resize geometry -- --------------------------------------------------

static bool formatLabel(const char16_t* label, char16_t topLine[MAX_NAME_LENGTH],
                                               char16_t bottomLine[MAX_NAME_LENGTH]) {
  size_t labelLength = TextMesh::getStringLength(label);
  if (labelLength > MAX_NAME_LENGTH >> 1) { // more than half max size -> search for separator to split lines
    const char16_t* afterEnd = (labelLength <= MAX_LINE_LENGTH) ? (label + (intptr_t)labelLength) : label + (intptr_t)MAX_LINE_LENGTH;
    const char16_t* beforeRevEnd = label + (intptr_t)(MAX_NAME_LENGTH >> 2);
    const char16_t* searchAfter = &label[MAX_NAME_LENGTH >> 1];
    const char16_t* searchBefore = searchAfter - 1;

    size_t separatorIndex = (size_t)0xFFFFFFF0u;
    while (searchAfter < afterEnd || searchBefore > beforeRevEnd) {
      if (searchAfter < afterEnd) {
        if (*searchAfter == u' ' || *searchAfter == u'-' || *searchAfter == u'_')
          separatorIndex = (searchAfter - label);
        ++searchAfter;
      }
      if (searchBefore > beforeRevEnd) {
        if (*searchBefore == u' ' || *searchBefore == u'-' || *searchBefore == u'_')
          separatorIndex = (searchBefore - label);
        --searchBefore;
      }
    }

    // separator found -or- label too long for display -> split lines
    if (separatorIndex + 1u < labelLength || labelLength > MAX_LINE_LENGTH) {
      if (separatorIndex + 1u >= labelLength) { // not found -> force separator
        separatorIndex = (MAX_NAME_LENGTH >> 1);
        memcpy(topLine, label, (MAX_NAME_LENGTH >> 1)*sizeof(char16_t));
        topLine[(MAX_NAME_LENGTH >> 1)] = u'-';
        topLine[(MAX_NAME_LENGTH >> 1) + 1u] = u'\0';
      }
      else if (label[separatorIndex] != u' ') { // hyphen/underscore
        memcpy(topLine, label, (separatorIndex+1u)*sizeof(char16_t)); // also copy separator
        topLine[(separatorIndex+1u)] = u'\0';
      }
      else { // white-space
        memcpy(topLine, label, separatorIndex * sizeof(char16_t));
        topLine[separatorIndex] = u'\0';
      }
      memcpy(bottomLine, label + (intptr_t)(separatorIndex+1u), (labelLength - separatorIndex)*sizeof(char16_t)); // also copy ending 0
      return true;
    }
  }
  // single line
  if (labelLength)
    memcpy(topLine, label, (labelLength+1u)*sizeof(char16_t)); // also copy ending 0
  else
    *topLine = u'\0';
  return false;
}

void Tile::init(RendererContext& context, const char16_t* label, int32_t x, int32_t labelY,
                uint32_t fixedWidth, uint32_t paddingY, const float backgroundColor[4]) {
  auto& labelFont = context.getFont(FontType::labels);
  const uint32_t height = labelFont.XHeight() + ((labelFont.XHeight() + paddingY) << 1);

  // create label
  char16_t topLine[MAX_NAME_LENGTH];
  char16_t bottomLine[MAX_NAME_LENGTH];
  if (formatLabel(label, topLine, bottomLine)) { // multi-line
    labelTopMesh = TextMesh(context.renderer(), labelFont, topLine, context.pixelSizeX(), context.pixelSizeY(),
                            x + (int32_t)(fixedWidth >> 1), labelY + 1u, TextAlignment::center);
    labelBottomMesh = TextMesh(context.renderer(), labelFont, bottomLine, context.pixelSizeX(), context.pixelSizeY(),
                               x + (int32_t)(fixedWidth >> 1), labelY + 1u + (labelFont.XHeight() << 1), TextAlignment::center);
  }
  else { // single-line
    labelTopMesh = TextMesh(context.renderer(), labelFont, topLine, context.pixelSizeX(), context.pixelSizeY(),
                          x + (int32_t)(fixedWidth >> 1), labelY + 1u + (int32_t)labelFont.XHeight(), TextAlignment::center);
  }
  
  // create background + border
  std::vector<ControlVertex> vertices(static_cast<size_t>(22));
  ControlVertex* vertexIt = vertices.data();

  const float colorDarker[4]{ backgroundColor[0]*0.85f, backgroundColor[1]*0.85f,
                              backgroundColor[2]*0.85f, backgroundColor[3]*0.95f };
  GeometryGenerator::fillControlVertex(*vertexIt,     backgroundColor, 0.f,               0.f); // background (gradient from top)
  GeometryGenerator::fillControlVertex(*(++vertexIt), backgroundColor, (float)fixedWidth, 0.f);
  GeometryGenerator::fillRectangleVertices(++vertexIt, colorDarker, 0.f, (float)fixedWidth, -(float)((height<<1)/3), -(float)height);
  vertexIt += 4;

  constexpr const float borderFactor = 1.3f;
  const float colorBorder[4]{ backgroundColor[0]*borderFactor, backgroundColor[1]*borderFactor,
                              backgroundColor[2]*borderFactor, backgroundColor[3] };
  GeometryGenerator::fillRectangleVertices(vertexIt, colorBorder, 0.f, (float)fixedWidth, 0.f, -1.f); // border top
  vertexIt += 4;
  GeometryGenerator::fillRectangleVertices(vertexIt, colorBorder, 0.f, (float)fixedWidth, // border bottom
                                           -(float)(height-1), -(float)height);
  vertexIt += 4;
  GeometryGenerator::fillRectangleVertices(vertexIt, colorBorder, 0.f, 1.f, -1.f, -(float)(height-1)); // border left
  vertexIt += 4;
  GeometryGenerator::fillRectangleVertices(vertexIt, colorBorder, (float)(fixedWidth-1), (float)fixedWidth, // border right
                                           -1.f, -(float)(height-1));
  
  std::vector<uint32_t> indices = { 0,1,2,2,1,3,       2,3,4,4,3,5,       6,7,8,8,7,9,
                                    10,11,12,12,11,13, 14,15,16,16,15,17, 18,19,20,20,19,21 };
  controlMesh = ControlMesh(context.renderer(), std::move(vertices), indices, context.pixelSizeX(), context.pixelSizeY(),
                            x, labelY - (int32_t)paddingY, fixedWidth, height);
  
  // create delete button
  constexpr const float buttonBorderFactor = borderFactor*1.2f;
  float buttonBorderColor[4]{ backgroundColor[0]*buttonBorderFactor, backgroundColor[1]*buttonBorderFactor, backgroundColor[2]*buttonBorderFactor, 1.f };
  const uint32_t buttonHeight = (height >> 1);
  vertices = std::vector<ControlVertex>(static_cast<size_t>(8 + 8));
  vertexIt = vertices.data();
  GeometryGenerator::fillRectangleVertices(vertexIt, colorBorder, 1.f, (float)(buttonHeight - 1u), -1.f, -(float)buttonHeight); // background
  vertexIt += 4;
  GeometryGenerator::fillRectangleVertices(vertexIt, buttonBorderColor, 0.f, 1.f, -1.f, -(float)buttonHeight); // border left
  vertexIt += 4;
  constexpr const float iconFactor = borderFactor*borderFactor*borderFactor;
  float iconColor[4]{ backgroundColor[0]*iconFactor, backgroundColor[1]*iconFactor, backgroundColor[2]*iconFactor, 1.f };
  const uint32_t iconOffset = (buttonHeight >> 2) + 2u;
  GeometryGenerator::fillCrossVertices(vertexIt, iconColor, (float)(iconOffset + 1u), (float)(buttonHeight - iconOffset + 1u),
                                       -(float)iconOffset, -(float)(buttonHeight - iconOffset));

  indices = { 0,1,2,2,1,3,  4,5,6,6,5,7,  8,9,10,10,9,11,  12,13,14,14,13,15 };
  deleteMesh = ControlMesh(context.renderer(), std::move(vertices), indices, context.pixelSizeX(), context.pixelSizeY(),
                           x + (int32_t)fixedWidth - (int32_t)buttonHeight, controlMesh.y(), buttonHeight - 1u, buttonHeight - 1u);

  // create edit button
  vertices = std::vector<ControlVertex>(static_cast<size_t>(11 + 8));
  vertexIt = vertices.data();
  GeometryGenerator::fillRectangleVertices(vertexIt, colorBorder, 1.f, (float)(buttonHeight - 1u), 0.f, -(float)(buttonHeight - 1u)); // background
  vertexIt += 4;
  GeometryGenerator::fillRectangleVertices(vertexIt, buttonBorderColor, 0.f, 1.f, 0.f, -(float)(buttonHeight - 1u)); // border left
  vertexIt += 4;
  GeometryGenerator::fillPencilVertices(vertexIt, iconColor, (float)(iconOffset - 1u), (float)(buttonHeight - iconOffset + 2u),
                                        -(float)(iconOffset - 2u), -(float)(buttonHeight - iconOffset + 1u));

  indices = { 0,1,2,2,1,3,  4,5,6,6,5,7,  8,9,10,10,9,11, 12,13,14,14,13,15, 16,17,18 };
  editMesh = ControlMesh(context.renderer(), std::move(vertices), indices, context.pixelSizeX(), context.pixelSizeY(),
                         x + (int32_t)fixedWidth - (int32_t)buttonHeight, controlMesh.y() + (int32_t)buttonHeight,
                         buttonHeight - 1u, buttonHeight - 1u);
}

// ---

void Tile::move(RendererContext& context, int32_t x, int32_t labelY, uint32_t fixedWidth) {
  if (labelBottomMesh.width()) { // multi-line
    labelTopMesh.move(context.renderer(), context.pixelSizeX(), context.pixelSizeY(),
                         x + ((int32_t)fixedWidth - (int32_t)labelTopMesh.width())/2, labelY + 1u);
    labelBottomMesh.move(context.renderer(), context.pixelSizeX(), context.pixelSizeY(),
                         x + ((int32_t)fixedWidth - (int32_t)labelBottomMesh.width())/2, labelY + 1u + (int32_t)(labelTopMesh.height() << 1));
  }
  else // single-line
    labelTopMesh.move(context.renderer(), context.pixelSizeX(), context.pixelSizeY(),
                      x + ((int32_t)fixedWidth - (int32_t)labelTopMesh.width())/2, labelY + 1u + (int32_t)labelTopMesh.height());
  
  const uint32_t paddingY = (controlMesh.height() - labelTopMesh.height()*3u) >> 1;
  if (fixedWidth != controlMesh.width()) {
    std::vector<ControlVertex> vertices = controlMesh.relativeVertices();
    ControlVertex* vertexIt = vertices.data() + 1;
    vertexIt->position[0] = (float)fixedWidth;
    GeometryGenerator::resizeRectangleVerticesX(++vertexIt, (float)fixedWidth);
    vertexIt += 4;
    GeometryGenerator::resizeRectangleVerticesX(vertexIt, (float)fixedWidth);
    vertexIt += 4;
    GeometryGenerator::resizeRectangleVerticesX(vertexIt, (float)fixedWidth);
    vertexIt += 8;
    GeometryGenerator::moveRectangleVerticesX(vertexIt, (float)(fixedWidth - 1), (float)fixedWidth);

    controlMesh.update(context.renderer(), std::move(vertices), context.pixelSizeX(), context.pixelSizeY(),
                       x, labelY - (int32_t)paddingY, fixedWidth, controlMesh.height());
  }
  else
    controlMesh.move(context.renderer(), context.pixelSizeX(), context.pixelSizeY(), x, labelY - (int32_t)paddingY);

  const uint32_t buttonHeight = (controlMesh.height() >> 1);
  deleteMesh.move(context.renderer(), context.pixelSizeX(), context.pixelSizeY(),
                  x + (int32_t)fixedWidth - (int32_t)buttonHeight, controlMesh.y());
  editMesh.move(context.renderer(), context.pixelSizeX(), context.pixelSizeY(),
                x + (int32_t)fixedWidth - (int32_t)buttonHeight, controlMesh.y() + (int32_t)buttonHeight);
}


// -- accessors/operations -- --------------------------------------------------

ControlStatus Tile::getStatus(int32_t mouseX, int32_t mouseY) const noexcept {
  return isHover(mouseX, mouseY) ? ControlStatus::hover : ControlStatus::regular;
}

bool Tile::click(RendererContext&, int32_t mouseX, int32_t mouseY) {
  if (onChange != nullptr) {
    if (mouseX >= deleteMesh.x()) {
      if (mouseY < editMesh.y()) // delete hover
        onChange(tileId, TileAction::remove);
      else // edit hover
        onChange(tileId, TileAction::edit);
    }
    else
      onChange(tileId, TileAction::select);
  }
  return false;
}


// -- rendering -- -------------------------------------------------------------

void Tile::drawBackground(RendererContext& context, int32_t mouseX, int32_t mouseY,
                          RendererStateBuffers& buffers, bool isSelected, bool isActive) {
  buffers.bindControlBuffer(context.renderer(), isActive ? ControlBufferType::active
                                                         : (isSelected ? ControlBufferType::selectedTile : ControlBufferType::regular));
  controlMesh.draw(context.renderer());
  
  if (isActive) {
    ControlBufferType deleteType = ControlBufferType::regular;
    ControlBufferType editType = ControlBufferType::regular;
    if (mouseX >= deleteMesh.x()) {
      if (mouseY < editMesh.y()) // delete hover
        deleteType =  ControlBufferType::active;
      else // edit hover
        editType = ControlBufferType::active;
    }
    buffers.bindControlBuffer(context.renderer(), deleteType);
    deleteMesh.draw(context.renderer());
    buffers.bindControlBuffer(context.renderer(), editType);
    editMesh.draw(context.renderer());
  }
}

void Tile::drawLabel(RendererContext& context, RendererStateBuffers& buffers, bool isSelected, bool isActive) {
  buffers.bindLabelBuffer(context.renderer(), isSelected ? LabelBufferType::selectedTile
                                                         : (isActive ? LabelBufferType::activeTile : LabelBufferType::tile));
  labelTopMesh.draw(context.renderer());
  if (labelBottomMesh.width())
    labelBottomMesh.draw(context.renderer());
}
