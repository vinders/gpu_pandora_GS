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
#include "menu/pages/page_content_builder.h"
#include "menu/pages/compatibility_settings.h"

using namespace video_api;
using namespace display;
using namespace display::controls;
using namespace menu::controls;
using namespace menu::pages;
using namespace menu;


// -- page -- ------------------------------------------------------------------

void CompatibilitySettings::init(const ColorTheme& theme, const MessageResources& localizedText, int32_t x, int32_t y, uint32_t width) {
  PageContentBuilder builder(*context, theme, x, y, width, 3);

  builder.addTitle(localizedText.getMessage(CompatibilitySettingsMessages::title), title);

  // special game fixes group
  builder.addFieldset(localizedText.getMessage(CompatibilitySettingsMessages::specialGameFixesGroup), 3, 0, specialGameFixesGroup);
  lowCompatibilityFrameReadValue = ignoreSmallFramebufferMovesValue = fakeGpuBusyStatesValue = false;

  builder.addCheckBox(0, localizedText.getMessage(CompatibilitySettingsMessages::lowCompatibilityFrameRead),
                      localizedText.getMessage(CompatibilitySettingsMessages::lowCompatibilityFrameRead_tooltip),
                      lowCompatibilityFrameReadValue, lowCompatibilityFrameRead);
  builder.addCheckBox(0, localizedText.getMessage(CompatibilitySettingsMessages::ignoreSmallFramebufferMoves),
                      localizedText.getMessage(CompatibilitySettingsMessages::ignoreSmallFramebufferMoves_tooltip),
                      ignoreSmallFramebufferMovesValue, ignoreSmallFramebufferMoves);
  builder.addCheckBox(0, localizedText.getMessage(CompatibilitySettingsMessages::fakeGpuBusyStates),
                      localizedText.getMessage(CompatibilitySettingsMessages::fakeGpuBusyStates_tooltip),
                      fakeGpuBusyStatesValue, fakeGpuBusyStates);

  // control registry
  Page::moveScrollbarThumb(builder.linePositionY());
  registerControls(std::move(builder.controlRegistry()));
}

CompatibilitySettings::~CompatibilitySettings() noexcept {
  title.release();

  specialGameFixesGroup.release();
  lowCompatibilityFrameRead.release();
  ignoreSmallFramebufferMoves.release();
  fakeGpuBusyStates.release();
}


// -- window events -- ---------------------------------------------------------

void CompatibilitySettings::move(int32_t x, int32_t y, uint32_t width, uint32_t height) {
  Page::moveBase(x, y, width, height);
  PageContentMover mover(*context, x, y, width);

  mover.moveTitle(title);

  // special game fixes group
  mover.moveFieldset(3, 0, specialGameFixesGroup);

  mover.moveCheckBox(lowCompatibilityFrameRead);
  mover.moveCheckBox(ignoreSmallFramebufferMoves);
  mover.moveCheckBox(fakeGpuBusyStates);

  Page::moveScrollbarThumb(mover.linePositionY()); // required after a move
}


// -- rendering -- -------------------------------------------------------------

void CompatibilitySettings::drawIcons() {
  // scrollable geometry
  buffers->bindScrollLocationBuffer(context->renderer(), ScissorRectangle(x(), y(), width(), contentHeight()));

  auto* hoverControl = getActiveControl();
  lowCompatibilityFrameRead.drawIcon(*context, *buffers, (hoverControl == &lowCompatibilityFrameRead));
  ignoreSmallFramebufferMoves.drawIcon(*context, *buffers, (hoverControl == &ignoreSmallFramebufferMoves));
  fakeGpuBusyStates.drawIcon(*context, *buffers, (hoverControl == &fakeGpuBusyStates));
}

void CompatibilitySettings::drawPageBackgrounds(int32_t, int32_t) {
  // scrollable geometry
  if (buffers->isFixedLocationBuffer())
    buffers->bindScrollLocationBuffer(context->renderer(), ScissorRectangle(x(), y(), width(), contentHeight()));

  specialGameFixesGroup.drawBackground(*context, *buffers);
}

void CompatibilitySettings::drawPageLabels() {
  // scrollable geometry
  auto& renderer = context->renderer();
  if (buffers->isFixedLocationBuffer())
    buffers->bindScrollLocationBuffer(renderer, ScissorRectangle(x(), y(), width(), contentHeight()));

  buffers->bindLabelBuffer(renderer, LabelBufferType::title);
  title.draw(renderer);

  specialGameFixesGroup.drawLabel(*context, *buffers);

  auto* hoverControl = getActiveControl();
  lowCompatibilityFrameRead.drawLabel(*context, *buffers, (hoverControl == &lowCompatibilityFrameRead));
  ignoreSmallFramebufferMoves.drawLabel(*context, *buffers, (hoverControl == &ignoreSmallFramebufferMoves));
  fakeGpuBusyStates.drawLabel(*context, *buffers, (hoverControl == &fakeGpuBusyStates));
}
