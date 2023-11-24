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
#include "menu/controls/button.h"

using namespace display;
using namespace display::controls;
using namespace menu::controls;
using namespace menu;

ControlType Button::type() const noexcept { return ControlType::button; }


// -- init/resize geometry -- --------------------------------------------------

void Button::init(RendererContext& context, const char16_t* label, int32_t x, int32_t labelY, const ButtonStyleProperties& style) {
  // try to load icon (if available)
  uint32_t iconWidthWithMargin = 0;
  ControlIcon iconData;
  if (style.icon != display::ControlIconType::none) {
    iconData = context.imageLoader().getIcon(style.icon);

    const bool isLabelEmpty = (label == nullptr || *label == (char16_t)0);
    if (iconData.texture() != nullptr)
      iconWidthWithMargin = !isLabelEmpty ? iconData.width() + iconMarginRight() : iconData.width();
    else if (isLabelEmpty) // icon not available + empty label -> set placeholder
      label = toDefaultLabel(style.icon);
  }
  
  // create label
  const int32_t labelX = x + (int32_t)style.paddingX + (int32_t)iconWidthWithMargin;
  labelMesh = TextMesh(context.renderer(), context.getFont(style.fontType), label,
                       context.pixelSizeX(), context.pixelSizeY(), labelX, labelY + 1, TextAlignment::left);

  // compute control size (based on label)
  uint32_t width = (style.paddingX << 1) + labelMesh.width() + iconWidthWithMargin;
  if (width < style.minButtonWidth)
    width = style.minButtonWidth;
  const uint32_t height = (style.paddingY << 1) + labelMesh.height();

  // create background
  const float cornerSize = (float)style.paddingY;
  std::vector<ControlVertex> vertices;
  std::vector<uint32_t> indices;
  if (style.borderSize) {
    vertices.resize(static_cast<size_t>(6 + 6));
    if (style.style == ButtonStyle::fromBottomLeft) {
      GeometryGenerator::fillTLBRCutRectangleVertices(vertices.data(), style.borderColor, // button borders
                                                      0.f, (float)width, 0.f, -(float)height, cornerSize);
      GeometryGenerator::fillTLBRCutRectangleVertices(vertices.data() + 6, style.backgroundColor, // button background
                                                      (float)style.borderSize, (float)(width - style.borderSize),
                                                      -(float)style.borderSize, -(float)(height - style.borderSize), cornerSize);
    }
    else if (style.style == ButtonStyle::fromTopLeft) {
      GeometryGenerator::fillBLTRCutRectangleVertices(vertices.data(), style.borderColor, // button borders
                                                      0.f, (float)width, 0.f, -(float)height, cornerSize);
      GeometryGenerator::fillBLTRCutRectangleVertices(vertices.data() + 6, style.backgroundColor, // button background
                                                      (float)style.borderSize, (float)(width - style.borderSize),
                                                      -(float)style.borderSize, -(float)(height - style.borderSize), cornerSize);
    }
    indices = { 0,1,2,2,1,3,  2,3,4,4,3,5,  6,7,8,8,7,9,  8,9,10,10,9,11 };
  }
  else {
    vertices.resize(static_cast<size_t>(6));
    if (style.style == ButtonStyle::fromBottomLeft)
      GeometryGenerator::fillTLBRCutRectangleVertices(vertices.data(), style.backgroundColor, // button background
                                                      0.f, (float)width, 0.f, -(float)height, cornerSize);
    else if (style.style == ButtonStyle::fromTopLeft)
      GeometryGenerator::fillBLTRCutRectangleVertices(vertices.data(), style.backgroundColor, // button background
                                                      0.f, (float)width, 0.f, -(float)height, cornerSize);
    indices = { 0,1,2,2,1,3,  2,3,4,4,3,5 };
  }
  controlMesh = ControlMesh(context.renderer(), std::move(vertices), indices, context.pixelSizeX(),
                            context.pixelSizeY(), x, labelY - (int32_t)style.paddingY, width, height);

  // create icon (optional)
  if (iconData.texture() != nullptr) {
    const int32_t iconX = labelMesh.x() - (int32_t)iconWidthWithMargin;
    const int32_t iconY = controlMesh.y() + ((int32_t)height - (int32_t)iconData.height())/2;
    iconMesh = IconMesh(context.renderer(), std::move(iconData.texture()), context.pixelSizeX(), context.pixelSizeY(),
                        iconX, iconY, iconData.offsetX(), iconData.offsetY(), iconData.width(), iconData.height());
  }
}

// ---

