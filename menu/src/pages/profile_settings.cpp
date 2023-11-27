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
#include <cstdlib>
#include <cstring>
#include "menu/tile_colors.h"
#include "menu/controls/geometry_generator.h"
#include "menu/pages/page_content_builder.h"
#include "menu/pages/profile_settings.h"

using namespace video_api;
using namespace display;
using namespace display::controls;
using namespace menu::controls;
using namespace menu::pages;
using namespace menu;


// -- helpers -- ---------------------------------------------------------------

#define DEFAULT_NAME_BUFFER_SIZE 20

static void setDefaultProfileName(uint32_t profileId, char16_t buffer[DEFAULT_NAME_BUFFER_SIZE]) {
  const char16_t defaultPrefix[] = u"Profile #";
  memcpy(buffer, defaultPrefix, sizeof(defaultPrefix));

  // append profile ID
  char16_t* it = buffer + (static_cast<intptr_t>(sizeof(defaultPrefix)/sizeof(char16_t)) - 1u);
  if (profileId >= 10u) {
    it += (profileId < 10000u)
        ? ((profileId < 100u) ? 1 : ((profileId < 1000u) ? 3 : 4) )
        : ((profileId < 1000000u)
           ? ((profileId < 100000u) ? 5 : 6)
           : ((profileId < 100000000u)
              ? ((profileId < 10000000u) ? 7 : 8)
              : ((profileId < 1000000000u) ? 9 : 10)));
    *it = u'\0';
    --it;
    while (profileId) {
      *it = u'0' + static_cast<char16_t>(profileId % 10u);
      profileId /= 10u;
      --it;
    }
  }
  else {
    *it = u'0' + static_cast<char16_t>(profileId);
    *(++it) = u'\0';
  }
}


// -- page -- ------------------------------------------------------------------

#define PROFILE_NAME_ID  1
#define TILE_COLOR_ID    2
#define APPLY_PRESET_ID  3
#define COPY_PROFILE_ID  4

#define MAX_NAME_LENGTH ComboBoxOption::maxLength

void ProfileSettings::init(const MessageResources& localizedText, int32_t x, int32_t y,
                           uint32_t width, const std::vector<ConfigProfile>& profiles, const std::vector<ConfigProfile>& presets) {
  PageContentBuilder builder(*context, *theme, x, y, width, 6,
                             std::bind(&ProfileSettings::onChange,this,std::placeholders::_1),
                             std::bind(&ProfileSettings::onValueChange,this,std::placeholders::_1,std::placeholders::_2));

  builder.addTitle(localizedText.getMessage(ProfileSettingsMessages::title), title);

  // find profile
  const ConfigProfile* currentProfile = nullptr;
  for (const auto& profile : profiles) {
    if (profile.id == profileId) {
      currentProfile = &profile;
      break;
    }
  }

  // profile ID group
  builder.addFieldset(localizedText.getMessage(ProfileSettingsMessages::profileIdGroup), 2, 0, profileIdGroup);

  if (currentProfile == nullptr || currentProfile->name == nullptr || currentProfile->name[0] == u'\0') {
    char16_t defaultName[DEFAULT_NAME_BUFFER_SIZE];
    setDefaultProfileName(profileId, defaultName);
    builder.addStringTextBox(PROFILE_NAME_ID, localizedText.getMessage(ProfileSettingsMessages::profileName),
                             localizedText.getMessage(ProfileSettingsMessages::profileName_tooltip),
                             defaultName, MAX_NAME_LENGTH, profileName);
  }
  else {
    builder.addStringTextBox(PROFILE_NAME_ID, localizedText.getMessage(ProfileSettingsMessages::profileName),
                             localizedText.getMessage(ProfileSettingsMessages::profileName_tooltip),
                             currentProfile->name.get(), MAX_NAME_LENGTH, profileName);
  }

  ComboBoxOption tileColorOptions[(size_t)TileColors::COUNT];
  for (size_t i = 0; i < (size_t)TileColors::COUNT; ++i)
    tileColorOptions[i] = ComboBoxOption(localizedText.getMessage((TileColors)i), (ComboValue)i);

  builder.addColorPicker(TILE_COLOR_ID, localizedText.getMessage(ProfileSettingsMessages::tileColor),
                         localizedText.getMessage(ProfileSettingsMessages::tileColor_tooltip),
                         theme->tileColor(TileColors::themeColor), tileColorOptions,
                         sizeof(tileColorOptions)/sizeof(*tileColorOptions), 0, tileColor, colorPreview);

  // preset group
  builder.addFieldset(localizedText.getMessage(ProfileSettingsMessages::presetGroup), 2, 0, presetGroup);
  hasOtherProfiles = false;

  std::vector<ComboBoxOption> comboBoxLabels;
  comboBoxLabels.reserve(presets.size());
  for (const auto& preset : presets) {
    comboBoxLabels.emplace_back(preset.name.get(), (ComboValue)preset.id);
  }
  const char16_t* buttonLabel = localizedText.getMessage(CommonMessages::apply);
  builder.addComboBoxWithButton(0, localizedText.getMessage(ProfileSettingsMessages::predefinedPreset),
                                localizedText.getMessage(ProfileSettingsMessages::predefinedPreset_tooltip),
                                comboBoxLabels.data(), comboBoxLabels.size(), presetToApply,
                                APPLY_PRESET_ID, buttonLabel, applyPreset);

  ComboBoxOption* firstComboBoxLabel = nullptr;
  comboBoxLabels.clear();
  if (profiles.size() > (size_t)1u) {
    comboBoxLabels.reserve(profiles.size() - 1u);
    for (const auto& profile : profiles) {
      if (profile.id != profileId)
        comboBoxLabels.emplace_back(profile.name.get(), (ComboValue)profile.id);
    }
    firstComboBoxLabel = comboBoxLabels.data();
    hasOtherProfiles = true;
  }
  builder.addComboBoxWithButton(0, localizedText.getMessage(ProfileSettingsMessages::existingProfile),
                                localizedText.getMessage(ProfileSettingsMessages::existingProfile_tooltip),
                                firstComboBoxLabel, comboBoxLabels.size(), profileToCopy,
                                COPY_PROFILE_ID, buttonLabel, copyProfile);

  // control registry
  Page::moveScrollbarThumb(builder.linePositionY());
  registerControls(std::move(builder.controlRegistry()));

  // confirmation popup
  const char16_t* popupButtons[]{ localizedText.getMessage(CommonMessages::ok),
                                  localizedText.getMessage(CommonMessages::cancel) };
  confirmationPopup = Popup(*context, *theme, localizedText.getMessage(ProfileSettingsMessages::applyPopupTitle),
                            localizedText.getMessage(ProfileSettingsMessages::applyPopupMessage),
                            nullptr, popupButtons, sizeof(popupButtons)/sizeof(*popupButtons));
  confirmationPopup.close();
}

