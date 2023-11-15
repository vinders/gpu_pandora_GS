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
#include "menu/pages/osd_settings.h"

using namespace video_api;
using namespace display;
using namespace display::controls;
using namespace menu::controls;
using namespace menu::pages;
using namespace menu;


// -- page -- ------------------------------------------------------------------

#define CLOCK_VISIBILITY_ID     1

void OsdSettings::init(const ColorTheme& theme, const MessageResources& localizedText, int32_t x, int32_t y, uint32_t width) {
  const uint32_t fieldsetPaddingX = Control::fieldsetMarginX(width);
  const int32_t controlX = x + (int32_t)fieldsetPaddingX + (int32_t)Control::fieldsetContentMarginX(width);
  uint32_t fieldsetWidth = width - (fieldsetPaddingX << 1);
  if (fieldsetWidth > Control::fieldsetMaxWidth())
    fieldsetWidth = Control::fieldsetMaxWidth();

  title = TextMesh(context->renderer(), context->getFont(FontType::titles), localizedText.getMessage(OsdSettingsMessages::title),
                   context->pixelSizeX(), context->pixelSizeY(), x + (int32_t)fieldsetPaddingX, y + Control::titleMarginTop(), TextAlignment::left);

  std::vector<ControlRegistration> registry;
  registry.reserve(6);
  int32_t currentLineY = title.y() + (int32_t)title.height() + Control::pageLineHeight();
  auto changeHandler = std::bind(&OsdSettings::onChange,this,std::placeholders::_1,std::placeholders::_2);

  // --- clock group ---
  clockGroup = Fieldset(*context, localizedText.getMessage(OsdSettingsMessages::clockGroup), theme.fieldsetStyle(),
                        theme.fieldsetControlColor(), x + (int32_t)fieldsetPaddingX, currentLineY, fieldsetWidth,
                        Control::fieldsetContentHeight(4));
  currentLineY += Control::pageLineHeight() + Control::fieldsetContentPaddingTop();

  // clock visibility
  {
    ComboBoxOption visibilityOptions[]{ ComboBoxOption(localizedText.getMessage(CommonMessages::disabled), 0/*TMP*/),
                                        ComboBoxOption(localizedText.getMessage(OsdSettingsMessages::clockVisibility_mouseMove), 1/*TMP*/),
                                        ComboBoxOption(localizedText.getMessage(OsdSettingsMessages::clockVisibility_always), 2/*TMP*/) };
    clockVisibility = Slider(*context, localizedText.getMessage(OsdSettingsMessages::clockVisibility), controlX, currentLineY,
                             Control::pageLabelWidth(), Control::pageControlWidth(), theme.sliderArrowColor(), CLOCK_VISIBILITY_ID,
                             changeHandler, visibilityOptions, sizeof(visibilityOptions)/sizeof(*visibilityOptions), 1);
    isClockEnabled = true;
    registry.emplace_back(clockVisibility, true);
    currentLineY += Control::pageLineHeight();
  }
  // clock format
  {
    ComboBoxOption formatOptions[]{ ComboBoxOption(u"HH:MM (00-24)", 0/*TMP*/),
                                    ComboBoxOption(u"H:MM (0-24)", 1/*TMP*/),
                                    ComboBoxOption(u"H:MM (0-12 AM/PM)", 2/*TMP*/) };
    clockFormat = Slider(*context, localizedText.getMessage(OsdSettingsMessages::clockFormat), controlX, currentLineY,
                         Control::pageLabelWidth(), Control::pageControlWidth(), theme.sliderArrowColor(), 0,
                         nullptr, formatOptions, sizeof(formatOptions)/sizeof(*formatOptions), 1, &isClockEnabled);
    registry.emplace_back(clockFormat, true);
    currentLineY += Control::pageLineHeight();
  }
  // clock location
  {
    ComboBoxOption locationOptionsX[]{ ComboBoxOption(localizedText.getMessage(CommonMessages::left), 0/*TMP*/),
                                       ComboBoxOption(localizedText.getMessage(CommonMessages::center), 1/*TMP*/),
                                       ComboBoxOption(localizedText.getMessage(CommonMessages::right), 2/*TMP*/) };
    clockLocationX = Slider(*context, localizedText.getMessage(OsdSettingsMessages::clockLocationX), controlX, currentLineY,
                            Control::pageLabelWidth(), Control::pageControlWidth(), theme.sliderArrowColor(), 0,
                            nullptr, locationOptionsX, sizeof(locationOptionsX)/sizeof(*locationOptionsX), 2, &isClockEnabled);
    registry.emplace_back(clockLocationX, true);
    currentLineY += Control::pageLineHeight();

    ComboBoxOption locationOptionsY[]{ ComboBoxOption(localizedText.getMessage(CommonMessages::top), 0/*TMP*/),
                                       ComboBoxOption(localizedText.getMessage(CommonMessages::bottom), 1/*TMP*/) };
    clockLocationY = Slider(*context, localizedText.getMessage(OsdSettingsMessages::clockLocationY), controlX, currentLineY,
                            Control::pageLabelWidth(), Control::pageControlWidth(), theme.sliderArrowColor(), 0,
                            nullptr, locationOptionsY, sizeof(locationOptionsY)/sizeof(*locationOptionsY), 0, &isClockEnabled);
    registry.emplace_back(clockLocationY, true);
    currentLineY += Control::pageLineHeight() + Control::fieldsetContentMarginBottom();
  }

  // --- tech info group ---
  techInfoGroup = Fieldset(*context, localizedText.getMessage(OsdSettingsMessages::techInfoGroup), theme.fieldsetStyle(),
                           theme.fieldsetControlColor(), x + (int32_t)fieldsetPaddingX, currentLineY, fieldsetWidth,
                           Control::fieldsetContentHeight(2));
  currentLineY += Control::pageLineHeight() + Control::fieldsetContentPaddingTop();

  // tech info visibility
  techInfoVisibility = CheckBox(*context, localizedText.getMessage(OsdSettingsMessages::techInfoVisibility), controlX,
                                currentLineY, Control::pageLabelWidth(), 0, nullptr, isTechInfoEnabled);
  isTechInfoEnabled = false;
  registry.emplace_back(techInfoVisibility, true);
  currentLineY += Control::pageLineHeight();

  // tech info format
  {
    ComboBoxOption formatOptions[]{ ComboBoxOption(localizedText.getMessage(OsdSettingsMessages::techInfoType_fps), 0/*TMP*/),
                                    ComboBoxOption(localizedText.getMessage(OsdSettingsMessages::techInfoType_format), 1/*TMP*/),
                                    ComboBoxOption(localizedText.getMessage(OsdSettingsMessages::techInfoType_all), 2/*TMP*/) };
    techInfoType = Slider(*context, localizedText.getMessage(OsdSettingsMessages::techInfoType), controlX, currentLineY,
                          Control::pageLabelWidth(), Control::pageControlWidth(), theme.sliderArrowColor(), 0, nullptr,
                          formatOptions, sizeof(formatOptions)/sizeof(*formatOptions), 0, &isTechInfoEnabled);
    registry.emplace_back(techInfoType, true);
    currentLineY += Control::pageLineHeight();// +Control::fieldsetContentMarginBottom();
  }

  // --- control registry ---
  if (currentLineY > y + (int32_t)contentHeight())
    Page::moveScrollbarThumb(currentLineY);
  registerControls(std::move(registry));
}

