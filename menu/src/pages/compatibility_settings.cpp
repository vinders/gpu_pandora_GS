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
#include "menu/pages/compatibility_settings.h"

using namespace video_api;
using namespace display;
using namespace display::controls;
using namespace menu::controls;
using namespace menu::pages;
using namespace menu;


// -- page -- ------------------------------------------------------------------

void CompatibilitySettings::init(const ColorTheme& theme, const MessageResources& localizedText, int32_t x, int32_t y, uint32_t width) {
  const MessageResource* textResources = localizedText.compatibilityMessageArray();
  const uint32_t fieldsetPaddingX = Control::fieldsetMarginX(width);
  const int32_t controlX = x + (int32_t)fieldsetPaddingX + (int32_t)Control::fieldsetContentMarginX(width);
  uint32_t fieldsetWidth = width - (fieldsetPaddingX << 1);
  if (fieldsetWidth > Control::fieldsetMaxWidth())
    fieldsetWidth = Control::fieldsetMaxWidth();

  title = TextMesh(context->renderer(), context->getFont(FontType::titles), GET_UI_MESSAGE(textResources,CompatibilitySettingsMessages::title),
                   context->pixelSizeX(), context->pixelSizeY(), x + (int32_t)fieldsetPaddingX, y + 24, TextAlignment::left);

  std::vector<ControlRegistration> registry;
  registry.reserve(3);
  int32_t currentLineY = title.y() + (int32_t)title.height() + Control::pageLineHeight();

  // --- special game fixes group ---
  specialGameFixesGroup = Fieldset(*context, GET_UI_MESSAGE(textResources,CompatibilitySettingsMessages::specialGameFixesGroup),
                                   theme.fieldsetStyle(), theme.fieldsetControlColor(), x + (int32_t)fieldsetPaddingX,
                                   currentLineY, fieldsetWidth, Control::fieldsetContentHeight(3));
  currentLineY += Control::pageLineHeight() + Control::fieldsetContentPaddingTop();

  lowCompatibilityFrameRead = CheckBox(*context, GET_UI_MESSAGE(textResources,CompatibilitySettingsMessages::lowCompatibilityFrameRead),
                                       controlX, currentLineY, Control::pageLabelWidth(), 0, nullptr, lowCompatibilityFrameReadValue);
  lowCompatibilityFrameReadValue = false;
  registry.emplace_back(lowCompatibilityFrameRead, true,
                        GET_UI_MESSAGE(textResources,CompatibilitySettingsMessages::lowCompatibilityFrameRead_tooltip));
  currentLineY += Control::pageLineHeight();

  ignoreSmallFrameBufferMoves = CheckBox(*context, GET_UI_MESSAGE(textResources,CompatibilitySettingsMessages::ignoreSmallFrameBufferMoves),
                                         controlX, currentLineY, Control::pageLabelWidth(), 0, nullptr, ignoreSmallFrameBufferMovesValue);
  ignoreSmallFrameBufferMovesValue = false;
  registry.emplace_back(ignoreSmallFrameBufferMoves, true,
                        GET_UI_MESSAGE(textResources,CompatibilitySettingsMessages::ignoreSmallFrameBufferMoves_tooltip));
  currentLineY += Control::pageLineHeight();

  fakeGpuBusyStates = CheckBox(*context, GET_UI_MESSAGE(textResources,CompatibilitySettingsMessages::fakeGpuBusyStates),
                               controlX, currentLineY, Control::pageLabelWidth(), 0, nullptr, fakeGpuBusyStatesValue);
  fakeGpuBusyStatesValue = false;
  registry.emplace_back(fakeGpuBusyStates, true, GET_UI_MESSAGE(textResources,CompatibilitySettingsMessages::fakeGpuBusyStates_tooltip));
  currentLineY += Control::pageLineHeight();// + Control::fieldsetContentMarginBottom();

  // --- control registry ---
  if (currentLineY > y + (int32_t)contentHeight())
    Page::moveScrollbarThumb(currentLineY);
  registerControls(std::move(registry));
}

CompatibilitySettings::~CompatibilitySettings() noexcept {
  title.release();

  specialGameFixesGroup.release();
  lowCompatibilityFrameRead.release();
  ignoreSmallFrameBufferMoves.release();
  fakeGpuBusyStates.release();
}


// -- window events -- ---------------------------------------------------------

void CompatibilitySettings::move(int32_t x, int32_t y, uint32_t width, uint32_t height) {
  Page::moveBase(x, y, width, height);
  const uint32_t fieldsetPaddingX = Control::fieldsetMarginX(width);
  const int32_t controlX = x + (int32_t)fieldsetPaddingX + (int32_t)Control::fieldsetContentMarginX(width);
  uint32_t fieldsetWidth = width - (fieldsetPaddingX << 1);
  if (fieldsetWidth > Control::fieldsetMaxWidth())
    fieldsetWidth = Control::fieldsetMaxWidth();

  title.move(context->renderer(), context->pixelSizeX(), context->pixelSizeY(), x + (int32_t)fieldsetPaddingX, y + 24);

  // aspect ratio group
  int32_t currentLineY = title.y() + (int32_t)title.height() + Control::pageLineHeight();
  specialGameFixesGroup.move(*context, x + (int32_t)fieldsetPaddingX, currentLineY, fieldsetWidth, Control::fieldsetContentHeight(3));
  currentLineY += Control::pageLineHeight() + Control::fieldsetContentPaddingTop();

  lowCompatibilityFrameRead.move(*context, controlX, currentLineY);
  currentLineY += Control::pageLineHeight();
  ignoreSmallFrameBufferMoves.move(*context, controlX, currentLineY);
  currentLineY += Control::pageLineHeight();
  fakeGpuBusyStates.move(*context, controlX, currentLineY);
  currentLineY += Control::pageLineHeight();// + Control::fieldsetContentMarginBottom();

  Page::moveScrollbarThumb(currentLineY); // required after a move
}


// -- rendering -- -------------------------------------------------------------

void CompatibilitySettings::drawIcons() {
  // scrollable geometry
  buffers->bindScrollLocationBuffer(context->renderer(), ScissorRectangle(x(), y(), width(), contentHeight()));

  auto* hoverControl = getActiveControl();
  lowCompatibilityFrameRead.drawIcon(*context, *buffers, (hoverControl == &lowCompatibilityFrameRead));
  ignoreSmallFrameBufferMoves.drawIcon(*context, *buffers, (hoverControl == &ignoreSmallFrameBufferMoves));
  fakeGpuBusyStates.drawIcon(*context, *buffers, (hoverControl == &fakeGpuBusyStates));
}

bool CompatibilitySettings::drawPageBackgrounds(int32_t, int32_t) {
  // scrollable geometry
  if (buffers->isFixedLocationBuffer())
    buffers->bindScrollLocationBuffer(context->renderer(), ScissorRectangle(x(), y(), width(), contentHeight()));

  specialGameFixesGroup.drawBackground(*context, *buffers);
  return false;
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
  ignoreSmallFrameBufferMoves.drawLabel(*context, *buffers, (hoverControl == &ignoreSmallFrameBufferMoves));
  fakeGpuBusyStates.drawLabel(*context, *buffers, (hoverControl == &fakeGpuBusyStates));
}
