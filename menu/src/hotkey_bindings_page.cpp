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
#include "menu/controls/geometry_generator.h"
#include "menu/hotkey_bindings_page.h"

using namespace video_api;
using namespace display;
using namespace display::controls;
using namespace menu::controls;
using namespace menu;


// -- page -- ------------------------------------------------------------------

void HotkeyBindingsPage::init(const ColorTheme& theme, const MessageResources& localizedText,
                              int32_t x, int32_t y, uint32_t width) {
  const MessageResource* textResources = localizedText.hotkeyBindingsMessageArray();
  const uint32_t fieldsetPaddingX = Control::fieldsetMarginX(width);
  const int32_t controlX = x + (int32_t)fieldsetPaddingX + (int32_t)Control::fieldsetContentMarginX(width);
  uint32_t fieldsetWidth = width - (fieldsetPaddingX << 1);
  if (fieldsetWidth > Control::fieldsetMaxWidth())
    fieldsetWidth = Control::fieldsetMaxWidth();

  title = TextMesh(context->renderer(), context->getFont(FontType::titles), GET_UI_MESSAGE(textResources,HotkeyBindingsMessages::title),
                   context->pixelSizeX(), context->pixelSizeY(), x + (int32_t)fieldsetPaddingX, y + 24, TextAlignment::left);

  std::vector<ControlRegistration> registry;
  registry.reserve(10);
  int32_t currentLineY = title.y() + (int32_t)title.height() + Control::pageLineHeight();

  // --- hotkey/hint settings group ---
  behaviorGroup = Fieldset(*context, GET_UI_MESSAGE(textResources,HotkeyBindingsMessages::behaviorGroup), theme.fieldsetStyle(),
                           theme.fieldsetControlColor(), x + (int32_t)fieldsetPaddingX, currentLineY, fieldsetWidth,
                           Control::fieldsetContentHeight(5));
  currentLineY += Control::pageLineHeight() + Control::fieldsetContentPaddingY();

  menuHintMouseMove = CheckBox(*context, GET_UI_MESSAGE(textResources,HotkeyBindingsMessages::menuHintMouseMove), controlX,
                               currentLineY, Control::pageLabelWidth(), 0, nullptr, showMenuHint);
  showMenuHint = true;
  registry.emplace_back(menuHintMouseMove, true, GET_UI_MESSAGE(textResources,HotkeyBindingsMessages::menuHintMouseMove_tooltip));
  currentLineY += Control::pageLineHeight();

  enableKeyBindings = CheckBox(*context, GET_UI_MESSAGE(textResources,HotkeyBindingsMessages::enableKeyBindings), controlX,
                               currentLineY, Control::pageLabelWidth(), 0, nullptr, isBindingEnabled);
  isBindingEnabled = true;
  registry.emplace_back(enableKeyBindings, true, GET_UI_MESSAGE(textResources,HotkeyBindingsMessages::enableKeyBindings_tooltip));
  currentLineY += Control::pageLineHeight();
  
  hotkeyBinding = KeyBinding(*context, GET_UI_MESSAGE(textResources,HotkeyBindingsMessages::hotkey), controlX, currentLineY,
                             Control::pageLabelWidth(), (Control::pageControlWidth() >> 1) + 1, theme.textBoxControlColor(),
                             theme.keyboardKeyColorParams(), KeyBindingType::controller, KeyBinding::emptyKeyValue(),
                             0x20u/*XINPUT_GAMEPAD_BACK*/, false, &isBindingEnabled);
  registry.emplace_back(hotkeyBinding, true, GET_UI_MESSAGE(textResources,HotkeyBindingsMessages::hotkey_tooltip));
  currentLineY += Control::pageLineHeight();

  ComboBoxOption speedCommandOptions[]{ ComboBoxOption(GET_UI_MESSAGE(textResources,HotkeyBindingsMessages::hold),  0/*TMP*/),
                                        ComboBoxOption(GET_UI_MESSAGE(textResources,HotkeyBindingsMessages::toggle), 1/*TMP*/) };
  slowMotionMode = Slider(*context, GET_UI_MESSAGE(textResources,HotkeyBindingsMessages::slowMotionMode), controlX, currentLineY,
                          Control::pageLabelWidth(), Control::pageControlWidth(), theme.sliderArrowColor(), 0,
                          nullptr, speedCommandOptions, sizeof(speedCommandOptions)/sizeof(*speedCommandOptions), 0, &isBindingEnabled);
  registry.emplace_back(slowMotionMode, true, GET_UI_MESSAGE(textResources,HotkeyBindingsMessages::slowMotionMode_tooltip));
  currentLineY += Control::pageLineHeight();

  fastForwardMode = Slider(*context, GET_UI_MESSAGE(textResources,HotkeyBindingsMessages::fastForwardMode), controlX, currentLineY,
                            Control::pageLabelWidth(), Control::pageControlWidth(), theme.sliderArrowColor(), 0,
                            nullptr, speedCommandOptions, sizeof(speedCommandOptions)/sizeof(*speedCommandOptions), 0, &isBindingEnabled);
  registry.emplace_back(fastForwardMode, true, GET_UI_MESSAGE(textResources,HotkeyBindingsMessages::fastForwardMode_tooltip));
  currentLineY += Control::pageLineHeight() + Control::fieldsetContentBottomMargin();

  // --- bindings group ---
  const MessageResource* actionTextResources = localizedText.hotkeyActionsMessageArray();
  const MessageResource* actionTooltipResources = localizedText.hotkeyActionsTooltipsMessageArray();
  bindingsGroup = Fieldset(*context, GET_UI_MESSAGE(textResources,HotkeyBindingsMessages::bindingsGroup), theme.fieldsetStyle(),
                           theme.fieldsetControlColor(), x + (int32_t)fieldsetPaddingX, currentLineY, fieldsetWidth,
                           Control::fieldsetContentHeight((uint32_t)HotkeyActions::COUNT) + (Control::pageLineHeight() >> 1)*3u);
  currentLineY += Control::pageLineHeight() + Control::fieldsetContentPaddingY();


  for (size_t i = 0; i < (size_t)HotkeyActions::COUNT; ++i) {
    if (i == (size_t)HotkeyActions::nextSaveSlot || i == (size_t)HotkeyActions::pauseResume || i == (size_t)HotkeyActions::screenshot)
      currentLineY += (int32_t)(Control::pageLineHeight() >> 1); // category separation space

    bindings[i] = KeyBinding(*context, GET_UI_MESSAGE(actionTextResources,i), controlX, currentLineY,
                             Control::pageLabelWidth(), Control::pageControlWidth(), theme.textBoxControlColor(),
                             theme.keyboardKeyColorParams(), KeyBindingType::both, KeyBinding::emptyKeyValue(),
                             KeyBinding::emptyKeyValue(), true, &isBindingEnabled);
    registry.emplace_back(bindings[i], true, GET_UI_MESSAGE(actionTooltipResources,i));
    currentLineY += Control::pageLineHeight();
  }
  //currentLineY += Control::fieldsetContentBottomMargin();

  // --- control registry ---
  if (currentLineY > y + (int32_t)contentHeight())
    Page::moveScrollbarThumb(currentLineY);
  registerControls(std::move(registry));
}

