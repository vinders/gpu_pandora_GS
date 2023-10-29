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
#include "menu/controls/tooltip.h"

using namespace display;
using namespace display::controls;
using namespace menu::controls;
using namespace menu;


// -- init/resize geometry -- --------------------------------------------------

void Tooltip::init(RendererContext& context, const char32_t* label, int32_t x, int32_t y,
                   uint32_t width, uint32_t height_, const float backgroundColor[4], display::ControlIconType icon) {
  // try to load icon (if available)
  uint32_t iconWidthWithMargin = 0;
  ControlIcon iconData;
  if (icon != display::ControlIconType::none) {
    iconData = context.imageLoader().getIcon(icon);

    const bool isLabelEmpty = (label == nullptr || *label == (char32_t)0);
    if (iconData.texture() != nullptr)
      iconWidthWithMargin = !isLabelEmpty ? iconData.width() + iconMarginRight() : iconData.width();
  }
  
  // create label
  auto& labelFont = context.getFont(fontType);
  const int32_t labelX = x + (int32_t)paddingX + (int32_t)iconWidthWithMargin;
  const uint32_t labelY = y + (int32_t)((height_ - labelFont.XHeight() - 1) >> 1);
  labelMesh = TextMesh(context.renderer(), labelFont, label, context.pixelSizeX(), context.pixelSizeY(),
                       labelX, labelY, TextAlignment::left);

  // create background
  const float colorDarker[4]{ backgroundColor[0]*0.85f, backgroundColor[1]*0.85f,
                              backgroundColor[2]*0.85f, backgroundColor[3] };
  std::vector<ControlVertex> vertices(static_cast<size_t>(4));
  GeometryGenerator::fillVerticalRectangleVertices(vertices.data(), backgroundColor, colorDarker,
                                                   0.f, (float)width, 0.f, -(float)height_);
  std::vector<uint32_t> indices{ 0,1,2, 2,1,3 };
  controlMesh = ControlMesh(context.renderer(), std::move(vertices), indices,
                            context.pixelSizeX(), context.pixelSizeY(), x, y, width, height_);

  // create icon (optional)
  if (iconData.texture() != nullptr) {
    const int32_t iconX = x + (int32_t)paddingX;
    const int32_t iconY = y + ((int32_t)height_ - (int32_t)iconData.height())/2;
    iconMesh = IconMesh(context.renderer(), std::move(iconData.texture()), context.pixelSizeX(), context.pixelSizeY(),
                        iconX, iconY, iconData.offsetX(), iconData.offsetY(), iconData.width(), iconData.height());
  }
}

// ---

void Tooltip::move(RendererContext& context, int32_t x, int32_t y, uint32_t width) {
  uint32_t iconWidthWithMargin = 0;
  if (iconMesh.width())
    iconWidthWithMargin = labelMesh.width() ? iconMesh.width() + iconMarginRight() : iconMesh.width();

  const int32_t labelX = x + (int32_t)paddingX + (int32_t)iconWidthWithMargin;
  const uint32_t labelY = y + (int32_t)((controlMesh.height() - labelMesh.height() - 1) >> 1);
  labelMesh.move(context.renderer(), context.pixelSizeX(), context.pixelSizeY(), labelX, labelY);

  std::vector<ControlVertex> vertices = controlMesh.relativeVertices();
  GeometryGenerator::resizeRectangleVerticesX(vertices.data(), (float)width);
  controlMesh.update(context.renderer(), std::move(vertices), context.pixelSizeX(), context.pixelSizeY(),
                     x, y, width, controlMesh.height());

  if (iconMesh.width()) {
    const int32_t iconX = x + (int32_t)paddingX;
    const int32_t iconY = y + ((int32_t)controlMesh.height() - (int32_t)iconMesh.height())/2;
    iconMesh.move(context.renderer(), context.pixelSizeX(), context.pixelSizeY(), iconX, iconY);
  }
}


// -- operations -- ------------------------------------------------------------

void Tooltip::updateIcon(RendererContext& context, display::ControlIconType icon) {
  // try to load icon (if available)
  uint32_t iconWidthWithMargin = 0;
  ControlIcon iconData;
  if (icon != display::ControlIconType::none) {
    iconData = context.imageLoader().getIcon(icon);
    if (iconData.texture() != nullptr)
      iconWidthWithMargin = labelMesh.width() ? iconData.width() + iconMarginRight() : iconData.width();
  }
  // replace icon
  if (iconData.texture() != nullptr) {
    const int32_t iconX = controlMesh.x() + (int32_t)paddingX;
    const int32_t iconY = controlMesh.y() + ((int32_t)controlMesh.height() - (int32_t)iconData.height())/2;
    iconMesh = IconMesh(context.renderer(), std::move(iconData.texture()), context.pixelSizeX(), context.pixelSizeY(),
                        iconX, iconY, iconData.offsetX(), iconData.offsetY(), iconData.width(), iconData.height());

    if (labelMesh.x() == iconMesh.x()) { // adjust label location
      labelMesh.move(context.renderer(), context.pixelSizeX(), context.pixelSizeY(),
                     iconMesh.x() + (int32_t)iconWidthWithMargin, labelMesh.y());
    }
  }
  else { // no icon
    iconMesh = IconMesh{};
    
    if (labelMesh.x() > controlMesh.x() + (int32_t)paddingX) { // adjust label location
      labelMesh.move(context.renderer(), context.pixelSizeX(), context.pixelSizeY(),
                     controlMesh.x() + (int32_t)paddingX, labelMesh.y());
    }
  }
}

void Tooltip::updateLabel(RendererContext& context, const char32_t* label, LabelBufferType textColor_) {
  this->textColor = textColor_;
  labelMesh = TextMesh(context.renderer(), context.getFont(fontType), label, context.pixelSizeX(), context.pixelSizeY(),
                       labelMesh.x(), labelMesh.y(), TextAlignment::left);
}


// -- rendering -- -------------------------------------------------------------

void Tooltip::drawBackground(RendererContext& context, RendererStateBuffers& buffers) {
  buffers.bindControlBuffer(context.renderer(), ControlBufferType::regular);
  controlMesh.draw(context.renderer());
}

void Tooltip::drawIcon(RendererContext& context, RendererStateBuffers& buffers) {
  if (iconMesh.width()) {
    buffers.bindIconBuffer(context.renderer(), ControlBufferType::regular);
    iconMesh.draw(context.renderer());
  }
}

void Tooltip::drawLabel(RendererContext& context, RendererStateBuffers& buffers) {
  if (labelMesh.width()) {
    buffers.bindLabelBuffer(context.renderer(), textColor);
    labelMesh.draw(context.renderer());
  }
}