ProfileSettings::~ProfileSettings() noexcept {
  title.release();

  profileIdGroup.release();
  profileName.release();
  colorPreview.release();
  tileColor.release();

  presetGroup.release();
  presetToApply.release();
  profileToCopy.release();
  applyPreset.release();
  copyProfile.release();

  theme = nullptr;
}


// -- window events -- ---------------------------------------------------------

void ProfileSettings::move(int32_t x, int32_t y, uint32_t width, uint32_t height) {
  Page::moveBase(x, y, width, height);
  PageContentMover mover(*context, x, y, width);
  const int32_t offsetX = mover.linePositionX() - profileName.x();

  mover.moveTitle(title);

  // profile ID group
  mover.moveFieldset(profileIdGroup);

  mover.moveTextBox(profileName);
  mover.moveColorPicker(tileColor, colorPreview);

  // preset group
  mover.moveFieldset(presetGroup);

  mover.moveComboBoxWithButton(presetToApply, applyPreset);
  mover.moveComboBoxWithButton(profileToCopy, copyProfile);

  Page::moveScrollbarThumb(mover.linePositionY()); // required after a move
  confirmationPopup.move(*context);
  Page::moveRegisteredControls(offsetX);
}

void ProfileSettings::onChange(uint32_t id) {
  switch (id) {
    case PROFILE_NAME_ID: {
      if (*profileName.valueText() == u'\0') {
        char16_t defaultName[DEFAULT_NAME_BUFFER_SIZE];
        setDefaultProfileName(this->profileId, defaultName);
        this->profileName.replaceValueText(*(this->context), defaultName);
      }
      break;
    }
    case APPLY_PRESET_ID: {
      setActivePopup(confirmationPopup, [this](uint32_t action) {
        if (action == 0)
          ;//TODO
      });
      break;
    }
    case COPY_PROFILE_ID: {
      setActivePopup(confirmationPopup, [this](uint32_t action) {
        if (action == 0)
          ;//TODO
      });
      break;
    }
    default: assert(false); break;
  }
}

void ProfileSettings::onValueChange(uint32_t id, uint32_t value) {
  if (id == TILE_COLOR_ID)
    PageContentBuilder::changeColorPickerColors(*context, theme->tileColor((TileColors)value), colorPreview);
}


// -- rendering -- -------------------------------------------------------------

void ProfileSettings::drawPageBackgrounds(int32_t mouseX, int32_t mouseY) {
  // scrollable geometry
  if (buffers->isFixedLocationBuffer())
    buffers->bindScrollLocationBuffer(context->renderer(), ScissorRectangle(x(), y(), width(), contentHeight()));

  profileIdGroup.drawBackground(*context, *buffers);
  presetGroup.drawBackground(*context, *buffers);

  auto* hoverControl = getActiveControl();
  profileName.drawBackground(*context, mouseX, mouseY, *buffers, (hoverControl == &profileName));
  buffers->bindControlBuffer(context->renderer(), ControlBufferType::regular);
  colorPreview.draw(context->renderer());

  tileColor.drawBackground(*context, *buffers, (hoverControl == &tileColor));
  presetToApply.drawBackground(*context, *buffers, (hoverControl == &presetToApply));
  profileToCopy.drawBackground(*context, *buffers, (hoverControl == &profileToCopy));

  applyPreset.drawBackground(*context, *buffers, (hoverControl == &applyPreset), isMouseDown());
  copyProfile.drawBackground(*context, *buffers, (hoverControl == &copyProfile), isMouseDown());
}

void ProfileSettings::drawPageLabels() {
  // scrollable geometry
  auto& renderer = context->renderer();
  if (buffers->isFixedLocationBuffer())
    buffers->bindScrollLocationBuffer(renderer, ScissorRectangle(x(), y(), width(), contentHeight()));

  buffers->bindLabelBuffer(renderer, LabelBufferType::title);
  title.draw(renderer);

  profileIdGroup.drawLabel(*context, *buffers);
  presetGroup.drawLabel(*context, *buffers);

  auto* hoverControl = getActiveControl();
  profileName.drawLabels(*context, *buffers, (hoverControl == &profileName));

  tileColor.drawLabels(*context, *buffers, (hoverControl == &tileColor));
  presetToApply.drawLabels(*context, *buffers, (hoverControl == &presetToApply));
  profileToCopy.drawLabels(*context, *buffers, (hoverControl == &profileToCopy));

  applyPreset.drawLabel(*context, *buffers, (hoverControl == &applyPreset));
  copyProfile.drawLabel(*context, *buffers, (hoverControl == &copyProfile));
}
