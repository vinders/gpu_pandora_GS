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
#include <cstring>
#include <video/window_keycodes.h>
#include "menu/controls/control.h"
#include "menu/controls/combo_box.h"
#include "menu/controls/text_box.h"
#include "menu/controls/ruler.h"
#include "menu/controls/slider.h"
#include "menu/controls/key_binding.h"
#include "menu/controls/tile.h"
#include "menu/controls/popup.h"
#include "menu/controls/geometry_generator.h"
#include "menu/pages/page.h"

using namespace video_api;
using namespace pandora::video;
using namespace display::controls;
using namespace menu::controls;
using namespace menu::pages;
using namespace menu;


#define HOVER_BORDER_RADIUS 3.f

Page::Page(std::shared_ptr<RendererContext> context_, std::shared_ptr<RendererStateBuffers> buffers_,
           const ColorTheme& theme, int32_t x, int32_t y, uint32_t width, uint32_t visibleHeight,
           bool enableTooltip, bool enableHoverMesh, uint32_t bottomBarHeight)
  : context(std::move(context_)),
    buffers(std::move(buffers_)),
    scrollY(0),
    bottomBarHeight(bottomBarHeight),
    x_(x),
    width_(width),
    activeControlIndex(noControlSelection()) {
  assert(this->context != nullptr && this->buffers != nullptr);

  // create page scrollbar
  const int32_t scrollBarX = x + (int32_t)width - (int32_t)Control::scrollbarWidth();
  auto scrollHandler = std::bind(&Page::onScroll, this, std::placeholders::_1);
  buffers->updateScrollBuffer(context->pixelSizeY(), 0);
  scrollbar = ScrollBar(*context, theme.scrollbarControlColor(), theme.scrollbarThumbColor(),
                        scrollBarX, y, Control::scrollbarWidth(), std::move(scrollHandler),
                        visibleHeight, visibleHeight, (Control::pageLineHeight() >> 1));

  // create tooltip bar
  if (enableTooltip) {
    tooltip = Tooltip(*context, u" ", FontType::inputText, LabelBufferType::regular,
                      0, y + (int32_t)visibleHeight - (int32_t)Control::tooltipBarHeight(),
                      context->clientWidth(), Control::tooltipBarHeight(), theme.tooltipControlColor(), display::ControlIconType::none);
    if (tooltip.height() > this->bottomBarHeight)
      this->bottomBarHeight = tooltip.height();
  }

  // create control line hover area
  const int32_t controlHoverX = x + (int32_t)Control::fieldsetMarginX(width)
                              + (int32_t)Control::fieldsetContentMarginX(width) - Control::lineHoverPaddingX();
  constexpr const uint32_t controlHoverWidth = Control::pageLabelWidth() + Control::pageControlWidth()
                                             + (Control::lineHoverPaddingX() << 1) + Control::labelMargin();
  std::vector<ControlVertex> controlHoverVertices(GeometryGenerator::getRoundedRectangleVertexCount(HOVER_BORDER_RADIUS));
  GeometryGenerator::fillRoundedRectangleVertices(controlHoverVertices.data(), theme.lineSelectorControlColor(),
                                                  0.f, (float)controlHoverWidth, 0.f, -(float)Control::pageLineHeight(),
                                                  HOVER_BORDER_RADIUS);
  std::vector<uint32_t> indices(GeometryGenerator::getRoundedRectangleVertexIndexCount(HOVER_BORDER_RADIUS));
  GeometryGenerator::fillRoundedRectangleIndices(indices.data(), 0, HOVER_BORDER_RADIUS);

  if (enableHoverMesh)
    controlHoverMesh = ControlMesh(context->renderer(), std::move(controlHoverVertices), indices, context->pixelSizeX(),
                                   context->pixelSizeY(), controlHoverX, 0, controlHoverWidth, Control::pageLineHeight());
}

Page::~Page() noexcept {
  // release controls before context
  scrollbar.release();
  tooltip.release();
  controlHoverMesh.release();
  controlRegistry.clear();

  buffers = nullptr;
  context = nullptr;
}

// ---

void Page::moveBase(int32_t x, int32_t y, uint32_t width, uint32_t visibleHeight) {
  const int32_t scrollBarX = x + (int32_t)width - (int32_t)scrollbar.width();
  scrollbar.moveControl(*context, scrollBarX, y, visibleHeight);
  this->x_ = x;
  this->width_ = width;

  if (tooltip.width()) {
    tooltip.move(*context, 0, y + (int32_t)visibleHeight - (int32_t)Control::tooltipBarHeight(), context->clientWidth());
    if (activeControlIndex != noControlSelection())
      tooltip.updateLabel(*context, u" ", LabelBufferType::regular);
  }

  if (controlHoverMesh.width()) {
    const int32_t controlHoverX = x + (int32_t)Control::fieldsetMarginX(width)
                                + (int32_t)Control::fieldsetContentMarginX(width) - Control::lineHoverPaddingX();
    controlHoverMesh.move(context->renderer(), context->pixelSizeX(), context->pixelSizeY(), controlHoverX, 0);
  }
  activeControlIndex = noControlSelection();

  if (openControl != nullptr) { // close open control
    openControl->control()->close();
    openControl = nullptr;
  }
}

