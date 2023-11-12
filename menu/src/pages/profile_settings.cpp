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
#include "menu/pages/profile_settings.h"

using namespace video_api;
using namespace display;
using namespace display::controls;
using namespace menu::controls;
using namespace menu::pages;
using namespace menu;


// -- helpers -- ---------------------------------------------------------------

#define DEFAULT_NAME_BUFFER_SIZE 20

static void setDefaultProfileName(uint32_t profileId, char32_t buffer[DEFAULT_NAME_BUFFER_SIZE]) {
  const char32_t defaultPrefix[] = U"Profile #";
  memcpy(buffer, defaultPrefix, sizeof(defaultPrefix));

  // append profile ID
  char32_t* it = buffer + (static_cast<intptr_t>(sizeof(defaultPrefix)/sizeof(char32_t)) - 1u);
  if (profileId != 0) {
    it += (profileId < 10000)
        ? ((profileId < 100)
           ? ((profileId < 10) ? 1 : 2)
           : ((profileId < 1000) ? 3 : 4) )
        : ((profileId < 1000000)
           ? ((profileId < 100000) ? 5 : 6)
           : ((profileId < 100000000)
              ? ((profileId < 10000000) ? 7 : 8)
              : ((profileId < 1000000000) ? 9 : 10)));
    *it = U'\0';
    --it;
    while (profileId) {
      *it = U'0' + static_cast<char32_t>(profileId % 10);
      profileId /= 10;
      --it;
    }
  }
  else {
    *it = U'0';
    *(++it) = U'\0';
  }
}

static void fillPreviewBorderColor(const float previewColor[4], float outRgba[4]) {
  *outRgba = *previewColor * 0.6f;
  *(++outRgba) = *(++previewColor) * 0.6f;
  *(++outRgba) = *(++previewColor) * 0.6f;
  *(++outRgba) = *(++previewColor);
}
static void fillPreviewTopColor(const float previewColor[4], float outRgba[4]) {
  *outRgba = *previewColor * 1.45f;
  if (*outRgba > 1.0f)
    *outRgba = 1.0f;
  *(++outRgba) = *(++previewColor) * 1.45f;
  if (*outRgba > 1.0f)
    *outRgba = 1.0f;
  *(++outRgba) = *(++previewColor) * 1.45f;
  if (*outRgba > 1.0f)
    *outRgba = 1.0f;
  *(++outRgba) = *(++previewColor);
}


// -- page -- ------------------------------------------------------------------

#define PROFILE_NAME_ID  1
#define TILE_COLOR_ID    2
#define APPLY_PRESET_ID  3
#define COPY_PROFILE_ID  4

#define MAX_NAME_LENGTH 36u

