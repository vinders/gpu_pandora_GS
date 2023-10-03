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
#include <display/image_loader.h>
#include "menu/controls/button.h"

using namespace display;
using namespace display::controls;
using namespace menu::controls;


static inline void setVertexPosition(float* position, float x, float y) {
  *position = x;
  *(++position) = y;
  *(++position) = 0.f; // z
  *(++position) = 1.f; // w
}

// ---

void Button::init(RendererContext& context, const char32_t* label, const ControlStyle& style, int32_t x, int32_t y) {
  // try to load icon (if available)
  uint32_t iconWidthWithMargin = 0;
  ControlIcon iconData;
  if (style.icon != display::ControlIconType::none) {
    iconData = context.imageLoader.getIcon(style.icon);

    const bool isLabelEmpty = (label == nullptr || *label == (char32_t)0);
    if (iconData.texture() != nullptr)
      iconWidthWithMargin = !isLabelEmpty ? iconData.width() + iconMarginRight() : iconData.width();
    else if (isLabelEmpty) // icon not available + empty label -> set placeholder
      label = toDefaultLabel(style.icon);
  }
  
  // create label
  const int32_t labelX = x + (int32_t)style.paddingX + (int32_t)iconWidthWithMargin;
  labelMesh = TextMesh(*context.renderer, context.getFont(style.fontType), label,
                       context.pixelSizeX, context.pixelSizeY, labelX, y + (int32_t)style.paddingY);

  // create background
  const uint32_t width = (style.paddingX << 1) + labelMesh.width() + iconWidthWithMargin;
  const uint32_t height = (style.paddingY << 1) + labelMesh.height();
  std::vector<ControlVertex> vertices;
  vertices.resize(5);

  ControlVertex* vertexIt = vertices.data();
  setVertexPosition(vertexIt->position,     0.f,          0.f);
  setVertexPosition((++vertexIt)->position, static_cast<float>(width - (style.paddingY << 1)), 0.f);
  setVertexPosition((++vertexIt)->position, 0.f,          -(float)height);
  setVertexPosition((++vertexIt)->position, (float)width, -(float)style.paddingY);
  setVertexPosition((++vertexIt)->position, (float)width, -(float)height);
  std::vector<uint32_t> indices{ 0,1,2, 1,3,2, 2,3,4 };

  controlMesh = ControlMesh(*context.renderer, std::move(vertices), indices, context.pixelSizeX, context.pixelSizeY,
                            labelMesh.x() - style.paddingX - iconWidthWithMargin, y, width, height);

  // create icon (optional)
  if (iconData.texture() != nullptr) {
    const int32_t iconX = labelMesh.x() - (int32_t)iconWidthWithMargin;
    const int32_t iconY = y + ((int32_t)height - (int32_t)iconData.height())/2;
    iconMesh = IconMesh(*context.renderer, std::move(iconData.texture()), context.pixelSizeX, context.pixelSizeY,
                        iconX, iconY, iconData.offsetX(), iconData.offsetY(), iconData.width(), iconData.height());
  }
}

// ---

void Button::move(RendererContext& context, int32_t x, int32_t y) {
  uint32_t iconWidthWithMargin = 0;
  if (iconMesh.width())
    iconWidthWithMargin = labelMesh.width() ? iconMesh.width() + iconMarginRight() : iconMesh.width();

  const int32_t labelX = x + (int32_t)paddingX + (int32_t)iconWidthWithMargin;
  labelMesh.move(*context.renderer, context.pixelSizeX, context.pixelSizeY, labelX, y + (int32_t)paddingY);

  controlMesh.move(*context.renderer, context.pixelSizeX, context.pixelSizeY,
                   labelMesh.x() - paddingX - iconWidthWithMargin, y);

  if (iconMesh.width()) {
    const int32_t iconX = labelMesh.x() - (int32_t)iconWidthWithMargin;
    const int32_t iconY = y + ((int32_t)controlMesh.height() - (int32_t)iconMesh.height())/2;
    iconMesh.move(*context.renderer, context.pixelSizeX, context.pixelSizeY, iconX, iconY);
  }
}