HotkeyBindingsPage::~HotkeyBindingsPage() noexcept {
  title.release();

  behaviorGroup.release();
  menuHintMouseMove.release();
  enableKeyBindings.release();
  hotkeyBinding.release();
  slowMotionMode.release();
  fastForwardMode.release();

  bindingsGroup.release();
  for (size_t i = 0; i < (size_t)HotkeyActions::COUNT; ++i)
    bindings[i].release();
}


// -- window events -- ---------------------------------------------------------

void HotkeyBindingsPage::move(int32_t x, int32_t y, uint32_t width, uint32_t height) {
  Page::moveBase(x, y, width, height);
  const uint32_t fieldsetPaddingX = Control::fieldsetMarginX(width);
  const int32_t controlX = x + (int32_t)fieldsetPaddingX + (int32_t)Control::fieldsetContentMarginX(width);
  uint32_t fieldsetWidth = width - (fieldsetPaddingX << 1);
  if (fieldsetWidth > Control::fieldsetMaxWidth())
    fieldsetWidth = Control::fieldsetMaxWidth();

  title.move(context->renderer(), context->pixelSizeX(), context->pixelSizeY(), x + (int32_t)fieldsetPaddingX, y + 24);

  // hotkey/hint group
  int32_t currentLineY = title.y() + (int32_t)title.height() + Control::pageLineHeight();
  behaviorGroup.move(*context, x + (int32_t)fieldsetPaddingX, currentLineY, fieldsetWidth,
                     Control::fieldsetContentHeight(5));
  currentLineY += Control::pageLineHeight() + Control::fieldsetContentPaddingY();

  menuHintMouseMove.move(*context, controlX, currentLineY);
  currentLineY += Control::pageLineHeight();
  enableKeyBindings.move(*context, controlX, currentLineY);
  currentLineY += Control::pageLineHeight();
  hotkeyBinding.move(*context, controlX, currentLineY);
  currentLineY += Control::pageLineHeight();
  slowMotionMode.move(*context, controlX, currentLineY);
  currentLineY += Control::pageLineHeight();
  fastForwardMode.move(*context, controlX, currentLineY);
  currentLineY += Control::pageLineHeight() + Control::fieldsetContentBottomMargin();

  // bindings group
  bindingsGroup.move(*context, x + (int32_t)fieldsetPaddingX, currentLineY, fieldsetWidth,
                     Control::fieldsetContentHeight((uint32_t)HotkeyActions::COUNT) + (Control::pageLineHeight() >> 1)*3u);
  currentLineY += Control::pageLineHeight() + Control::fieldsetContentPaddingY();

  for (size_t i = 0; i < (size_t)HotkeyActions::COUNT; ++i) {
    if (i == (size_t)HotkeyActions::nextSaveSlot || i == (size_t)HotkeyActions::pauseResume || i == (size_t)HotkeyActions::screenshot)
      currentLineY += (int32_t)(Control::pageLineHeight() >> 1); // category separation space

    bindings[i].move(*context, controlX, currentLineY);
    currentLineY += Control::pageLineHeight();
  }
  //currentLineY += Control::fieldsetContentBottomMargin();

  Page::moveScrollbarThumb(currentLineY); // required after a move
}