OsdSettings::~OsdSettings() noexcept {
  title.release();

  clockGroup.release();
  clockVisibility.release();
  clockFormat.release();
  clockLocationX.release();
  clockLocationY.release();

  techInfoGroup.release();
  techInfoVisibility.release();
  techInfoType.release();
}


// -- window events -- ---------------------------------------------------------

void OsdSettings::move(int32_t x, int32_t y, uint32_t width, uint32_t height) {
  Page::moveBase(x, y, width, height);
  const uint32_t fieldsetPaddingX = Control::fieldsetMarginX(width);
  const int32_t controlX = x + (int32_t)fieldsetPaddingX + (int32_t)Control::fieldsetContentMarginX(width);
  uint32_t fieldsetWidth = width - (fieldsetPaddingX << 1);
  if (fieldsetWidth > Control::fieldsetMaxWidth())
    fieldsetWidth = Control::fieldsetMaxWidth();

  title.move(context->renderer(), context->pixelSizeX(), context->pixelSizeY(), x + (int32_t)fieldsetPaddingX, y + Control::titleMarginTop());

  // clock group
  int32_t currentLineY = title.y() + (int32_t)title.height() + Control::pageLineHeight();
  clockGroup.move(*context, x + (int32_t)fieldsetPaddingX, currentLineY, fieldsetWidth, Control::fieldsetContentHeight(4));
  currentLineY += Control::pageLineHeight() + Control::fieldsetContentPaddingTop();

  clockVisibility.move(*context, controlX, currentLineY);
  currentLineY += Control::pageLineHeight();
  clockFormat.move(*context, controlX, currentLineY);
  currentLineY += Control::pageLineHeight();
  clockLocationX.move(*context, controlX, currentLineY);
  currentLineY += Control::pageLineHeight();
  clockLocationY.move(*context, controlX, currentLineY);
  currentLineY += Control::pageLineHeight() + Control::fieldsetContentMarginBottom();

  // tech info group
  techInfoGroup.move(*context, x + (int32_t)fieldsetPaddingX, currentLineY, fieldsetWidth, Control::fieldsetContentHeight(2));
  currentLineY += Control::pageLineHeight() + Control::fieldsetContentPaddingTop();

  techInfoVisibility.move(*context, controlX, currentLineY);
  currentLineY += Control::pageLineHeight();
  techInfoType.move(*context, controlX, currentLineY);
  currentLineY += Control::pageLineHeight();// + Control::fieldsetContentMarginBottom();

  Page::moveScrollbarThumb(currentLineY); // required after a move
}

