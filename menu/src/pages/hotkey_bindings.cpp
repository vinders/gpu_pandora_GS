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
#include "menu/pages/page_content_builder.h"
#include "menu/pages/hotkey_bindings.h"

using namespace video_api;
using namespace display;
using namespace display::controls;
using namespace menu::controls;
using namespace menu::pages;
using namespace menu;


// -- page -- ------------------------------------------------------------------

void HotkeyBindings::init(const ColorTheme& theme, const MessageResources& localizedText,
                          int32_t x, int32_t y, uint32_t width) {
  PageContentBuilder builder(*context, theme, x, y, width, (size_t)HotkeyActions::COUNT + 5u);

  builder.addTitle(localizedText.getMessage(HotkeyBindingsMessages::title), title);

  // hotkey/hint settings group
  builder.addFieldset(localizedText.getMessage(HotkeyBindingsMessages::behaviorGroup), 5, 0, behaviorGroup);
  showMenuHint = isBindingEnabled = true;

  builder.addCheckBox(0, localizedText.getMessage(HotkeyBindingsMessages::menuHintMouseMove),
                      localizedText.getMessage(HotkeyBindingsMessages::menuHintMouseMove_tooltip),
                      showMenuHint, menuHintMouseMove);
  builder.addCheckBox(0, localizedText.getMessage(HotkeyBindingsMessages::enableKeyBindings),
                      localizedText.getMessage(HotkeyBindingsMessages::enableKeyBindings_tooltip),
                      isBindingEnabled, enableKeyBindings);
  builder.setEnabler(isBindingEnabled);

  constexpr const uint32_t hotkeySettingsWidth = Control::pageControlWidth()*2u/3u;
  builder.addControllerKeyBinding(localizedText.getMessage(HotkeyBindingsMessages::hotkey),
                                  localizedText.getMessage(HotkeyBindingsMessages::hotkey_tooltip),
                                  hotkeySettingsWidth, 0x40u/*XINPUT_GAMEPAD_LEFT_THUMB*/, hotkeyBinding);

  ComboBoxOption speedCommandOptions[]{ ComboBoxOption(localizedText.getMessage(HotkeyBindingsMessages::hold),  0/*TMP*/),
                                        ComboBoxOption(localizedText.getMessage(HotkeyBindingsMessages::toggle), 1/*TMP*/) };
  builder.addSlider(0, localizedText.getMessage(HotkeyBindingsMessages::slowMotionMode),
                    localizedText.getMessage(HotkeyBindingsMessages::slowMotionMode_tooltip), hotkeySettingsWidth,
                    speedCommandOptions, sizeof(speedCommandOptions)/sizeof(*speedCommandOptions), 0, slowMotionMode);
  builder.addSlider(0, localizedText.getMessage(HotkeyBindingsMessages::fastForwardMode),
                    localizedText.getMessage(HotkeyBindingsMessages::fastForwardMode_tooltip), hotkeySettingsWidth,
                    speedCommandOptions, sizeof(speedCommandOptions)/sizeof(*speedCommandOptions), 0, fastForwardMode);

  // bindings group
  constexpr const uint32_t categorySeparationHeight = (Control::pageLineHeight() >> 1);
  builder.addFieldset(localizedText.getMessage(HotkeyBindingsMessages::bindingsGroup), (uint32_t)HotkeyActions::COUNT,
                      categorySeparationHeight*3u, bindingsGroup);

  for (size_t i = 0; i < (size_t)HotkeyActions::COUNT; ++i) {
    if (i == (size_t)HotkeyActions::nextSaveSlot || i == (size_t)HotkeyActions::pauseResume || i == (size_t)HotkeyActions::screenshot)
      builder.addLineOffset((int32_t)categorySeparationHeight);

    builder.addDualKeyBinding(localizedText.getMessage((HotkeyActions)i), localizedText.getMessageTooltip((HotkeyActions)i),
                              KeyBinding::emptyKeyValue(), KeyBinding::emptyKeyValue(), bindings[i]);
  }

  // control registry
  Page::moveScrollbarThumb(builder.linePositionY());
  registerControls(std::move(builder.controlRegistry()));
}

HotkeyBindings::~HotkeyBindings() noexcept {
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

void HotkeyBindings::move(int32_t x, int32_t y, uint32_t width, uint32_t height) {
  Page::moveBase(x, y, width, height);
  PageContentMover mover(*context, x, y, width);
  const int32_t offsetX = mover.linePositionX() - menuHintMouseMove.x();

  mover.moveTitle(title);

  // hotkey/hint group
  mover.moveFieldset(behaviorGroup);

  mover.moveCheckBox(menuHintMouseMove);
  mover.moveCheckBox(enableKeyBindings);
  mover.moveKeyBinding(hotkeyBinding);
  mover.moveSlider(slowMotionMode);
  mover.moveSlider(fastForwardMode);

  // bindings group
  constexpr const uint32_t categorySeparationHeight = (Control::pageLineHeight() >> 1);
  mover.moveFieldset(bindingsGroup);

  for (size_t i = 0; i < (size_t)HotkeyActions::COUNT; ++i) {
    if (i == (size_t)HotkeyActions::nextSaveSlot || i == (size_t)HotkeyActions::pauseResume || i == (size_t)HotkeyActions::screenshot)
      mover.addLineOffset((int32_t)categorySeparationHeight);
    mover.moveKeyBinding(bindings[i]);
  }

  Page::moveScrollbarThumb(mover.linePositionY()); // required after a move
  Page::moveRegisteredControls(offsetX);
}

void HotkeyBindings::resolveKeyboardBindings(const controls::KeyBinding* updatedControl) noexcept {
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

void HotkeyBindings::drawIcons() {
  // scrollable geometry
  buffers->bindScrollLocationBuffer(context->renderer(), ScissorRectangle(x(), y(), width(), contentHeight()));

  auto* hoverControl = getActiveControl();
  menuHintMouseMove.drawIcon(*context, *buffers, (hoverControl == &menuHintMouseMove));
  enableKeyBindings.drawIcon(*context, *buffers, (hoverControl == &enableKeyBindings));

  hotkeyBinding.drawIcon(*context, *buffers);
  for (size_t i = 0; i < (size_t)HotkeyActions::COUNT; ++i)
    bindings[i].drawIcon(*context, *buffers);
}

void HotkeyBindings::drawPageBackgrounds(int32_t mouseX, int32_t) {
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
}

void HotkeyBindings::drawPageLabels() {
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