void Page::updateColors(const ColorTheme& theme) {
  scrollbar.updateColors(*context, theme.scrollbarControlColor(), theme.scrollbarThumbColor());
  if (tooltip.width())
    tooltip.updateColors(*context, theme.tooltipControlColor());

  if (controlHoverMesh.width()) {
    std::vector<ControlVertex> vertices = controlHoverMesh.relativeVertices();
    for (auto& vertex : vertices)
      memcpy(vertex.color, theme.lineSelectorControlColor(), sizeof(float) * 4u);
    controlHoverMesh.update(context->renderer(), std::move(vertices), context->pixelSizeX(), context->pixelSizeY(),
                            controlHoverMesh.x(), controlHoverMesh.y(), controlHoverMesh.width(), controlHoverMesh.height());
  }
}

void Page::setActivePopup(Popup& popup, std::function<void(uint32_t)> handler) {
  if (openControl != nullptr) {
    openControl->control()->close();
    openControl = nullptr;
  }
  activeControlIndex = noControlSelection();

  popup.open(std::move(handler));
  if (popup.isOpen()) {
    popup.selectIndex(0);
    openPopup = &popup;
  }
}

// ---

void Page::onScroll(uint32_t visibleTopY) {
  if (scrollY != (int32_t)visibleTopY) {
    scrollY = (int32_t)visibleTopY;
    buffers->updateScrollBuffer(context->pixelSizeY(), visibleTopY);
  }
}

void Page::onHover(int32_t controlIndex) {
  if (controlIndex != activeControlIndex) {
    activeControlIndex = controlIndex;
    if (controlIndex != noControlSelection()) {
      const auto* control = &controlRegistry[controlIndex];

      // move hover rectangle (if a control is selected)
      if (controlHoverMesh.width()) {
        int32_t controlHoverX;
        uint32_t controlHoverWidth;
        const int32_t controlX = x_ + (int32_t)Control::fieldsetMarginX(width_) + (int32_t)Control::fieldsetContentMarginX(width_);
        if (control->x() < controlX + (int32_t)Control::pageLabelWidth()) {
          controlHoverX = controlX - (int32_t)Control::lineHoverPaddingX();
          controlHoverWidth = Control::pageLabelWidth() + Control::pageControlWidth()
                            + (Control::lineHoverPaddingX() << 1) + Control::labelMargin();

          if (controlIndex != (int32_t)controlRegistry.size() - 1 && control->y() == (control+1)->y()) { // next control on the same line
            int32_t nextControlRightHoverX = (control + 1)->x() + (int32_t)(control+1)->width() + (int32_t)Control::lineHoverPaddingX();
            if (controlHoverX + (int32_t)controlHoverWidth < nextControlRightHoverX)
              controlHoverWidth = static_cast<uint32_t>(nextControlRightHoverX - controlHoverX);
          }
          else if (control->x() > controlX + (int32_t)(Control::pageLabelWidth() >> 1)) {
            const int32_t paddingX = (controlHoverX + (int32_t)controlHoverWidth) - (control->x() + (int32_t)control->width());
            controlHoverX = control->x() - paddingX;
            controlHoverWidth = control->width() + ((uint32_t)paddingX << 1);
          }
        }
        else if (controlIndex != 0 && control->y() == (control - 1)->y()) { // previous control on the same line
          controlHoverX = (control-1)->x() - (int32_t)Control::lineHoverPaddingX();
          controlHoverWidth = Control::pageLabelWidth() + Control::pageControlWidth()
                            + (Control::lineHoverPaddingX() << 1) + Control::labelMargin();
          int32_t controlRightHoverX = control->x() + (int32_t)control->width() + (int32_t)Control::lineHoverPaddingX();
          if (controlHoverX + (int32_t)controlHoverWidth < controlRightHoverX)
            controlHoverWidth = static_cast<uint32_t>(controlRightHoverX - controlHoverX);
        }
        else {
          controlHoverX = controlX + (int32_t)Control::pageLabelWidth() + 12 - (int32_t)Control::lineHoverPaddingX();
          controlHoverWidth = Control::pageControlWidth() + (Control::lineHoverPaddingX() << 1) + Control::labelMargin() - 12u;
        }
      
        const int32_t controlHoverY = control->y() - static_cast<int32_t>((Control::pageLineHeight() - control->height()) >> 1) - 1;
        if (controlHoverWidth != controlHoverMesh.width()) {
          std::vector<ControlVertex> controlHoverVertices = controlHoverMesh.relativeVertices();
          GeometryGenerator::resizeRoundedRectangleVerticesX(controlHoverVertices.data(), (float)controlHoverWidth, 3.f);
          controlHoverMesh.update(context->renderer(), std::move(controlHoverVertices), context->pixelSizeX(), context->pixelSizeY(),
                                  controlHoverX, controlHoverY, controlHoverWidth, controlHoverMesh.height());
        }
        else
          controlHoverMesh.move(context->renderer(), context->pixelSizeX(), context->pixelSizeY(), controlHoverX, controlHoverY);
      }

      // replace tooltip content
      if (tooltip.width())
        tooltip.updateLabel(*context, control->tooltipMessage() ? control->tooltipMessage() : u" ", LabelBufferType::regular);
    }
    // clear tooltip content
    else if (tooltip.width())
      tooltip.updateLabel(*context, u" ", LabelBufferType::regular);
  }
}