void OsdSettings::onChange(uint32_t id, uint32_t value) {
  switch (id) {
    case CLOCK_VISIBILITY_ID: isClockEnabled = (value != 0); break;
    default: assert(false); break;
  }
}


// -- rendering -- -------------------------------------------------------------

void OsdSettings::drawIcons() {
  // scrollable geometry
  buffers->bindScrollLocationBuffer(context->renderer(), ScissorRectangle(x(), y(), width(), contentHeight()));

  auto* hoverControl = getActiveControl();
  techInfoVisibility.drawIcon(*context, *buffers, (hoverControl == &techInfoVisibility));
}

bool OsdSettings::drawPageBackgrounds(int32_t mouseX, int32_t) {
  // scrollable geometry
  if (buffers->isFixedLocationBuffer())
    buffers->bindScrollLocationBuffer(context->renderer(), ScissorRectangle(x(), y(), width(), contentHeight()));

  auto* hoverControl = getActiveControl();
  clockGroup.drawBackground(*context, *buffers);
  techInfoGroup.drawBackground(*context, *buffers);

  clockVisibility.drawBackground(*context, mouseX, *buffers, (hoverControl == &clockVisibility));
  clockFormat.drawBackground(*context, mouseX, *buffers, (hoverControl == &clockFormat));
  clockLocationX.drawBackground(*context, mouseX, *buffers, (hoverControl == &clockLocationX));
  clockLocationY.drawBackground(*context, mouseX, *buffers, (hoverControl == &clockLocationY));
  techInfoType.drawBackground(*context, mouseX, *buffers, (hoverControl == &techInfoType));
  return false;
}

void OsdSettings::drawPageLabels() {
  // scrollable geometry
  auto& renderer = context->renderer();
  if (buffers->isFixedLocationBuffer())
    buffers->bindScrollLocationBuffer(renderer, ScissorRectangle(x(), y(), width(), contentHeight()));

  buffers->bindLabelBuffer(renderer, LabelBufferType::title);
  title.draw(renderer);

  auto* hoverControl = getActiveControl();
  clockGroup.drawLabel(*context, *buffers);
  techInfoGroup.drawLabel(*context, *buffers);

  clockVisibility.drawLabels(*context, *buffers, (hoverControl == &clockVisibility));
  clockFormat.drawLabels(*context, *buffers, (hoverControl == &clockFormat));
  clockLocationX.drawLabels(*context, *buffers, (hoverControl == &clockLocationX));
  clockLocationY.drawLabels(*context, *buffers, (hoverControl == &clockLocationY));
  techInfoType.drawLabels(*context, *buffers, (hoverControl == &techInfoType));

  techInfoVisibility.drawLabel(*context, *buffers, (hoverControl == &techInfoVisibility));
}
