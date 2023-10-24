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
#include <cassert>
#include <vector>
#include <unordered_map>
#include <video/window_keycodes.h>
#include "menu/controls/check_box.h"
#include "menu/controls/combo_box.h"
#include "menu/controls/text_box.h"
#include "menu/controls/ruler.h"
#include "menu/controls/slider.h"
#include "menu/page.h"

using namespace video_api;
using namespace display;
using namespace menu::controls;
using namespace menu;


bool ControlRegistration::isEnabled() const noexcept {
  assert(target != nullptr);
  switch (target->Type()) {
    case ControlType::checkBox: return reinterpret_cast<const CheckBox*>(target)->isEnabled();
    case ControlType::comboBox: return reinterpret_cast<const ComboBox*>(target)->isEnabled();
    case ControlType::textBox: return reinterpret_cast<const TextBox*>(target)->isEnabled();
    case ControlType::ruler: return reinterpret_cast<const Ruler*>(target)->isEnabled();
    case ControlType::slider: return reinterpret_cast<const Slider*>(target)->isEnabled();
    default: return false;
  };
}



Page::Page(std::shared_ptr<RendererContext> context, int32_t x, int32_t y,
           uint32_t width, uint32_t visibleHeight, uint32_t totalPageHeight) {
  
}

void Page::move(int32_t x, int32_t y, uint32_t width, uint32_t visibleHeight, uint32_t totalPageHeight) {
  
}

void Page::mouseDown(int32_t mouseX, int32_t mouseY) {
  
}
void Page::mouseMove(int32_t mouseX, int32_t mouseY) {
  
}
void Page::mouseUp(int32_t mouseX, int32_t mouseY) {
  
}
void Page::mouseScroll(int32_t deltaY) {
  
}