// -- user interactions -- -----------------------------------------------------

int32_t Page::findActiveControlIndex(int32_t mouseX, int32_t mouseY) const noexcept {
  if (controlRegistry.empty())
    return noControlSelection();

  // search for fixed controls at the bottom (won't be detected by binary search)
  uint32_t fixedBottomControlCount = 0;
  if (controlRegistry.back().isFixed()) {
    const ControlRegistration* lastRevIt = controlRegistry.data();
    for (const ControlRegistration* revIt = &controlRegistry.back(); revIt >= lastRevIt && revIt->isFixed(); --revIt) {
      ++fixedBottomControlCount;
      int compare = revIt->compareLocation(mouseX, mouseY, 0);
      if (compare == 0)
        return (uint32_t)controlRegistry.size() - fixedBottomControlCount;
      if (compare <= 0)
        break;
    }
    if (fixedBottomControlCount >= (uint32_t)controlRegistry.size())
      return noControlSelection();
  }

  // page control click detection (binary search)
  uint32_t low = 0;
  uint32_t high = static_cast<uint32_t>(controlRegistry.size() - fixedBottomControlCount - 1u);
  while (low < high) {
    uint32_t mid = (low + high + 1u) >> 1;
    if (controlRegistry[mid].compareLocation(mouseX, mouseY, scrollY) > 0)
      high = mid - 1u;
    else
      low = mid;
  }
  return (controlRegistry[low].compareLocation(mouseX, mouseY, scrollY) == 0) ? low : noControlSelection();
}

// ---

void Page::selectControlIndex(uint32_t controlIndex) {
  if (controlIndex >= (uint32_t)controlRegistry.size()
  || controlRegistry[controlIndex].controlStatus(0,0,0) == ControlStatus::disabled)
    return;
  if (openControl != nullptr) {
    openControl->control()->close();
    openControl = nullptr;
  }

  onHover((int32_t)controlIndex);
  adaptControlSelection((int32_t)controlIndex, &controlRegistry[controlIndex]);
}

void Page::selectPreviousLineControl() {
  if (controlRegistry.empty())
    return;
  if (openControl != nullptr) {
    openControl->control()->close();
    openControl = nullptr;
  }
  if (activeControlIndex == 0) { // top entry -> no previous -> deselect
    onHover(noControlSelection());
    return;
  }
  
  // select previous entry (selected from keyboard/pad)
  int32_t controlIndex, maxAlignedX, limitY;
  if (activeControlIndex != noControlSelection()) {
    controlIndex = activeControlIndex - 1;
    auto* currentControl = &controlRegistry[activeControlIndex];
    maxAlignedX = currentControl->x() - (int32_t)(Control::lineHoverPaddingX()); // horizontal limit (same row)
    limitY = currentControl->y() - (int32_t)(Control::pageLineHeight() >> 1); // vertical limit (same line)
  }
  else { // no current selection -> select last
    controlIndex = static_cast<int32_t>(controlRegistry.size() - 1u);
    maxAlignedX = controlRegistry[0].x() - (int32_t)(controlRegistry[0].width() >> 1);
    limitY = 0x7FFFFFFF; // no limit (max positive value)
  }

  // search for any allowed control in a previous line
  auto* control = &controlRegistry[controlIndex];
  while (controlIndex >= 0 && (control->isFixed()
                            || control->y() > limitY
                            || control->controlStatus(0,0,0) == ControlStatus::disabled)) {
    --controlIndex;
    --control;
  }
  if (controlIndex < 0) { // not found -> deselect
    onHover(noControlSelection());
    return;
  }

  // if multiple controls are on previous line, try to find the one with the best alignment
  limitY = control->y() - (int32_t)(Control::pageLineHeight() >> 1);
  auto* prevControl = control - 1;
  while (controlIndex > 0 && prevControl->y() > limitY && prevControl->x() > maxAlignedX
          && !prevControl->isFixed() && prevControl->controlStatus(0,0,0) != ControlStatus::disabled) {
    --controlIndex;
    --control;
    --prevControl;
  }

  onHover(controlIndex);
  adaptControlSelection(controlIndex, control);
}