void ProfileSettings::init(const MessageResources& localizedText, int32_t x, int32_t y,
                           uint32_t width, const std::vector<ConfigProfile>& profiles, const std::vector<ConfigProfile>& presets) {
  const MessageResource* textResources = localizedText.profileSettingsMessageArray();
  const uint32_t fieldsetPaddingX = Control::fieldsetMarginX(width);
  const int32_t controlX = x + (int32_t)fieldsetPaddingX + (int32_t)Control::fieldsetContentMarginX(width);
  uint32_t fieldsetWidth = width - (fieldsetPaddingX << 1);
  if (fieldsetWidth > Control::fieldsetMaxWidth())
    fieldsetWidth = Control::fieldsetMaxWidth();

  title = TextMesh(context->renderer(), context->getFont(FontType::titles), GET_UI_MESSAGE(textResources,ProfileSettingsMessages::title),
                   context->pixelSizeX(), context->pixelSizeY(), x + (int32_t)fieldsetPaddingX, y + 24, TextAlignment::left);

  std::vector<ControlRegistration> registry;
  registry.reserve(6);
  int32_t currentLineY = title.y() + (int32_t)title.height() + Control::pageLineHeight();
  auto changeHandler = std::bind(&ProfileSettings::onChange,this,std::placeholders::_1);

  // find profile
  const ConfigProfile* currentProfile = nullptr;
  for (const auto& profile : profiles) {
    if (profile.id == profileId) {
      currentProfile = &profile;
      break;
    }
  }

  // --- profile ID group ---
  profileIdGroup = Fieldset(*context, GET_UI_MESSAGE(textResources,ProfileSettingsMessages::profileIdGroup),
                            theme->fieldsetStyle(), theme->fieldsetControlColor(), x + (int32_t)fieldsetPaddingX,
                            currentLineY, fieldsetWidth, Control::fieldsetContentHeight(2));
  currentLineY += Control::pageLineHeight() + Control::fieldsetContentPaddingY();

  // profile name
  if (currentProfile == nullptr || currentProfile->name == nullptr || currentProfile->name[0] == U'\0') {
    char32_t defaultName[DEFAULT_NAME_BUFFER_SIZE];
    setDefaultProfileName(profileId, defaultName);
    profileName = TextBox(*context, GET_UI_MESSAGE(textResources,ProfileSettingsMessages::profileName), nullptr, controlX,
                          currentLineY, Control::pageLabelWidth(), Control::pageControlWidth(),
                          theme->textBoxControlColor(), PROFILE_NAME_ID, changeHandler, defaultName, MAX_NAME_LENGTH);
  }
  else {
    profileName = TextBox(*context, GET_UI_MESSAGE(textResources,ProfileSettingsMessages::profileName), nullptr, controlX,
                          currentLineY, Control::pageLabelWidth(), Control::pageControlWidth(),
                          theme->textBoxControlColor(), PROFILE_NAME_ID, changeHandler, currentProfile->name.get(), MAX_NAME_LENGTH);
  }
  registry.emplace_back(profileName, true);
  currentLineY += Control::pageLineHeight();

  // color picker
  {
    const MessageResource* colorNames = localizedText.tileColorsMessageArray();
    ComboBoxOption tileColorOptions[(size_t)TileColors::COUNT];
    for (size_t i = 0; i < (size_t)TileColors::COUNT; ++i)
      tileColorOptions[i] = ComboBoxOption(GET_UI_MESSAGE(colorNames,(TileColors)i), (ComboValue)i);

    auto colorChangeHandler = std::bind(&ProfileSettings::onColorChange,this,std::placeholders::_1,std::placeholders::_2);
    tileColor = ComboBox(*context, GET_UI_MESSAGE(textResources,ProfileSettingsMessages::tileColor), controlX, currentLineY,
                         Control::pageLabelWidth(), (Control::pageControlWidth() >> 1), ComboBoxStyle::classic, theme->comboBoxColorParams(),
                         TILE_COLOR_ID, colorChangeHandler, tileColorOptions, sizeof(tileColorOptions)/sizeof(*tileColorOptions), 0);
    registry.emplace_back(tileColor, true);

    // color preview
    const uint32_t previewBoxSize = tileColor.height();
    const float* previewColor = theme->tileColor(TileColors::themeColor);
    float previewBorderColor[4];
    fillPreviewBorderColor(previewColor, previewBorderColor);

    std::vector<ControlVertex> vertices(static_cast<size_t>(8)); 
    GeometryGenerator::fillRectangleVertices(vertices.data(), previewBorderColor,
                                             0.f, (float)previewBoxSize, 0.f, -(float)previewBoxSize);
    GeometryGenerator::fillRectangleVertices(vertices.data() + 4, previewColor,
                                             1.f, (float)previewBoxSize - 1.f, -1.f, -(float)previewBoxSize + 1.f);
    fillPreviewTopColor(previewColor, vertices[4].color);
    fillPreviewTopColor(previewColor, vertices[5].color);
    std::vector<uint32_t> indices{ 0,1,2,2,1,3,  4,5,6,6,5,7 };
    colorPreview = ControlMesh(context->renderer(), std::move(vertices), indices, context->pixelSizeX(), context->pixelSizeY(),
                               tileColor.x() + (int32_t)tileColor.width() + 1, tileColor.y(), previewBoxSize, previewBoxSize);
    currentLineY += Control::pageLineHeight() + Control::fieldsetContentBottomMargin();
  }

  // --- preset group ---
  presetGroup = Fieldset(*context, GET_UI_MESSAGE(textResources,ProfileSettingsMessages::presetGroup),
                         theme->fieldsetStyle(), theme->fieldsetControlColor(), x + (int32_t)fieldsetPaddingX,
                         currentLineY, fieldsetWidth, Control::fieldsetContentHeight(2));
  currentLineY += Control::pageLineHeight() + Control::fieldsetContentPaddingY();

  ButtonStyleProperties buttonStyle(ButtonStyle::fromTopLeft, FontType::inputText, ControlIconType::none, theme->buttonControlColor(),
                                    theme->buttonBorderColor(), 1, 0, Control::buttonPaddingX(), Control::comboBoxPaddingY());
  {
    std::vector<ComboBoxOption> presetLabels;
    presetLabels.reserve(presets.size());
    for (const auto& preset : presets)
      presetLabels.emplace_back(preset.name.get(), (ComboValue)preset.id);
    presetToApply = ComboBox(*context, GET_UI_MESSAGE(textResources,ProfileSettingsMessages::predefinedPreset), controlX,
                             currentLineY, Control::pageLabelWidth(), (Control::pageControlWidth() >> 1), ComboBoxStyle::classic,
                             theme->comboBoxColorParams(), 0, nullptr, presetLabels.data(), presetLabels.size(), 0);
    registry.emplace_back(presetToApply, true);

    applyPreset = Button(*context, GET_UI_MESSAGE(textResources,ProfileSettingsMessages::apply),
                         presetToApply.x() + (int32_t)presetToApply.width() + Control::controlButtonMargin(),
                         currentLineY, buttonStyle, APPLY_PRESET_ID, changeHandler);
    registry.emplace_back(applyPreset, true);
    currentLineY += Control::pageLineHeight();
  }
  {
    std::vector<ComboBoxOption> profileLabels;
    ComboBoxOption* firstOption = nullptr;
    if (profiles.size() > (size_t)1u) {
      profileLabels.reserve(profiles.size() - 1u);
      for (const auto& profile : profiles) {
        if (profile.id != profileId)
          profileLabels.emplace_back(profile.name.get(), (ComboValue)profile.id);
      }
      firstOption = profileLabels.data();
      hasOtherProfiles = true;
    }
    profileToCopy = ComboBox(*context, GET_UI_MESSAGE(textResources,ProfileSettingsMessages::existingProfile), controlX,
                             currentLineY, Control::pageLabelWidth(), (Control::pageControlWidth() >> 1), ComboBoxStyle::classic,
                             theme->comboBoxColorParams(), 0, nullptr, firstOption, profileLabels.size(), 0, &hasOtherProfiles);
    registry.emplace_back(profileToCopy, true);

    copyProfile = Button(*context, GET_UI_MESSAGE(textResources,ProfileSettingsMessages::apply),
                         profileToCopy.x() + (int32_t)profileToCopy.width() + Control::controlButtonMargin(),
                         currentLineY, buttonStyle, COPY_PROFILE_ID, changeHandler, &hasOtherProfiles);
    registry.emplace_back(copyProfile, true);
    currentLineY += Control::pageLineHeight();//+ Control::fieldsetContentBottomMargin();
  }

  // --- control registry ---
  if (currentLineY > y + (int32_t)contentHeight())
    Page::moveScrollbarThumb(currentLineY);
  registerControls(std::move(registry));
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
  const uint32_t fieldsetPaddingX = Control::fieldsetMarginX(width);
  const int32_t controlX = x + (int32_t)fieldsetPaddingX + (int32_t)Control::fieldsetContentMarginX(width);
  uint32_t fieldsetWidth = width - (fieldsetPaddingX << 1);
  if (fieldsetWidth > Control::fieldsetMaxWidth())
    fieldsetWidth = Control::fieldsetMaxWidth();

  title.move(context->renderer(), context->pixelSizeX(), context->pixelSizeY(), x + (int32_t)fieldsetPaddingX, y + 24);

  // profile ID group
  int32_t currentLineY = title.y() + (int32_t)title.height() + Control::pageLineHeight();
  profileIdGroup.move(*context, x + (int32_t)fieldsetPaddingX, currentLineY, fieldsetWidth, Control::fieldsetContentHeight(2));
  currentLineY += Control::pageLineHeight() + Control::fieldsetContentPaddingY();

  profileName.move(*context, controlX, currentLineY);
  currentLineY += Control::pageLineHeight();
  tileColor.move(*context, controlX, currentLineY);
  colorPreview.move(context->renderer(), context->pixelSizeX(), context->pixelSizeY(),
                    tileColor.x() + (int32_t)tileColor.width() + 1, tileColor.y());
  currentLineY += Control::pageLineHeight() + Control::fieldsetContentBottomMargin();

  // preset group
  presetGroup.move(*context, x + (int32_t)fieldsetPaddingX, currentLineY, fieldsetWidth, Control::fieldsetContentHeight(2));
  currentLineY += Control::pageLineHeight() + Control::fieldsetContentPaddingY();

  presetToApply.move(*context, controlX, currentLineY);
  applyPreset.move(*context, presetToApply.x() + (int32_t)presetToApply.width() + Control::controlButtonMargin(), currentLineY);
  currentLineY += Control::pageLineHeight();
  profileToCopy.move(*context, controlX, currentLineY);
  copyProfile.move(*context, profileToCopy.x() + (int32_t)profileToCopy.width() + Control::controlButtonMargin(), currentLineY);
  currentLineY += Control::pageLineHeight();//+ Control::fieldsetContentBottomMargin();

  Page::moveScrollbarThumb(currentLineY); // required after a move
}

