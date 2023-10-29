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
#include <video/window_keycodes.h>
#include "menu/controls/button.h"
#include "menu/controls/check_box.h"
#include "menu/controls/combo_box.h"
#include "menu/controls/text_box.h"
#include "menu/controls/ruler.h"
#include "menu/controls/slider.h"
#include "menu/controls/geometry_generator.h"
#include "menu/page.h"

using namespace video_api;
using namespace display::controls;
using namespace menu::controls;
using namespace menu;


Page::Page(std::shared_ptr<RendererContext> context_, std::shared_ptr<RendererStateBuffers> buffers_,
           const ColorTheme& theme, int32_t x, int32_t y, uint32_t width, uint32_t visibleHeight,
           uint32_t totalPageHeight, bool enableTooltip)
  : context(std::move(context_)),
    buffers(std::move(buffers_)),
    scrollY(0),
    activeControlIndex(noControlSelection()),
    x_(x),
    width_(width) {
  assert(this->context != nullptr && this->buffers != nullptr);

  // create page scrollbar
  const int32_t scrollBarX = x + (int32_t)width - (int32_t)ColorTheme::scrollbarWidth();
  auto scrollHandler = std::bind(&Page::onScroll, this, std::placeholders::_1);
  scrollbar = ScrollBar(*context, theme.scrollbarControlColor(), theme.scrollbarThumbColor(),
                        scrollBarX, y, ColorTheme::scrollbarWidth(), std::move(scrollHandler),
                        visibleHeight, totalPageHeight, (ColorTheme::pageLineHeight() >> 1));

  // create tooltip bar
  if (enableTooltip) {
    const uint32_t tooltipBarWidth = scrollbar.isEnabled() ? (width - scrollbar.width()) : width;
    tooltip = Tooltip(*context, U" ", FontType::inputText, LabelBufferType::regular,
                      x, y + (int32_t)visibleHeight - (int32_t)ColorTheme::tooltipBarHeight(),
                      tooltipBarWidth, ColorTheme::tooltipBarHeight(), theme.scrollbarWidth(),
                      theme.tooltipControlColor(), display::ControlIconType::none);
  }

  // create control line hover area
  const int32_t controlHoverX = x + (int32_t)ColorTheme::pageFieldsetMarginX(width)
                              + (int32_t)ColorTheme::fieldsetPaddingX(width) - ColorTheme::lineHoverPaddingX();
  constexpr const uint32_t controlHoverWidth = ColorTheme::pageLabelWidth() + ColorTheme::pageControlWidth()
                                             + (ColorTheme::lineHoverPaddingX() << 1) + Control::labelMargin();
  std::vector<ControlVertex> controlHoverVertices(static_cast<size_t>(8));
  GeometryGenerator::fillCornerCutRectangleVertices(controlHoverVertices.data(), theme.lineSelectorControlColor(),
                                                    0.f, (float)controlHoverWidth, 0.f, -(float)ColorTheme::pageLineHeight(), 3.f);
  std::vector<uint32_t> controlHoverIndices{ 0,1,2,2,1,3,  2,3,4,4,3,5,  4,5,6,6,5,7 };

  controlHoverMesh = ControlMesh(context->renderer(), std::move(controlHoverVertices), controlHoverIndices,
                                 context->pixelSizeX(), context->pixelSizeY(), controlHoverX, 0,
                                 controlHoverWidth, ColorTheme::pageLineHeight());
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

void Page::move(int32_t x, int32_t y, uint32_t width, uint32_t visibleHeight, uint32_t totalPageHeight) {
  const int32_t scrollBarX = x + (int32_t)width - (int32_t)scrollbar.width();
  scrollbar.move(*context, scrollBarX, y, visibleHeight, totalPageHeight); // will call onScroll if needed

  if (tooltip.width()) {
    const uint32_t tooltipBarWidth = scrollbar.isEnabled() ? (width - scrollbar.width()) : width;
    tooltip.move(*context, x, y + (int32_t)visibleHeight - (int32_t)ColorTheme::tooltipBarHeight(), tooltipBarWidth);
    if (activeControlIndex != noControlSelection())
      tooltip.updateLabel(*context, U" ", LabelBufferType::regular);
  }

  const int32_t controlHoverX = x + (int32_t)ColorTheme::pageFieldsetMarginX(width)
                              + (int32_t)ColorTheme::fieldsetPaddingX(width) - ColorTheme::lineHoverPaddingX();
  controlHoverMesh.move(context->renderer(), context->pixelSizeX(), context->pixelSizeY(), controlHoverX, 0);
  activeControlIndex = noControlSelection();

  this->x_ = x;
  this->width_ = width;
  if (openControl != nullptr) { // close open control
    openControl->control()->close();
    openControl = nullptr;
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

    // move hover rectangle (if a control is selected)
    if (controlIndex != noControlSelection()) {
      const auto* control = &controlRegistry[controlIndex];
      bool isLeftPadded = (controlIndex == 0 || control->y() != (control - 1)->y());
      bool isRightPadded = (controlIndex == (int32_t)controlRegistry.size() - 1 || control->y() != (control+1)->y());

      int32_t controlHoverX;
      uint32_t controlHoverWidth;
      if (isLeftPadded && isRightPadded) {
        controlHoverX = control->x() - (int32_t)ColorTheme::lineHoverPaddingX();
        controlHoverWidth = ColorTheme::pageLabelWidth() + ColorTheme::pageControlWidth()
                          + (ColorTheme::lineHoverPaddingX() << 1) + Control::labelMargin();
      }
      else {
        controlHoverX = isLeftPadded ? (control->x() - (int32_t)ColorTheme::lineHoverPaddingX()) : (control->x() - 3);
        controlHoverWidth = control->width() + ColorTheme::lineHoverPaddingX() + 3;
      }
      
      const int32_t controlHoverY = control->y() - static_cast<int32_t>((ColorTheme::pageLineHeight() - control->height()) >> 1) - 1;
      if (controlHoverWidth != controlHoverMesh.width()) {
        std::vector<ControlVertex> controlHoverVertices = controlHoverMesh.relativeVertices();
        GeometryGenerator::resizeCornerCutRectangleVerticesX(controlHoverVertices.data(), (float)controlHoverWidth, 3.f);
        controlHoverMesh.update(context->renderer(), std::move(controlHoverVertices), context->pixelSizeX(), context->pixelSizeY(),
                                controlHoverX, controlHoverY, controlHoverWidth, controlHoverMesh.height());
      }
      else
        controlHoverMesh.move(context->renderer(), context->pixelSizeX(), context->pixelSizeY(), controlHoverX, controlHoverY);

      // replace tooltip content
      if (tooltip.width())
        tooltip.updateLabel(*context, control->tooltipMessage() ? control->tooltipMessage() : U" ", LabelBufferType::regular);
    }
    // clear tooltip content
    else if (tooltip.width())
      tooltip.updateLabel(*context, U" ", LabelBufferType::regular);
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

void Page::selectPreviousControlIndex() noexcept {
  if (controlRegistry.empty())
    return;
  if (openControl != nullptr) {
    openControl->control()->close();
    openControl = nullptr;
  }

  if (activeControlIndex == 0) { // top entry -> no previous -> deselect
    onHover(noControlSelection());
  }
  else { // select previous entry (selected from key/pad)
    int32_t controlIndex = (activeControlIndex != noControlSelection())
                         ? (activeControlIndex - 1) : static_cast<int32_t>(controlRegistry.size() - 1u);
    auto* control = &controlRegistry[controlIndex];
    while (controlIndex >= 0 && control->controlStatus(0,0,0) == ControlStatus::disabled) {
      --controlIndex;
      --control;
    }
    if (controlIndex < 0)
      controlIndex = noControlSelection();
    onHover(controlIndex);

    if (controlIndex != noControlSelection()) {
      if (scrollbar.isEnabled()) { // auto-scroll if needed
        uint32_t controlLevel = static_cast<uint32_t>(control->y() - scrollbar.y());
        if (controlLevel < scrollbar.visibleTop()) {
          bool isTopLevel = (controlIndex == 0 || controlLevel < ColorTheme::autoScrollPaddingY());
          scrollbar.setTopPosition(*context, isTopLevel ? controlLevel - ColorTheme::autoScrollPaddingY() : 0);
        }
      }
      if (control->control()->Type() == ControlType::textBox) { // automatic focus if text-box
        if (control->control()->click(*context, TextBox::noMouseCoord()))
          openControl = control;
      }
    }
  }
}

void Page::selectNextControlIndex() noexcept {
  if (controlRegistry.empty())
    return;
  if (openControl != nullptr) {
    openControl->control()->close();
    openControl = nullptr;
  }

  if (activeControlIndex >= static_cast<int32_t>(controlRegistry.size() - 1u)) { // bottom entry -> no next -> deselect
    onHover(noControlSelection());
  }
  else { // select next entry (selected from key/pad)
    int32_t controlIndex = (activeControlIndex != noControlSelection()) ? (activeControlIndex + 1) : 0;
    auto* control = &controlRegistry[controlIndex];
    while (controlIndex < (int32_t)controlRegistry.size() && control->controlStatus(0,0,0) == ControlStatus::disabled) {
      ++controlIndex;
      ++control;
    }
    if (controlIndex >= (int32_t)controlRegistry.size())
      controlIndex = noControlSelection();
    onHover(controlIndex);

    if (controlIndex != noControlSelection()) {
      if (scrollbar.isEnabled()) { // auto-scroll if needed
        uint32_t controlBottomLevel = static_cast<uint32_t>(control->bottomY() - scrollbar.y());
        if (controlBottomLevel >= scrollbar.visibleBottom())
          scrollbar.setBottomPosition(*context, controlBottomLevel + ColorTheme::autoScrollPaddingY());
      }
      if (control->control()->Type() == ControlType::textBox) { // automatic focus if text-box
        if (control->control()->click(*context, TextBox::noMouseCoord()))
          openControl = control;
      }
    }
  }
}

// ---

void Page::mouseDown(int32_t mouseX, int32_t mouseY) {
  // click with an open control -> verify and click/close it
  if (openControl != nullptr) {
    auto status = openControl->controlStatus(mouseX, mouseY, scrollY);
    if (status == ControlStatus::hover) {
      if (!openControl->control()->click(*context, mouseX))
        openControl = nullptr;
    }
    else { // clicked elsewhere -> close open control (and don't click on any other control)
      openControl->control()->close();
      openControl = nullptr;
    }
  }
  // click on scrollbar
  else if (mouseX >= scrollbar.x()) {
    if (scrollbar.isEnabled() && scrollbar.isHover(mouseX, mouseY))
      scrollbar.click(*context, mouseY, true);
  }
  // page control index detection
  else {
    int32_t controlIndex = findActiveControlIndex(mouseX, mouseY);
    if (controlIndex != noControlSelection()) {
      auto* activeControl = &controlRegistry[controlIndex];
      if (activeControl->control()->click(*context, mouseX))
        openControl = activeControl;
    }
  }
}

void Page::mouseMove(int32_t mouseX, int32_t mouseY) {
  this->mouseX_ = mouseX;
  this->mouseY_ = mouseY;

  // moving with an open control -> update it
  if (openControl != nullptr) {
    auto status = openControl->controlStatus(mouseX, mouseY, scrollY);
    if (status == ControlStatus::hover)
      openControl->control()->mouseMove(*context, mouseX, mouseY);
  }
  // moving while dragging scrollbar
  else if (scrollbar.isDragged()) {
    scrollbar.mouseMove(*context, mouseY);
    onHover(noControlSelection());
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
  mouseX_ = -1;
  mouseY_ = -1;
  if (scrollbar.isDragged())
    scrollbar.mouseLeave();
}

// ---

void Page::keyDown(char32_t keyCode) {
  if (openControl != nullptr && openControl->control()->Type() == ControlType::textBox) {
    auto control = reinterpret_cast<TextBox*>(openControl->control());
    if (control->isEditMode()) {
      if ((keyCode > U'\x1F' && keyCode < U'\x7F') || keyCode > U'\x9F')
        control->addChar(*context, keyCode);
    }
  }
}

void Page::vkeyDown(uint32_t virtualKeyCode) {
  if (openControl != nullptr) {
    auto controlType = openControl->control()->Type();

    // control edit - text-box
    if (controlType == ControlType::textBox) {
      auto target = reinterpret_cast<TextBox*>(openControl->control());
      if (target->isEditMode()) {
        switch (virtualKeyCode) {
          case _P_VK_DELETE:      if (target->nextChar(*context)) { target->removeChar(*context); } break; // erase next char
          case _P_VK_BACKSPACE:   target->removeChar(*context); break;   // erase previous char
          case _P_VK_ARROW_LEFT:  target->previousChar(*context); break; // move caret left
          case _P_VK_ARROW_RIGHT: target->nextChar(*context); break;     // move caret right
          case _P_VK_TAB:
          case _P_VK_ARROW_DOWN: selectNextControlIndex(); break;        // move to next control
          case _P_VK_ARROW_UP: selectPreviousControlIndex(); break;      // move to previous control
          default: break;
        }
      }
      else openControl = nullptr;
    }
    // control edit - combo-box
    else if (controlType == ControlType::comboBox) {
      auto target = reinterpret_cast<ComboBox*>(openControl->control());
      if (target->isOpen()) {
        switch (virtualKeyCode) {
          case _P_VK_ENTER:
          case _P_VK_ENTER_PAD:  if (!target->click(*context, target->controlX())) { openControl = nullptr; } break; // confirm
          case _P_VK_TAB:        target->close(); openControl = nullptr; break; // close
          case _P_VK_ARROW_UP:   target->selectPrevious(*context); break;       // previous option
          case _P_VK_ARROW_DOWN: target->selectNext(*context); break;           // next option
          default: break;
        }
      }
      else openControl = nullptr;
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
          auto controlType = control->control()->Type();
          if (controlType == ControlType::button || controlType == ControlType::checkBox || controlType == ControlType::comboBox) {
            if (control->control()->click(*context, control->rightX() - 10))
              openControl = control;
          }
        }
        break;
      case _P_VK_TAB:
      case _P_VK_ARROW_DOWN: selectNextControlIndex(); break;   // move to next control
      case _P_VK_ARROW_UP: selectPreviousControlIndex(); break; // move to previous control
      case _P_VK_ARROW_LEFT:
        if (activeControlIndex != noControlSelection()) {
          auto* control = &controlRegistry[activeControlIndex];
          auto controlType = control->control()->Type();
          if (controlType == ControlType::ruler)
            reinterpret_cast<Ruler*>(control->control())->selectPrevious(*context);
          else if (controlType == ControlType::slider)
            reinterpret_cast<Slider*>(control->control())->selectPrevious();
        }
        break;
      case _P_VK_ARROW_RIGHT:
        if (activeControlIndex != noControlSelection()) {
          auto* control = &controlRegistry[activeControlIndex];
          auto controlType = control->control()->Type();
          if (controlType == ControlType::ruler)
            reinterpret_cast<Ruler*>(control->control())->selectNext(*context);
          else if (controlType == ControlType::slider)
            reinterpret_cast<Slider*>(control->control())->selectNext();
        }
        break;
      default: break;
    }
  }
}

void Page::padButtonDown(uint32_t virtualKeyCode) {
  //...
  //...
  //...
}


// -- rendering -- -------------------------------------------------------------

bool Page::drawBackgrounds() {
  // scrollable geometry
  auto& renderer = context->renderer();
  buffers->bindScrollLocationBuffer(renderer, ScissorRectangle(x_, scrollbar.y(), width_, contentHeight()));

  if (activeControlIndex != noControlSelection()) {
    buffers->bindControlBuffer(renderer, ControlBufferType::regular);
    controlHoverMesh.draw(renderer);
  }
  bool hasForeground = drawPageBackgrounds(mouseX_, mouseY_);

  // fixed geometry
  ScissorRectangle fullWindowArea(0, 0, context->clientWidth(), context->clientHeight());
  buffers->bindFixedLocationBuffer(renderer, fullWindowArea);

  scrollbar.drawControl(*context, mouseX_, mouseY_, *buffers);
  if (tooltip.width())
    tooltip.drawBackground(*context, *buffers);
  return hasForeground;
}

void Page::drawLabels() {
  // scrollable geometry
  buffers->bindScrollLocationBuffer(context->renderer(), ScissorRectangle(x_, scrollbar.y(), width_, contentHeight()));

  drawPageLabels();

  if (tooltip.width()) {
    ScissorRectangle fullWindowArea(0, 0, context->clientWidth(), context->clientHeight()); // fixed geometry
    buffers->bindFixedLocationBuffer(context->renderer(), fullWindowArea);

    tooltip.drawLabel(*context, *buffers);
  }
}