void HotkeyBindingsPage::resolveKeyboardBindings(const controls::KeyBinding* updatedControl) noexcept {
  // search for conflicts
  for (size_t i = 0; i < (size_t)HotkeyActions::COUNT; ++i) {
    auto& current = bindings[i];
    if (&current != updatedControl && current.keyboardValue() == updatedControl->keyboardValue()) {
      // reset older conflicted value
      current.setKeyboardValue(*context, KeyBinding::emptyKeyValue());
      break; // always verified -> max 1 conflict
    }
  }
}


// -- rendering -- -------------------------------------------------------------

void HotkeyBindingsPage::drawIcons() {
  // scrollable geometry
  buffers->bindScrollLocationBuffer(context->renderer(), ScissorRectangle(x(), y(), width(), contentHeight()));

  auto* hoverControl = getActiveControl();
  menuHintMouseMove.drawIcon(*context, *buffers, (hoverControl == &menuHintMouseMove));
  enableKeyBindings.drawIcon(*context, *buffers, (hoverControl == &enableKeyBindings));

  hotkeyBinding.drawIcon(*context, *buffers);
  for (size_t i = 0; i < (size_t)HotkeyActions::COUNT; ++i)
    bindings[i].drawIcon(*context, *buffers);
}

bool HotkeyBindingsPage::drawPageBackgrounds(int32_t mouseX, int32_t) {
  // scrollable geometry
  if (buffers->isFixedLocationBuffer())
    buffers->bindScrollLocationBuffer(context->renderer(), ScissorRectangle(x(), y(), width(), contentHeight()));

  auto* hoverControl = getActiveControl();
  behaviorGroup.drawBackground(*context, *buffers);
  bindingsGroup.drawBackground(*context, *buffers);

  slowMotionMode.drawBackground(*context, mouseX, *buffers, (hoverControl == &slowMotionMode));
  fastForwardMode.drawBackground(*context, mouseX, *buffers, (hoverControl == &fastForwardMode));

  hotkeyBinding.drawBackground(*context, mouseX, *buffers, (hoverControl == &hotkeyBinding));
  for (size_t i = 0; i < (size_t)HotkeyActions::COUNT; ++i)
    bindings[i].drawBackground(*context, mouseX, *buffers, (hoverControl == &bindings[i]));
  return false;
}

void HotkeyBindingsPage::drawPageLabels() {
  // scrollable geometry
  auto& renderer = context->renderer();
  if (buffers->isFixedLocationBuffer())
    buffers->bindScrollLocationBuffer(renderer, ScissorRectangle(x(), y(), width(), contentHeight()));

  buffers->bindLabelBuffer(renderer, LabelBufferType::title);
  title.draw(renderer);

  auto* hoverControl = getActiveControl();
  behaviorGroup.drawLabel(*context, *buffers);
  bindingsGroup.drawLabel(*context, *buffers);

  menuHintMouseMove.drawLabel(*context, *buffers, (hoverControl == &menuHintMouseMove));
  enableKeyBindings.drawLabel(*context, *buffers, (hoverControl == &enableKeyBindings));

  slowMotionMode.drawLabels(*context, *buffers, (hoverControl == &slowMotionMode));
  fastForwardMode.drawLabels(*context, *buffers, (hoverControl == &fastForwardMode));

  hotkeyBinding.drawLabels(*context, *buffers, (hoverControl == &hotkeyBinding));
  for (size_t i = 0; i < (size_t)HotkeyActions::COUNT; ++i)
    bindings[i].drawLabels(*context, *buffers, (hoverControl == &bindings[i]));
}
