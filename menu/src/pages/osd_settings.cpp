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
#include "menu/pages/osd_settings.h"

using namespace video_api;
using namespace display;
using namespace display::controls;
using namespace menu::controls;
using namespace menu::pages;
using namespace menu;

PageId OsdSettings::PageType() const noexcept { return PageId::generalOsdSettings; }


// -- page -- ------------------------------------------------------------------

#define CLOCK_VISIBILITY_ID     1

void OsdSettings::init(const ColorTheme& theme, const MessageResources& localizedText, int32_t x, int32_t y, uint32_t width) {
  PageContentBuilder builder(*context, theme, x, y, width, 6, nullptr,
                             std::bind(&OsdSettings::onValueChange,this,std::placeholders::_1,std::placeholders::_2));

  builder.addTitle(localizedText.getMessage(OsdSettingsMessages::title), title);

  // clock group
  builder.addFieldset(localizedText.getMessage(OsdSettingsMessages::clockGroup), 4, 0, clockGroup);
  isClockEnabled = true;

  ComboBoxOption visibilityOptions[]{ ComboBoxOption(localizedText.getMessage(CommonMessages::disabled), 0/*TMP*/),
                                      ComboBoxOption(localizedText.getMessage(OsdSettingsMessages::clockVisibility_mouseMove), 1/*TMP*/),
                                      ComboBoxOption(localizedText.getMessage(OsdSettingsMessages::clockVisibility_always), 2/*TMP*/) };
  builder.addSlider(CLOCK_VISIBILITY_ID, localizedText.getMessage(OsdSettingsMessages::clockVisibility), nullptr, Control::pageControlWidth(),
                    visibilityOptions, sizeof(visibilityOptions)/sizeof(*visibilityOptions), 1, clockVisibility);
  builder.setEnabler(isClockEnabled);

  ComboBoxOption formatOptions[]{ ComboBoxOption(u"HH:MM (00-23)", 0/*TMP*/),
                                  ComboBoxOption(u"H:MM (0-23)", 1/*TMP*/),
                                  ComboBoxOption(u"H:MM (0-11 AM/PM)", 2/*TMP*/) };
  builder.addSlider(0, localizedText.getMessage(OsdSettingsMessages::clockFormat), nullptr, Control::pageControlWidth(),
                    formatOptions, sizeof(formatOptions)/sizeof(*formatOptions), 1, clockFormat);

  ComboBoxOption locationOptionsX[]{ ComboBoxOption(localizedText.getMessage(CommonMessages::left), 0/*TMP*/),
                                     ComboBoxOption(localizedText.getMessage(CommonMessages::center), 1/*TMP*/),
                                     ComboBoxOption(localizedText.getMessage(CommonMessages::right), 2/*TMP*/) };
  builder.addSlider(0, localizedText.getMessage(OsdSettingsMessages::clockLocationX), nullptr, Control::pageControlWidth(),
                    locationOptionsX, sizeof(locationOptionsX)/sizeof(*locationOptionsX), 2, clockLocationX);

  ComboBoxOption locationOptionsY[]{ ComboBoxOption(localizedText.getMessage(CommonMessages::top), 0/*TMP*/),
                                     ComboBoxOption(localizedText.getMessage(CommonMessages::bottom), 1/*TMP*/) };
  builder.addSlider(0, localizedText.getMessage(OsdSettingsMessages::clockLocationY), nullptr, Control::pageControlWidth(),
                    locationOptionsY, sizeof(locationOptionsY)/sizeof(*locationOptionsY), 0, clockLocationY);
  builder.resetEnabler();

  // tech info group
  builder.addFieldset(localizedText.getMessage(OsdSettingsMessages::techInfoGroup), 2, 0, techInfoGroup);
  isTechInfoEnabled = false;

  builder.addCheckBox(0, localizedText.getMessage(OsdSettingsMessages::techInfoVisibility), nullptr,
                      isTechInfoEnabled, techInfoVisibility);
  builder.setEnabler(isTechInfoEnabled);

  ComboBoxOption techInfoOptions[]{ ComboBoxOption(localizedText.getMessage(OsdSettingsMessages::techInfoType_fps), 0/*TMP*/),
                                    ComboBoxOption(localizedText.getMessage(OsdSettingsMessages::techInfoType_format), 1/*TMP*/),
                                    ComboBoxOption(localizedText.getMessage(OsdSettingsMessages::techInfoType_all), 2/*TMP*/) };
  builder.addSlider(0, localizedText.getMessage(OsdSettingsMessages::techInfoType), nullptr, Control::pageControlWidth(),
                    techInfoOptions, sizeof(techInfoOptions)/sizeof(*techInfoOptions), 0, techInfoType);

  // control registry
  Page::moveScrollbarThumb(builder.linePositionY());
  registerControls(std::move(builder.controlRegistry()));
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
  PageContentMover mover(*context, x, y, width);
  const int32_t offsetX = mover.linePositionX() - clockVisibility.x();

  mover.moveTitle(title);

  // clock group
  mover.moveFieldset(clockGroup);

  mover.moveSlider(clockVisibility);
  mover.moveSlider(clockFormat);
  mover.moveSlider(clockLocationX);
  mover.moveSlider(clockLocationY);

  // tech info group
  mover.moveFieldset(techInfoGroup);

  mover.moveCheckBox(techInfoVisibility);
  mover.moveSlider(techInfoType);

  Page::moveScrollbarThumb(mover.linePositionY()); // required after a move
  Page::moveRegisteredControls(offsetX);
}

void OsdSettings::onValueChange(uint32_t id, uint32_t value) {
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

void OsdSettings::drawPageBackgrounds(int32_t mouseX, int32_t) {
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