void Page::selectPreviousSameLineControl() {
  // no controls -or- no selection -> exit
  if (controlRegistry.empty() || activeControlIndex == noControlSelection())
    return;
  if (openControl != nullptr) {
    openControl->control()->close();
    openControl = nullptr;
  }
  if (activeControlIndex == 0) // top entry -> no previous -> exit
    return;

  int32_t controlIndex = activeControlIndex - 1;
  auto* control = &controlRegistry[controlIndex];
  const int32_t limitY = (control + 1)->y() - (int32_t)(Control::pageLineHeight() >> 1); // vertical limit (same line)

  while (controlIndex >= 0 && control->y() > limitY // search for any allowed control in the same line
      && (control->isFixed() || control->controlStatus(0, 0, 0) == ControlStatus::disabled)) {
    --controlIndex;
    --control;
  }
  if (controlIndex < 0 || control->y() <= limitY) // not found -or- previous line -> exit
    return;

  onHover(controlIndex);
  adaptControlSelection(controlIndex, control);
}

void Page::selectNextLineControl() {
  if (controlRegistry.empty())
    return;
  if (openControl != nullptr) {
    openControl->control()->close();
    openControl = nullptr;
  }
  if (activeControlIndex >= static_cast<int32_t>(controlRegistry.size() - 1u)) { // bottom entry -> no next -> deselect
    onHover(noControlSelection());
    return;
  }
  
  // select next entry (selected from keyboard/pad)
  int32_t controlIndex, maxAlignedX, limitY;
  if (activeControlIndex != noControlSelection()) {
    controlIndex = activeControlIndex + 1;
    auto* currentControl = &controlRegistry[activeControlIndex];
    maxAlignedX = currentControl->x() + (int32_t)(Control::lineHoverPaddingX()); // horizontal limit (same row)
    limitY = currentControl->y() + (int32_t)(Control::pageLineHeight() >> 1); // vertical limit (same line)
  }
  else { // no current selection -> select first
    controlIndex = 0;
    maxAlignedX = controlRegistry[0].x() + (int32_t)(Control::lineHoverPaddingX());
    limitY = 0X80000000; // no limit (min negative value)
  }

  // search for any allowed control in a next line
  auto* control = &controlRegistry[controlIndex];
  while (controlIndex < (int32_t)controlRegistry.size()
      && (control->isFixed() || control->y() <= limitY || control->controlStatus(0,0,0) == ControlStatus::disabled)) {
    ++controlIndex;
    ++control;
  }
  if (controlIndex >= (int32_t)controlRegistry.size()) { // not found -> deselect
    onHover(noControlSelection());
    return;
  }

  // if multiple controls are on next line, try to find the one with the best alignment
  limitY = control->y() + (int32_t)(Control::pageLineHeight() >> 1);
  auto* nextControl = control + 1;
  while (controlIndex < (int32_t)controlRegistry.size() - 1 && nextControl->y() <= limitY && nextControl->x() <= maxAlignedX
          && !nextControl->isFixed() && nextControl->controlStatus(0,0,0) != ControlStatus::disabled) {
    ++controlIndex;
    ++control;
    ++nextControl;
  }

  onHover(controlIndex);
  adaptControlSelection(controlIndex, control);
}

void Page::selectNextSameLineControl() {
  // no controls -or- no selection -or- selection is last item -> exit
  if (controlRegistry.empty() || activeControlIndex == noControlSelection())
    return;
  if (openControl != nullptr) {
    openControl->control()->close();
    openControl = nullptr;
  }
  if (activeControlIndex == static_cast<int32_t>(controlRegistry.size() - 1u)) // bottom entry -> no next -> exit
    return;

  int32_t controlIndex = activeControlIndex + 1;
  auto* control = &controlRegistry[controlIndex];
  const int32_t limitY = (control - 1)->y() + (int32_t)(Control::pageLineHeight() >> 1); // vertical limit (same line)

  while (controlIndex < (int32_t)controlRegistry.size() && control->y() <= limitY
      && control->isFixed() && control->controlStatus(0, 0, 0) == ControlStatus::disabled) {
    ++controlIndex;
    ++control;
  }
  if (controlIndex >= (int32_t)controlRegistry.size() || control->y() > limitY) // not found -or- next line -> exit
    return;

  onHover(controlIndex);
  adaptControlSelection(controlIndex, control);
}