void ProfileSettings::onChange(uint32_t id) {
  switch (id) {
    case PROFILE_NAME_ID: {
      if (*profileName.valueText() == U'\0') {
        char32_t defaultName[DEFAULT_NAME_BUFFER_SIZE];
        setDefaultProfileName(this->profileId, defaultName);
        this->profileName.replaceValueText(*(this->context), defaultName);
      }
      break;
    }
    case APPLY_PRESET_ID: {
      break;
    }
    case COPY_PROFILE_ID: {
      break;
    }
    default: assert(false); break;
  }
}

void ProfileSettings::onColorChange(uint32_t id, uint32_t value) {
  if (id == TILE_COLOR_ID) {
    const float* previewColor = theme->tileColor((TileColors)value);
    float previewBorderColor[4];
    fillPreviewBorderColor(previewColor, previewBorderColor);

    std::vector<ControlVertex> vertices = colorPreview.relativeVertices();
    memcpy(vertices[0].color, previewBorderColor, sizeof(float)*4);
    memcpy(vertices[1].color, previewBorderColor, sizeof(float)*4);
    memcpy(vertices[2].color, previewBorderColor, sizeof(float)*4);
    memcpy(vertices[3].color, previewBorderColor, sizeof(float)*4);
    fillPreviewTopColor(previewColor, vertices[4].color);
    fillPreviewTopColor(previewColor, vertices[5].color);
    memcpy(vertices[6].color, previewColor, sizeof(float)*4);
    memcpy(vertices[7].color, previewColor, sizeof(float)*4);

    colorPreview.update(context->renderer(), std::move(vertices), context->pixelSizeX(), context->pixelSizeY(),
                        colorPreview.x(), colorPreview.y(), colorPreview.width(), colorPreview.height());
  }
}


