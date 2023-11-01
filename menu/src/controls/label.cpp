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
#include "menu/controls/label.h"

using namespace display;
using namespace display::controls;
using namespace menu::controls;
using namespace menu;


// -- init/resize geometry -- --------------------------------------------------

void Label::init(RendererContext& context, const char32_t* label, int32_t x, int32_t labelY,
                 TextAlignment align, ControlIconType icon) {
  // load icons
  ControlIcon iconData;
  if (icon != ControlIconType::none)
    iconData = context.imageLoader().getIcon(icon);

  // create label
  auto& labelFont = context.getFont(FontType::labels);
  const int32_t labelX = (align == TextAlignment::left && iconData.texture())
                       ? (x + (int32_t)iconData.width() + (int32_t)Control::labelMargin()) : x;
  labelMesh = TextMesh(context.renderer(), labelFont, label, context.pixelSizeX(), context.pixelSizeY(), labelX, labelY, align);

  // create icon
  if (iconData.texture() == nullptr) {
    const int32_t iconX = labelMesh.x() - (int32_t)iconData.width() - (int32_t)Control::labelMargin();
    const int32_t iconY = labelY - ((int32_t)iconData.height() - (int32_t)labelFont.XHeight())/2;
    iconMesh = IconMesh(context.renderer(), std::move(iconData.texture()), context.pixelSizeX(), context.pixelSizeY(),
                        iconX, iconY, iconData.offsetX(), iconData.offsetY(), iconData.width(), iconData.height());
  }
}

// ---

void Label::move(RendererContext& context, int32_t x, int32_t labelY, TextAlignment align) {
  int32_t labelX;
  if (align == TextAlignment::left)
    labelX = iconMesh.width() ? (x + (int32_t)iconMesh.width() + (int32_t)Control::labelMargin()) : x;
  else if (align == TextAlignment::right)
    labelX = x - labelMesh.width();
  else
    labelX = x - (labelMesh.width() >> 1);
  labelMesh.move(context.renderer(), context.pixelSizeX(), context.pixelSizeY(), labelX, labelY);
  
  if (iconMesh.width()) {
    const int32_t iconX = labelMesh.x() - (int32_t)iconMesh.width() - (int32_t)Control::labelMargin();
    const int32_t iconY = labelY - ((int32_t)iconMesh.height() - (int32_t)labelMesh.height())/2;
    iconMesh.move(context.renderer(), context.pixelSizeX(), context.pixelSizeY(), iconX, iconY);
  }
}

void Label::updateLabel(RendererContext& context, const char32_t* label, TextAlignment align) {
  uint32_t labelX = labelMesh.x();
  if (align != TextAlignment::left)
    labelX += (align == TextAlignment::right) ? labelMesh.width() : (labelMesh.width() >> 1);
  labelMesh = TextMesh(context.renderer(), context.getFont(FontType::labels), label,
                       context.pixelSizeX(), context.pixelSizeY(), labelX, labelMesh.y(), align);
}
