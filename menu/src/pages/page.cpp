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
#include "menu/controls/geometry_generator.h"
#include "menu/pages/page.h"

using namespace video_api;
using namespace pandora::video;
using namespace display::controls;
using namespace menu::controls;
using namespace menu::pages;
using namespace menu;


static ControlMesh generateBackground(RendererContext& context, const ColorTheme& theme,
                                      int32_t x, int32_t y, uint32_t width, uint32_t height) {
  std::vector<ControlVertex> backgroundVertices;
  std::vector<uint32_t> indices;

  if (theme.backgroundStyle() == BackgroundStyle::radialGradient) {
    backgroundVertices.resize(static_cast<size_t>(13));
    GeometryGenerator::fillRadialGradientRectangleVertices(backgroundVertices.data(), theme.backgroundColor(),
                                                           theme.backgroundGradientColor(),
                                                           0.f, (float)width, 0.f, -(float)height);
    indices = { 0,1,3,0,3,5,  1,2,4,2,6,4,    3,1,7,1,4,7,  3,7,5,4,6,7,
                5,7,8,7,11,8, 5,8,10,8,11,10, 7,6,9,7,9,11, 6,12,9,9,12,11 };
  }
  else { // BackgroundStyle::plain
    backgroundVertices.resize(static_cast<size_t>(4));
    GeometryGenerator::fillRectangleVertices(backgroundVertices.data(), theme.backgroundColor(),
                                             0.f, (float)width, 0.f, -(float)height);
    indices = { 0,1,2,2,1,3 };
  }
  return ControlMesh(context.renderer(), std::move(backgroundVertices), indices,
                     context.pixelSizeX(), context.pixelSizeY(), x, y, width, height);
}

// ---

#define HOVER_BORDER_RADIUS 3.f

Page::Page(std::shared_ptr<RendererContext> context_, std::shared_ptr<RendererStateBuffers> buffers_,
           const ColorTheme& theme, int32_t x, int32_t y, uint32_t width, uint32_t visibleHeight, bool enableTooltip)
  : context(std::move(context_)),
    buffers(std::move(buffers_)),
    scrollY(0),
    activeControlIndex(noControlSelection()),
    backgroundType(theme.backgroundStyle()) {
  assert(this->context != nullptr && this->buffers != nullptr);

  // create page scrollbar
  const int32_t scrollBarX = x + (int32_t)width - (int32_t)Control::scrollbarWidth();
  auto scrollHandler = std::bind(&Page::onScroll, this, std::placeholders::_1);
  scrollbar = ScrollBar(*context, theme.scrollbarControlColor(), theme.scrollbarThumbColor(),
                        scrollBarX, y, Control::scrollbarWidth(), std::move(scrollHandler),
                        visibleHeight, visibleHeight, (Control::pageLineHeight() >> 1));

  // create tooltip bar
  if (enableTooltip) {
    tooltip = Tooltip(*context, U" ", FontType::inputText, LabelBufferType::regular,
                      x, y + (int32_t)visibleHeight - (int32_t)Control::tooltipBarHeight(),
                      width, Control::tooltipBarHeight(), theme.tooltipControlColor(), display::ControlIconType::none);
  }

  // create page background
  backgroundMesh = generateBackground(*context, theme, x, y, width, visibleHeight);

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

  controlHoverMesh = ControlMesh(context->renderer(), std::move(controlHoverVertices), indices, context->pixelSizeX(),
                                 context->pixelSizeY(), controlHoverX, 0, controlHoverWidth, Control::pageLineHeight());
}

Page::~Page() noexcept {
  // release controls before context
  scrollbar.release();
  tooltip.release();
  backgroundMesh.release();
  controlHoverMesh.release();
  controlRegistry.clear();

  buffers = nullptr;
  context = nullptr;
}

// ---