// ---

void Page::adaptControlSelection(int32_t controlIndex, ControlRegistration* control) {
  if (controlIndex != noControlSelection()) {
    if (scrollbar.isEnabled()) { // auto-scroll if needed
      const uint32_t controlTopLevel = static_cast<uint32_t>(control->y() - scrollbar.y());
      if (controlTopLevel < scrollbar.visibleTop()) {
        scrollbar.setTopPosition(*context, (controlTopLevel > Control::autoScrollPaddingY())
                                           ? controlTopLevel - Control::autoScrollPaddingY() : 0);
      }
      else {
        const uint32_t controlBottomLevel = static_cast<uint32_t>(control->bottomY() - scrollbar.y());
        if (controlBottomLevel >= scrollbar.visibleBottom() - bottomBarHeight)
          scrollbar.setBottomPosition(*context, controlBottomLevel + Control::autoScrollPaddingY() + bottomBarHeight);
      }
    }
  }
}

// ---

void Page::mouseClick(int32_t mouseX, int32_t mouseY) {
  isMouseDown_ = true;
  isControllerUsed_ = false;

  // click on scrollbar
  if (scrollbar.isEnabled() && mouseX >= scrollbar.x()) {
    if (scrollbar.isEnabled() && scrollbar.isHover(mouseX, mouseY))
      scrollbar.click(*context, mouseY, true);
  }
  // click on open popup -> click/close it
  else if (openPopup != nullptr) {
    if (!openPopup->click(*context, mouseX, mouseY))
      openPopup = nullptr;
  }
  // click with an open control -> verify and click/close it
  else if (openControl != nullptr) {
    auto status = openControl->controlStatus(mouseX, mouseY, scrollY);
    if (status == ControlStatus::hover) {
      if (!openControl->control()->click(*context, mouseX, mouseY)) {
        if (openControl != nullptr) {                        // check if not NULL: on lang/theme change, the control will
          auto controlType = openControl->control()->type(); // no longer exist after 'click' (-> openControl reset to NULL)
          if (controlType == ControlType::comboBox)
            shrinkScrollArea();
          else if (controlType == ControlType::keyBinding) {
            const auto* target = reinterpret_cast<const KeyBinding*>(openControl->control());
            if (target->keyboardValue() != KeyBinding::emptyKeyValue())
              resolveKeyboardBindings(target);
          }
        }
        openControl = nullptr;
      }
    }
    else { // clicked elsewhere -> close open control (and don't click on any other control)
      openControl->control()->close();
      auto controlType = openControl->control()->type();
      openControl = nullptr; // reset open control BEFORE calling mouseMove

      if (controlType != ControlType::comboBox || !shrinkScrollArea())
        mouseMove(mouseX, mouseY);
    }
  }
  // click on page control
  else {
    int32_t controlIndex = findActiveControlIndex(mouseX, mouseY);
    if (controlIndex != noControlSelection()) {
      auto* activeControl = &controlRegistry[controlIndex];
      if (activeControl->control()->click(*context, mouseX, mouseY)) {
        openControl = activeControl;

        // adjust visibility if combo-box longer than page size
        if (openControl->control()->type() == ControlType::comboBox) {
          const auto* target = reinterpret_cast<const ComboBox*>(openControl->control());
          expandScrollArea(target->y() + (int32_t)target->height());
        }
      }
    }
  }
}

void Page::mouseButton(int32_t, int32_t, MouseButton button) {
  if (openControl != nullptr && openControl->control()->type() == ControlType::keyBinding) {
    auto* target = reinterpret_cast<KeyBinding*>(openControl->control());
    if (!target->setKeyboardValue(*context, KeyBinding::toMouseKeyCode(button))) {
      if (target->keyboardValue() != KeyBinding::emptyKeyValue())
        resolveKeyboardBindings(target);
      openControl = nullptr;
    }
  }
}

void Page::mouseMove(int32_t mouseX, int32_t mouseY) {
  isControllerUsed_ = false;

  // moving while dragging scrollbar
  if (scrollbar.isDragged()) {
    scrollbar.mouseMove(*context, mouseY);
    onHover(noControlSelection());
  }
  // moving with an open popup -> update it
  else if (openPopup != nullptr) {
    openPopup->mouseMove(mouseX, mouseY);
  }
  // moving with an open control -> update it
  else if (openControl != nullptr) {
    auto status = openControl->controlStatus(mouseX, mouseY, scrollY);
    if (status == ControlStatus::hover)
      openControl->control()->mouseMove(*context, mouseX, openControl->isFixed() ? mouseY : (mouseY + scrollY));
  }
  // page control index detection
  else {
    int32_t controlIndex = findActiveControlIndex(mouseX, mouseY);
    if (controlIndex != noControlSelection()) {
      auto status = controlRegistry[controlIndex].controlStatus(mouseX, mouseY, scrollY);
      if (status != ControlStatus::hover)
        controlIndex = noControlSelection();
    }
    onHover(controlIndex);
  }
}

