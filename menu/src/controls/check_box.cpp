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
#include "menu/controls/check_box.h"

using namespace display;
using namespace display::controls;
using namespace menu::controls;
using namespace menu;

ControlType CheckBox::type() const noexcept { return ControlType::checkBox; }


// -- init/resize geometry -- --------------------------------------------------

void CheckBox::init(RendererContext& context, const char16_t* label, int32_t x, int32_t labelY) {
  // load icons
  ControlIcon iconDataOn = context.imageLoader().getIcon(ControlIconType::checked);
  ControlIcon iconDataOff = context.imageLoader().getIcon(ControlIconType::unchecked);
  if (iconDataOn.texture() == nullptr) // if not available, generate visual indicator (to replace icons)
    iconDataOn = context.imageLoader().generateSquareIcon(true);
  if (iconDataOff.texture() == nullptr)
    iconDataOff = context.imageLoader().generateSquareIcon(false);

  // create label
  auto& labelFont = context.getFont(FontType::labels);
  labelMesh = TextMesh(context.renderer(), labelFont, label, context.pixelSizeX(), context.pixelSizeY(), x, labelY);
  uint32_t labelWidth = (minLabelWidth >= labelMesh.width()) ? minLabelWidth : labelMesh.width();
  if (labelWidth)
    labelWidth += labelMargin();

  // create icons
  const int32_t boxX = x + (int32_t)labelWidth;
  const int32_t boxY = labelY - ((int32_t)iconDataOn.height() - (int32_t)labelFont.XHeight())/2 - 1;
  checkedMesh = IconMesh(context.renderer(), std::move(iconDataOn.texture()), context.pixelSizeX(), context.pixelSizeY(),
                         boxX, boxY, iconDataOn.offsetX(), iconDataOn.offsetY(), iconDataOn.width(), iconDataOn.height());
  uncheckedMesh = IconMesh(context.renderer(), std::move(iconDataOff.texture()), context.pixelSizeX(), context.pixelSizeY(),
                           boxX, boxY, iconDataOff.offsetX(), iconDataOff.offsetY(), iconDataOff.width(), iconDataOff.height());
}

// ---

void CheckBox::move(RendererContext& context, int32_t x, int32_t labelY) {
  labelMesh.move(context.renderer(), context.pixelSizeX(), context.pixelSizeY(), x, labelY);
  uint32_t labelWidth = (minLabelWidth >= labelMesh.width()) ? minLabelWidth : labelMesh.width();
  if (labelWidth)
    labelWidth += labelMargin();

  const int32_t boxX = x + (int32_t)labelWidth;
  const int32_t boxY = labelY - ((int32_t)checkedMesh.height() - (int32_t)labelMesh.height())/2 - 1;
  checkedMesh.move(context.renderer(), context.pixelSizeX(), context.pixelSizeY(), boxX, boxY);
  uncheckedMesh.move(context.renderer(), context.pixelSizeX(), context.pixelSizeY(), boxX, boxY);
}


// -- accessors/operations -- --------------------------------------------------

ControlStatus CheckBox::getStatus(int32_t mouseX, int32_t mouseY) const noexcept {
  return isEnabled() ? (isHover(mouseX, mouseY) ? ControlStatus::hover : ControlStatus::regular) : ControlStatus::disabled;
}

bool CheckBox::click(RendererContext&, int32_t, int32_t) {
  click();
  return false;
}


// -- rendering -- -------------------------------------------------------------

void CheckBox::drawIcon(RendererContext& context, RendererStateBuffers& buffers, bool isActive) {
  buffers.bindIconBuffer(context.renderer(), isEnabled()
                                             ? (isActive ? ControlBufferType::activeIcon : ControlBufferType::coloredIcon)
                                             : ControlBufferType::disabledIcon);
  if (*boundValue)
    checkedMesh.draw(context.renderer());
  else
    uncheckedMesh.draw(context.renderer());
}

void CheckBox::drawLabel(RendererContext& context, RendererStateBuffers& buffers, bool isActive) {
  if (labelMesh.width()) {
    buffers.bindLabelBuffer(context.renderer(), isEnabled()
                                                ? (isActive ? LabelBufferType::active : LabelBufferType::regular)
                                                : LabelBufferType::disabled);
    labelMesh.draw(context.renderer());
  }
}