void Page::moveBase(int32_t x, int32_t y, uint32_t width, uint32_t visibleHeight) {
  const int32_t scrollBarX = x + (int32_t)width - (int32_t)scrollbar.width();
  scrollbar.moveControl(*context, scrollBarX, y, visibleHeight);

  if (tooltip.width()) {
    tooltip.move(*context, x, y + (int32_t)visibleHeight - (int32_t)Control::tooltipBarHeight(), width);
    if (activeControlIndex != noControlSelection())
      tooltip.updateLabel(*context, U" ", LabelBufferType::regular);
  }

  auto backgroundVertices = backgroundMesh.relativeVertices();
  if (backgroundType == BackgroundStyle::radialGradient)
    GeometryGenerator::resizeRadialGradientRectangleVertices(backgroundVertices.data(), (float)width, -(float)visibleHeight);
  else
    GeometryGenerator::resizeRectangleVertices(backgroundVertices.data(), (float)width, -(float)visibleHeight);
  backgroundMesh.update(context->renderer(), std::move(backgroundVertices), context->pixelSizeX(), context->pixelSizeY(),
                        x, y, width, visibleHeight);

  const int32_t controlHoverX = x + (int32_t)Control::fieldsetMarginX(width)
                              + (int32_t)Control::fieldsetContentMarginX(width) - Control::lineHoverPaddingX();
  controlHoverMesh.move(context->renderer(), context->pixelSizeX(), context->pixelSizeY(), controlHoverX, 0);
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
  backgroundMesh = generateBackground(*context, theme, backgroundMesh.x(), backgroundMesh.y(),
                                      backgroundMesh.width(), backgroundMesh.height());
  backgroundType = theme.backgroundStyle();

  std::vector<ControlVertex> vertices = controlHoverMesh.relativeVertices();
  for (auto& vertex : vertices)
    memcpy(vertex.color, theme.lineSelectorControlColor(), sizeof(float) * 4u);
  controlHoverMesh.update(context->renderer(), std::move(vertices), context->pixelSizeX(), context->pixelSizeY(),
                          controlHoverMesh.x(), controlHoverMesh.y(), controlHoverMesh.width(), controlHoverMesh.height());
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
        controlHoverX = control->x() - (int32_t)Control::lineHoverPaddingX();
        controlHoverWidth = Control::pageLabelWidth() + Control::pageControlWidth()
                          + (Control::lineHoverPaddingX() << 1) + Control::labelMargin();
      }
      else {
        controlHoverX = isLeftPadded ? (control->x() - (int32_t)Control::lineHoverPaddingX()) : (control->x() - 3);
        controlHoverWidth = control->width() + Control::lineHoverPaddingX() + 3;
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

    adaptControlSelection(controlIndex, control);
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

    adaptControlSelection(controlIndex, control);
  }
}

void Page::adaptControlSelection(int32_t controlIndex, ControlRegistration* control) noexcept {
  if (controlIndex != noControlSelection()) {
    if (scrollbar.isEnabled()) { // auto-scroll if needed
      const uint32_t controlTopLevel = static_cast<uint32_t>(control->y() - scrollbar.y());
      if (controlTopLevel < scrollbar.visibleTop()) {
        scrollbar.setTopPosition(*context, (controlTopLevel > Control::autoScrollPaddingY())
                                           ? controlTopLevel - Control::autoScrollPaddingY() : 0);
      }
      else {
        const uint32_t tooltipHeight = tooltip.width() ? tooltip.height() : 0;
        const uint32_t controlBottomLevel = static_cast<uint32_t>(control->bottomY() - scrollbar.y());
        if (controlBottomLevel >= scrollbar.visibleBottom() - tooltipHeight)
          scrollbar.setBottomPosition(*context, controlBottomLevel + Control::autoScrollPaddingY() + tooltipHeight);
      }
    }
    if (control->control()->type() == ControlType::textBox) { // automatic focus if text-box
      if (control->control()->click(*context, TextBox::noMouseCoord()))
        openControl = control;
    }
  }
}

// ---

void Page::mouseClick(int32_t mouseX, int32_t mouseY) {
  // click with an open control -> verify and click/close it
  if (openControl != nullptr) {
    auto status = openControl->controlStatus(mouseX, mouseY, scrollY);
    if (status == ControlStatus::hover) {
      auto controlType = openControl->control()->type();
      if (!openControl->control()->click(*context, mouseX)) { // on lang/theme change, the control will no longer exist after this call
        if (controlType == ControlType::keyBinding) {         // -> store control type before call
          const auto* target = reinterpret_cast<const KeyBinding*>(openControl->control());
          if (target->keyboardValue() != KeyBinding::emptyKeyValue())
            resolveKeyboardBindings(target);
        }
        openControl = nullptr;
      }
    }
    else { // clicked elsewhere -> close open control (and don't click on any other control)
      openControl->control()->close();
      openControl = nullptr;
      mouseMove(mouseX, mouseY);
    }
  }
  // click on scrollbar
  else if (mouseX >= scrollbar.x()) {
    if (scrollbar.isEnabled() && scrollbar.isHover(mouseX, mouseY))
      scrollbar.click(*context, mouseY, true);
  }
  // click on key binding -> open to redirect key/pad inputs
  else {
    int32_t controlIndex = findActiveControlIndex(mouseX, mouseY);
    if (controlIndex != noControlSelection()) {
      auto* activeControl = &controlRegistry[controlIndex];
      if (activeControl->control()->click(*context, mouseX))
        openControl = activeControl;
    }
  }
}