// -- rendering -- -------------------------------------------------------------

bool ProfileSettings::drawPageBackgrounds(int32_t, int32_t) {
  // scrollable geometry
  if (buffers->isFixedLocationBuffer())
    buffers->bindScrollLocationBuffer(context->renderer(), ScissorRectangle(x(), y(), width(), contentHeight()));

  profileIdGroup.drawBackground(*context, *buffers);
  presetGroup.drawBackground(*context, *buffers);

  profileName.drawBackground(*context, *buffers);
  buffers->bindControlBuffer(context->renderer(), ControlBufferType::regular);
  colorPreview.draw(context->renderer());

  auto* hoverControl = getActiveControl();
  tileColor.drawBackground(*context, *buffers, (hoverControl == &tileColor));
  presetToApply.drawBackground(*context, *buffers, (hoverControl == &presetToApply));
  profileToCopy.drawBackground(*context, *buffers, (hoverControl == &profileToCopy));

  applyPreset.drawBackground(*context, *buffers, (hoverControl == &applyPreset));
  copyProfile.drawBackground(*context, *buffers, (hoverControl == &copyProfile));
  return false;
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

void ProfileSettings::drawForegrounds() {
  auto& renderer = context->renderer();

  ScissorRectangle fullWindowArea(0, 0, context->clientWidth(), context->clientHeight());
  buffers->bindScrollLocationBuffer(renderer, fullWindowArea); // visible outside of scroll area -> full window

  tileColor.drawDropdown(*context, *buffers);
  presetToApply.drawDropdown(*context, *buffers);
  profileToCopy.drawDropdown(*context, *buffers);
}

void ProfileSettings::drawForegroundLabels() {
  ScissorRectangle fullWindowArea(0, 0, context->clientWidth(), context->clientHeight());
  buffers->bindScrollLocationBuffer(context->renderer(), fullWindowArea); // visible outside of scroll area -> full window

  tileColor.drawOptions(*context, *buffers);
  presetToApply.drawOptions(*context, *buffers);
  profileToCopy.drawOptions(*context, *buffers);
}