void Page::mouseUp(int32_t mouseX, int32_t mouseY) {
  isMouseDown_ = false;
  if (scrollbar.isDragged()) {
    scrollbar.mouseUp(*context, mouseY);
  }
  else if (openControl != nullptr) {
    if (openControl->control()->mouseUp(*context, mouseX))
      openControl = nullptr;
  }
}

void Page::mouseScroll(int32_t deltaY) {
  if (scrollbar.isEnabled())
    scrollbar.scroll(*context, deltaY);
}

void Page::mouseLeave() noexcept {
  if (scrollbar.isDragged())
    scrollbar.mouseLeave();
}

// ---

void Page::keyDown(char32_t keyCode) {
  if (openControl != nullptr && openControl->control()->type() == ControlType::textBox) {
    auto control = reinterpret_cast<TextBox*>(openControl->control());
    if (control->isEditMode()) {
      if ((keyCode > U'\x1F' && keyCode < U'\x7F') || keyCode > U'\x9F')
        control->addChar(*context, keyCode);
    }
  }
}

bool Page::vkeyDown(uint32_t virtualKeyCode) {
  isControllerUsed_ = false;
  if (openPopup != nullptr) {
    switch (virtualKeyCode) {
      case _P_VK_ARROW_UP:
      case _P_VK_ARROW_LEFT: openPopup->selectPrevious(); break;
      case _P_VK_ARROW_DOWN:
      case _P_VK_ARROW_RIGHT: openPopup->selectNext(); break;
      case _P_VK_ESC:
      case _P_VK_DELETE:
        openPopup->close();
        openPopup = nullptr;
        break;
      case _P_VK_ENTER:
      case _P_VK_ENTER_PAD:
        if (!openPopup->clickSelectedIndex())
          openPopup = nullptr;
        break;
      default: break;
    }
  }
  else if (openControl != nullptr) {
    auto controlType = openControl->control()->type();

    // control edit - text-box
    if (controlType == ControlType::textBox) {
      auto* target = reinterpret_cast<TextBox*>(openControl->control());
      if (target->isEditMode()) {
        switch (virtualKeyCode) {
          case _P_VK_DELETE:      if (target->nextChar(*context)) { target->removeChar(*context); } break; // erase next char
          case _P_VK_BACKSPACE:   target->removeChar(*context); break;   // erase previous char
          case _P_VK_ARROW_LEFT:  target->previousChar(*context); break; // move caret left
          case _P_VK_ARROW_RIGHT: target->nextChar(*context); break;     // move caret right
          case _P_VK_ADD:
          case _P_VK_PUNCT_PLUS: // increment
            if (target->valueDataType() == TextBoxType::integer && !target->click(*context, TextBox::plusMinusCoordX(), TextBox::plusCoordY()))
              target->click(*context, TextBox::noMouseCoord(), TextBox::noMouseCoord()); // keep focus
            break;
          case _P_VK_SUBTRACT:
          case _P_VK_PUNCT_MINUS: // decrement
            if (target->valueDataType() == TextBoxType::integer && !target->click(*context, TextBox::plusMinusCoordX(), TextBox::minusCoordY()))
              target->click(*context, TextBox::noMouseCoord(), TextBox::noMouseCoord()); // keep focus
            break;
          case _P_VK_TAB:
          case _P_VK_ARROW_DOWN: selectNextLineControl(); break;    // move to next control line
          case _P_VK_ARROW_UP: selectPreviousLineControl(); break;  // move to previous control line
          default: break;
        }
      }
      else openControl = nullptr;
    }
    // control edit - combo-box
    else if (controlType == ControlType::comboBox) {
      auto* target = reinterpret_cast<ComboBox*>(openControl->control());
      if (target->isOpen()) {
        switch (virtualKeyCode) {
          case _P_VK_ENTER:
          case _P_VK_ENTER_PAD: // confirm
            if (!target->click(*context, target->controlX(), target->y())) {
              openControl = nullptr;
              shrinkScrollArea();
            }
            break;
          case _P_VK_TAB: // close
            target->close();
            openControl = nullptr;
            shrinkScrollArea();
            break; 
          case _P_VK_ARROW_UP: // previous option
            target->selectPrevious(*context);
            if (scrollbar.isEnabled() && target->getHoverLineY() < y() + scrollY)
              scrollbar.setTopPosition(*context, target->getHoverLineY());
            break;
          case _P_VK_ARROW_DOWN: // next option
            target->selectNext(*context);
            if (scrollbar.isEnabled() && target->getHoverLineY() + (int32_t)target->controlHeight() > y() + (int32_t)height() + scrollY)
              scrollbar.setBottomPosition(*context, target->getHoverLineY() + (int32_t)target->controlHeight());
            break;
          default: break;
        }
      }
      else openControl = nullptr;
    }
    else if (controlType == ControlType::keyBinding) {
      auto* target = reinterpret_cast<KeyBinding*>(openControl->control());
      if (!target->setKeyboardValue(*context, virtualKeyCode)) {
        if (target->keyboardValue() != KeyBinding::emptyKeyValue())
          resolveKeyboardBindings(target);
        openControl = nullptr;
      }
      return true;
    }
    else openControl = nullptr;
  }
  // control selection
  else {
    switch (virtualKeyCode) {
      case _P_VK_ENTER:
      case _P_VK_ENTER_PAD:
        if (activeControlIndex != noControlSelection()) {
          auto* control = &controlRegistry[activeControlIndex];
          auto controlType = control->control()->type();
          if (controlType != ControlType::textBox && controlType != ControlType::ruler && controlType != ControlType::slider) {
            if (control->control()->click(*context, control->rightX() - (int32_t)(control->width() >> 1), control->y())) {
              if (control->control()->type() == ControlType::comboBox) {
                const auto* target = reinterpret_cast<const ComboBox*>(control->control());
                expandScrollArea(target->y() + (int32_t)target->height());
              }
              openControl = control;
            }
          }
        }
        break;
      case _P_VK_TAB:
      case _P_VK_ARROW_DOWN: selectNextLineControl(); break;   // move to next control line
      case _P_VK_ARROW_UP: selectPreviousLineControl(); break; // move to previous control line
      case _P_VK_ARROW_LEFT:
        if (activeControlIndex != noControlSelection()) {
          auto* control = &controlRegistry[activeControlIndex];
          auto controlType = control->control()->type();
          if (controlType == ControlType::ruler)
            reinterpret_cast<Ruler*>(control->control())->selectPrevious(*context);
          else if (controlType == ControlType::slider)
            reinterpret_cast<Slider*>(control->control())->selectPrevious();
          else
            selectPreviousSameLineControl(); // move to previous control on same line (if any)
        }
        else selectPreviousSameLineControl(); // move to previous control on same line (if any)
        break;
      case _P_VK_ARROW_RIGHT:
        if (activeControlIndex != noControlSelection()) {
          auto* control = &controlRegistry[activeControlIndex];
          auto controlType = control->control()->type();
          if (controlType == ControlType::ruler)
            reinterpret_cast<Ruler*>(control->control())->selectNext(*context);
          else if (controlType == ControlType::slider)
            reinterpret_cast<Slider*>(control->control())->selectNext();
          else
            selectNextSameLineControl(); // move to next control on same line (if any)
        }
        else selectNextSameLineControl(); // move to next control on same line (if any)
        break;
      case _P_VK_DELETE:
        if (activeControlIndex != noControlSelection()) {
          auto* control = &controlRegistry[activeControlIndex];
          auto controlType = control->control()->type();
          if (controlType == ControlType::keyBinding) { // clear binding
            reinterpret_cast<KeyBinding*>(control->control())->setKeyboardValue(*context, KeyBinding::emptyKeyValue());
            reinterpret_cast<KeyBinding*>(control->control())->setControllerValue(*context, KeyBinding::emptyKeyValue());
          }
          else if (controlType == ControlType::tile) {
            reinterpret_cast<Tile*>(control->control())->remove();
          }
        }
        break;
      case _P_VK_SPACE:
        if (activeControlIndex != noControlSelection()) {
          auto* control = &controlRegistry[activeControlIndex];
          if (control->control()->type() == ControlType::tile) {
            reinterpret_cast<Tile*>(control->control())->edit();
          }
        }
        break;
      default: break;
    }
  }
  return false;
}