void Page::mouseButton(int32_t mouseX, int32_t mouseY, MouseButton button) {
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
  this->mouseX_ = mouseX;
  this->mouseY_ = mouseY;

  // moving with an open control -> update it
  if (openControl != nullptr) {
    auto status = openControl->controlStatus(mouseX, mouseY, scrollY);
    if (status == ControlStatus::hover)
      openControl->control()->mouseMove(*context, mouseX, openControl->isFixed() ? mouseY : (mouseY + scrollY));
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
  if (openControl != nullptr && openControl->control()->type() == ControlType::textBox) {
    auto control = reinterpret_cast<TextBox*>(openControl->control());
    if (control->isEditMode()) {
      if ((keyCode > U'\x1F' && keyCode < U'\x7F') || keyCode > U'\x9F')
        control->addChar(*context, keyCode);
    }
  }
}

void Page::vkeyDown(uint32_t virtualKeyCode) {
  if (openControl != nullptr) {
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
      auto* target = reinterpret_cast<ComboBox*>(openControl->control());
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
    else if (controlType == ControlType::keyBinding) {
      auto* target = reinterpret_cast<KeyBinding*>(openControl->control());
      if (!target->setKeyboardValue(*context, virtualKeyCode)) {
        if (target->keyboardValue() != KeyBinding::emptyKeyValue())
          resolveKeyboardBindings(target);
        openControl = nullptr;
      }
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
          if (controlType == ControlType::button || controlType == ControlType::checkBox
           || controlType == ControlType::comboBox || controlType == ControlType::keyBinding) {
            if (control->control()->click(*context, control->rightX() - control->height() - 10))
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
          auto controlType = control->control()->type();
          if (controlType == ControlType::ruler)
            reinterpret_cast<Ruler*>(control->control())->selectPrevious(*context);
          else if (controlType == ControlType::slider)
            reinterpret_cast<Slider*>(control->control())->selectPrevious();
        }
        break;
      case _P_VK_ARROW_RIGHT:
        if (activeControlIndex != noControlSelection()) {
          auto* control = &controlRegistry[activeControlIndex];
          auto controlType = control->control()->type();
          if (controlType == ControlType::ruler)
            reinterpret_cast<Ruler*>(control->control())->selectNext(*context);
          else if (controlType == ControlType::slider)
            reinterpret_cast<Slider*>(control->control())->selectNext();
        }
        break;
      case _P_VK_DELETE:
      case _P_VK_BACKSPACE:
        if (activeControlIndex != noControlSelection()) {
          auto* control = &controlRegistry[activeControlIndex];
          if (control->control()->type() == ControlType::keyBinding) { // clear binding
            reinterpret_cast<KeyBinding*>(control->control())->setKeyboardValue(*context, KeyBinding::emptyKeyValue());
            reinterpret_cast<KeyBinding*>(control->control())->setControllerValue(*context, KeyBinding::emptyKeyValue());
          }
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
  // fixed geometry
  auto& renderer = context->renderer();
  if (!buffers->isFixedLocationBuffer())
    buffers->bindFixedLocationBuffer(renderer, ScissorRectangle(0, 0, context->clientWidth(), context->clientHeight()));

  buffers->bindControlBuffer(renderer, ControlBufferType::regular);
  backgroundMesh.draw(renderer);
  if (tooltip.width())
    tooltip.drawBackground(*context, *buffers);
  scrollbar.drawControl(*context, mouseX_, mouseY_, *buffers);

  // scrollable geometry
  if (activeControlIndex != noControlSelection()) {
    buffers->bindScrollLocationBuffer(renderer, ScissorRectangle(backgroundMesh.x(), backgroundMesh.y(),
                                                                 backgroundMesh.width(), contentHeight()));
    buffers->bindControlBuffer(renderer, ControlBufferType::regular);
    controlHoverMesh.draw(renderer);
  }

  bool hasForegroundGeometry = drawPageBackgrounds(mouseX_, mouseY_);
  return (hasForegroundGeometry || (openControl != nullptr && openControl->control()->type() == ControlType::comboBox));
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