void Button::move(RendererContext& context, int32_t x, int32_t labelY) {
  uint32_t iconWidthWithMargin = 0;
  if (iconMesh.width())
    iconWidthWithMargin = labelMesh.width() ? iconMesh.width() + iconMarginRight() : iconMesh.width();

  const int32_t labelX = x + (int32_t)paddingX + (int32_t)iconWidthWithMargin;
  labelMesh.move(context.renderer(), context.pixelSizeX(), context.pixelSizeY(), labelX, labelY + 1);

  controlMesh.move(context.renderer(), context.pixelSizeX(), context.pixelSizeY(),
                   labelMesh.x() - paddingX - iconWidthWithMargin, labelY - (int32_t)paddingY);

  if (iconMesh.width()) {
    const int32_t iconX = labelMesh.x() - (int32_t)iconWidthWithMargin;
    const int32_t iconY = controlMesh.y() + ((int32_t)controlMesh.height() - (int32_t)iconMesh.height())/2;
    iconMesh.move(context.renderer(), context.pixelSizeX(), context.pixelSizeY(), iconX, iconY);
  }
}

void Button::move(RendererContext& context, uint32_t width, const float backgroundColor[4]) {
  std::vector<ControlVertex> vertices = controlMesh.relativeVertices();
  if (vertices.size() > 6) { // with borders
    const uint32_t borderSize = static_cast<uint32_t>(vertices[6].position[0] - vertices[0].position[0] + 0.5f);
    if (vertices[0].position[1] == vertices[1].position[1]) {
      GeometryGenerator::resizeBLTRCutRectangleVerticesX(vertices.data(), (float)width);
      GeometryGenerator::resizeBLTRCutRectangleVerticesX(vertices.data() + 6, (float)(width - borderSize));
    }
    else {
      GeometryGenerator::resizeTLBRCutRectangleVerticesX(vertices.data(), (float)width);
      GeometryGenerator::resizeTLBRCutRectangleVerticesX(vertices.data() + 6, (float)(width - borderSize));
    }
    for (uint32_t i = 6; i < (uint32_t)vertices.size(); ++i)
      memcpy(vertices[i].color, backgroundColor, sizeof(float)*4u);
  }
  else {
    if (vertices[0].position[1] == vertices[1].position[1])
      GeometryGenerator::resizeBLTRCutRectangleVerticesX(vertices.data(), (float)width);
    else
      GeometryGenerator::resizeTLBRCutRectangleVerticesX(vertices.data(), (float)width);
    for (auto& vertex : vertices)
      memcpy(vertex.color, backgroundColor, sizeof(float)*4u);
  }
  controlMesh.update(context.renderer(), std::move(vertices), context.pixelSizeX(), context.pixelSizeY(),
                     controlMesh.x(), controlMesh.y(), width, controlMesh.height());
}


// -- accessors/operations -- --------------------------------------------------

ControlStatus Button::getStatus(int32_t mouseX, int32_t mouseY) const noexcept {
  return isEnabled() ? (isHover(mouseX, mouseY) ? ControlStatus::hover : ControlStatus::regular) : ControlStatus::disabled;
}

bool Button::click(RendererContext&, int32_t, int32_t) {
  click();
  return false;
}


// -- rendering -- -------------------------------------------------------------

void Button::drawBackground(RendererContext& context, RendererStateBuffers& buffers, bool isActive, bool isMouseDown) {
  buffers.bindControlBuffer(context.renderer(), isEnabled() ? ((isActive && !isMouseDown) ? ControlBufferType::active : ControlBufferType::regular)
                                                            : ControlBufferType::disabled);
  controlMesh.draw(context.renderer());
}

void Button::drawPopupBackground(RendererContext& context, RendererStateBuffers& buffers, bool isActive, bool isSelected) {
  buffers.bindControlBuffer(context.renderer(), isEnabled() ? (isActive
                                                               ? ControlBufferType::active
                                                               : (isSelected ? ControlBufferType::activeScroll : ControlBufferType::regular) )
                                                            : ControlBufferType::disabled);
  controlMesh.draw(context.renderer());
}

void Button::drawIcon(RendererContext& context, RendererStateBuffers& buffers, bool isActive) {
  if (iconMesh.width()) {
    buffers.bindIconBuffer(context.renderer(), isEnabled() ? (isActive ? ControlBufferType::active : ControlBufferType::regular)
                                                            : ControlBufferType::disabled);
    iconMesh.draw(context.renderer());
  }
}

void Button::drawLabel(RendererContext& context, RendererStateBuffers& buffers, bool isActive) {
  if (labelMesh.width()) {
    buffers.bindLabelBuffer(context.renderer(), isEnabled() ? (isActive ? LabelBufferType::buttonActive : LabelBufferType::button)
                                                            : LabelBufferType::buttonDisabled);
    labelMesh.draw(context.renderer());
  }
}