void Page::padButtonDown(uint32_t virtualKeyCode) {
  isControllerUsed_ = true;
  if (openControl != nullptr && openPopup == nullptr) {
    auto controlType = openControl->control()->type();
    if (controlType == ControlType::textBox) {
      switch (virtualKeyCode) {
        case /*XINPUT_GAMEPAD_DPAD_LEFT*/0x0004: vkeyDown(_P_VK_SUBTRACT); return;
        case /*XINPUT_GAMEPAD_DPAD_RIGHT*/0x0008: vkeyDown(_P_VK_ADD); return;
        default: break;
      }
    }
    else if (controlType == ControlType::keyBinding) {
      auto* target = reinterpret_cast<KeyBinding*>(openControl->control());
      if (!target->setControllerValue(*context, virtualKeyCode)) {
        if (target->keyboardValue() != KeyBinding::emptyKeyValue())
          resolveKeyboardBindings(target);
        openControl = nullptr;
      }
      return;
    }
  }
  switch (virtualKeyCode) {
    case /*XINPUT_GAMEPAD_DPAD_UP*/0x0001: vkeyDown(_P_VK_ARROW_UP); break;
    case /*XINPUT_GAMEPAD_DPAD_DOWN*/0x0002: vkeyDown(_P_VK_ARROW_DOWN); break;
    case /*XINPUT_GAMEPAD_DPAD_LEFT*/0x0004: vkeyDown(_P_VK_ARROW_LEFT); break;
    case /*XINPUT_GAMEPAD_DPAD_RIGHT*/0x0008: vkeyDown(_P_VK_ARROW_RIGHT); break;
    case /*XINPUT_GAMEPAD_BACK*/0x0020: vkeyDown(_P_VK_BACKSPACE); break;
    case /*XINPUT_GAMEPAD_START*/0x0010:
    case /*XINPUT_GAMEPAD_A*/0x1000: vkeyDown(_P_VK_ENTER); break;
    case /*XINPUT_GAMEPAD_B*/0x2000: vkeyDown(_P_VK_DELETE); break;
    case /*XINPUT_GAMEPAD_Y*/0x8000: vkeyDown(_P_VK_SPACE); break;
    default: break;
  }
}


// -- rendering -- -------------------------------------------------------------

bool Page::drawBackgrounds(int32_t mouseX, int32_t mouseY) {
  // fixed geometry
  auto& renderer = context->renderer();
  if (tooltip.width() || scrollbar.isEnabled()) {
    if (!buffers->isFixedLocationBuffer())
      buffers->bindFixedLocationBuffer(renderer, ScissorRectangle(0, 0, context->clientWidth(), context->clientHeight()));

    if (tooltip.width())
      tooltip.drawBackground(*context, *buffers);
    scrollbar.drawControl(*context, mouseX, mouseY, *buffers);
  }

  // scrollable geometry
  if (activeControlIndex != noControlSelection() && controlHoverMesh.width()) {
    buffers->bindScrollLocationBuffer(renderer, ScissorRectangle(x(), y(), width(), contentHeight()));
    buffers->bindControlBuffer(renderer, ControlBufferType::regular);
    controlHoverMesh.draw(renderer);
  }

  drawPageBackgrounds(mouseX, mouseY);
  return ((openControl != nullptr && openControl->control()->type() == ControlType::comboBox) || openPopup != nullptr);
}

void Page::drawLabels() {
  if (tooltip.width()) { // fixed geometry
    if (!buffers->isFixedLocationBuffer())
      buffers->bindFixedLocationBuffer(context->renderer(), ScissorRectangle(0, 0, context->clientWidth(), context->clientHeight()));
    tooltip.drawLabel(*context, *buffers);
  }

  // custom page geometry
  drawPageLabels();
}

void Page::drawForegrounds() {
  if (openPopup != nullptr) {
    buffers->bindFixedLocationBuffer(context->renderer(), ScissorRectangle(0, 0, context->clientWidth(), context->clientHeight()));
    openPopup->drawBackground(*context, *buffers);
  }
  else if (openControl != nullptr && openControl->control()->type() == ControlType::comboBox) {
    auto& renderer = context->renderer();
    ScissorRectangle fullWindowArea(0, 0, context->clientWidth(), context->clientHeight()); // visible outside of scroll area -> full window
    if (openControl->isFixed())
      buffers->bindFixedLocationBuffer(renderer, fullWindowArea);
    else
      buffers->bindScrollLocationBuffer(renderer, fullWindowArea);

    reinterpret_cast<ComboBox*>(openControl->control())->drawDropdown(*context, *buffers);
  }
}

void Page::drawForegroundLabels() {
  if (openPopup != nullptr) {
    buffers->bindFixedLocationBuffer(context->renderer(), ScissorRectangle(0, 0, context->clientWidth(), context->clientHeight()));
    openPopup->drawLabels(*context, *buffers);
  }
  else if (openControl != nullptr && openControl->control()->type() == ControlType::comboBox) {
    auto& renderer = context->renderer();
    ScissorRectangle fullWindowArea(0, 0, context->clientWidth(), context->clientHeight()); // visible outside of scroll area -> full window
    if (openControl->isFixed())
      buffers->bindFixedLocationBuffer(renderer, fullWindowArea);
    else
      buffers->bindScrollLocationBuffer(renderer, fullWindowArea);

    reinterpret_cast<ComboBox*>(openControl->control())->drawOptions(*context, *buffers);
  }
}
