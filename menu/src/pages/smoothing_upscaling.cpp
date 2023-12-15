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
#include "menu/pages/smoothing_upscaling.h"

using namespace video_api;
using namespace pandora::hardware;
using namespace display;
using namespace display::controls;
using namespace menu::controls;
using namespace menu::pages;
using namespace menu;

PageId SmoothingUpscaling::PageType() const noexcept { return PageId::profileSmoothingUpscaling; }


// -- helpers -- ---------------------------------------------------------------

static inline uint32_t getSelectedValue(const ComboBox& comboBox, uint32_t defaultValue) {
  const auto* value = comboBox.getSelectedValue();
  return (value != nullptr) ? *value : defaultValue;
}

static inline char16_t* copyString(const char16_t* value, char16_t* destination) {
  size_t length = TextMesh::getStringLength(value);
  memcpy(destination, value, length*sizeof(char16_t));
  return destination + length;
}

static inline char16_t* resolutionToString(uint32_t value, char16_t* destination) {
  if (value >= 10u) {
    destination += (value < 10000u)
      ? ((value < 100u) ? 1 : ((value < 1000u) ? 3 : 4) )
      : ((value < 1000000u)
        ? ((value < 100000u) ? 5 : 6)
        : ((value < 100000000u)
          ? ((value < 10000000u) ? 7 : 8)
          : ((value < 1000000000u) ? 9 : 10)));
    auto* end = destination;
    --destination;
    while (value) {
      *destination = u'0' + static_cast<char16_t>(value % 10u);
      value /= 10u;
      --destination;
    }
    return end;
  }
  else {
    *destination = u'0' + static_cast<char16_t>(value);
    return destination + 1;
  }
}

// ---

#define INT_RES_VALUE_BUFFER_SIZE 48
#define MAX_FRAMEBUFFER_SIZE_X 640u
#define MAX_FRAMEBUFFER_SIZE_Y 256u

static void fillFramebufferResolutionInfo(uint32_t resX, uint32_t resY, const MessageResources& localizedText,
                                        char16_t outValue[INT_RES_VALUE_BUFFER_SIZE]) {
  outValue = copyString(localizedText.getMessage(SmoothingUpscalingMessages::framebufferResolution_prefix), outValue);
  outValue = resolutionToString(resX*320u, outValue);
  *outValue = u'x';
  outValue = resolutionToString(resY*224u, outValue + 1);
  outValue = copyString(localizedText.getMessage(CommonMessages::to), outValue);
  outValue = resolutionToString(resX*MAX_FRAMEBUFFER_SIZE_X, outValue);
  *outValue = u'x';
  outValue = resolutionToString(resY*MAX_FRAMEBUFFER_SIZE_Y, outValue + 1);
  *outValue = u'\0';
}
static void fillDisplaySizeValue(uint32_t resX, uint32_t resY, uint32_t upscalingFactor, const MessageResources& localizedText,
                                 char16_t outValue[INT_RES_VALUE_BUFFER_SIZE]) {
  resX *= upscalingFactor;
  resY *= upscalingFactor;
  outValue = copyString(localizedText.getMessage(SmoothingUpscalingMessages::displaySize_prefix), outValue);
  outValue = resolutionToString(resX*320u, outValue);
  *outValue = u'x';
  outValue = resolutionToString(resY*224u, outValue + 1);
  outValue = copyString(localizedText.getMessage(CommonMessages::to), outValue);
  outValue = resolutionToString(resX*MAX_FRAMEBUFFER_SIZE_X, outValue);
  *outValue = u'x';
  outValue = resolutionToString(resY*MAX_FRAMEBUFFER_SIZE_Y, outValue + 1);
  *outValue = u'\0';
}

#define UPSCALING_FACTORS_MAX_ARRAY_SIZE 5

static size_t fillUpscalingFactors(UpscalingType mode, ComboBoxOption options[UPSCALING_FACTORS_MAX_ARRAY_SIZE]) {
  switch (mode) {
    case UpscalingType::jinc:
    case UpscalingType::SaI:
    case UpscalingType::super_xBR:
      *options = ComboBoxOption(u"2x", 2);
      *(++options) = ComboBoxOption(u"4x", 4);
      *(++options) = ComboBoxOption(u"8x", 8);
      return 3;
    case UpscalingType::SABR:
      *options = ComboBoxOption(u"2x", 2);
      *(++options) = ComboBoxOption(u"3x", 3);
      *(++options) = ComboBoxOption(u"4x", 4);
      return 3;
    case UpscalingType::xBRZ:
      *options = ComboBoxOption(u"2x", 2);
      *(++options) = ComboBoxOption(u"3x", 3);
      *(++options) = ComboBoxOption(u"4x", 4);
      *(++options) = ComboBoxOption(u"5x", 5);
      *(++options) = ComboBoxOption(u"6x", 6);
      return 5;
    case UpscalingType::scaleFX:
      *options = ComboBoxOption(u"2x", 2);
      *(++options) = ComboBoxOption(u"3x", 3);
      *(++options) = ComboBoxOption(u"4x", 4);
      *(++options) = ComboBoxOption(u"6x", 6);
      *(++options) = ComboBoxOption(u"9x", 9);
      return 5;
    default: // OFF
      *options = ComboBoxOption(u"1x", 1);
      return 1;
  }
}


// -- page -- ------------------------------------------------------------------

#define INTERNAL_RES_X_ID           1
#define INTERNAL_RES_Y_ID           2
#define SCREEN_UPSCALING_ID         3
#define TEXTURE_UPSCALING_ID        4
#define SPRITE_UPSCALING_ID         5
#define SCREEN_UPSCALING_FACTOR_ID  6
#define SPRITE_SAME_TEXTURE_ID      7

#define MAX_BLUR_VALUE 3
#define MAX_SPLATTING_VALUE 3

void SmoothingUpscaling::init(const ColorTheme& theme, int32_t x, int32_t y, uint32_t width) {
  PageContentBuilder builder(*context, theme, x, y, width, 17,
                             std::bind(&SmoothingUpscaling::onChange,this,std::placeholders::_1),
                             std::bind(&SmoothingUpscaling::onValueChange,this,std::placeholders::_1,std::placeholders::_2));

  builder.addTitle(localizedText->getMessage(SmoothingUpscalingMessages::title), title);

  constexpr const uint32_t grainModeWidth = Control::pageControlWidth() - (Control::pageControlWidth() >> 2) + 19u;
  constexpr const uint32_t smoothingModeWidth = ((Control::pageControlWidth() + grainModeWidth) >> 1) - 1u;
  constexpr const uint32_t upscalingFactorWidth = (Control::pageControlWidth() >> 2);
  constexpr const uint32_t antiAliasingWidth = Control::pageControlWidth() - upscalingFactorWidth - Control::controlSideMargin();
  constexpr const uint32_t rulerWidth = (Control::pageControlWidth() >> 2) - 1u;

  // framebuffer group
  builder.addFieldset(localizedText->getMessage(SmoothingUpscalingMessages::framebufferGroup), 3, 
                      Control::pageLineHeight() - (Control::pageLineHeight() >> 2), framebufferGroup);

  builder.addDoubleTextBox(INTERNAL_RES_X_ID, INTERNAL_RES_Y_ID, localizedText->getMessage(SmoothingUpscalingMessages::internalResolution),
                           nullptr, localizedText->getMessage(SmoothingUpscalingMessages::internalResolution_tooltip),
                           4u, 4u, 2u, internalResolutionX, internalResolutionY);
  builder.addLineOffset(-(int32_t)(Control::pageLineHeight() >> 2));

  // -> framebuffer output resolution
  char16_t resolutionValue[INT_RES_VALUE_BUFFER_SIZE];
  fillFramebufferResolutionInfo(internalResolutionX.valueInteger(), internalResolutionY.valueInteger(), *localizedText, resolutionValue);
  framebufferResolutionInfo = TextMesh(context->renderer(), context->getFont(FontType::inputText),
                                       resolutionValue, context->pixelSizeX(), context->pixelSizeY(),
                                       internalResolutionX.controlX(), builder.linePositionY() + 1);
  builder.addLineOffset((int32_t)Control::pageLineHeight());

  ComboBoxOption aaOptions[(size_t)AntiAliasing::COUNT];
  for (uint32_t i = 0; i < (uint32_t)AntiAliasing::COUNT; ++i)
    aaOptions[i] = ComboBoxOption(localizedText->getMessage((AntiAliasing)i), (ComboValue)i);
  builder.addComboBox(0, localizedText->getMessage(SmoothingUpscalingMessages::antiAliasing),
                      localizedText->getMessage(SmoothingUpscalingMessages::antiAliasing_tooltip), antiAliasingWidth,
                      aaOptions, (size_t)AntiAliasing::COUNT, 0, antiAliasing);

  ComboBoxOption mdecOptions[(size_t)MdecFilter::COUNT];
  for (uint32_t i = 0; i < (uint32_t)MdecFilter::COUNT; ++i)
    mdecOptions[i] = ComboBoxOption(localizedText->getMessage((MdecFilter)i), (ComboValue)i);
  builder.addComboBox(0, localizedText->getMessage(SmoothingUpscalingMessages::mdecMovieFilter),
                      localizedText->getMessage(SmoothingUpscalingMessages::mdecMovieFilter_tooltip), antiAliasingWidth,
                      mdecOptions, (size_t)MdecFilter::COUNT, 1, mdecMovieFilter);

  // screen group
  builder.addFieldset(localizedText->getMessage(SmoothingUpscalingMessages::screenGroup), 3, 
                      Control::pageLineHeight() - (Control::pageLineHeight() >> 2), screenGroup);
  screenBlurValue = 0;
  constexpr const uint32_t screenUpscalingValue = 0;

  ComboBoxOption upscalingOptions[(size_t)UpscalingType::COUNT];
  for (uint32_t i = 0; i < (uint32_t)UpscalingType::COUNT; ++i)
    upscalingOptions[i] = ComboBoxOption(localizedText->getMessage((UpscalingType)i), (ComboValue)i);
  const char16_t* upscalingTooltip = localizedText->getMessage(SmoothingUpscalingMessages::upscaling_tooltip);

  ComboBoxOption scaleOptions[UPSCALING_FACTORS_MAX_ARRAY_SIZE];
  size_t scaleOptionsCount = fillUpscalingFactors((UpscalingType)screenUpscalingValue, scaleOptions);
  builder.addDoubleComboBox(SCREEN_UPSCALING_ID, localizedText->getMessage(SmoothingUpscalingMessages::screenUpscaling),
                            upscalingTooltip, upscalingOptions, (size_t)UpscalingType::COUNT, screenUpscalingValue, screenUpscaling,
                            SCREEN_UPSCALING_FACTOR_ID, upscalingFactorWidth, scaleOptions, scaleOptionsCount, 0, screenUpscalingFactor);
  builder.addLineOffset(-(int32_t)(Control::pageLineHeight() >> 2));

  // -> display output resolution
  fillDisplaySizeValue(internalResolutionX.valueInteger(), internalResolutionY.valueInteger(),
                       getSelectedValue(screenUpscalingFactor, 1), *localizedText, resolutionValue);
  displaySizeInfo = TextMesh(context->renderer(), context->getFont(FontType::inputText),
                             resolutionValue, context->pixelSizeX(), context->pixelSizeY(),
                             screenUpscaling.controlX(), builder.linePositionY() + 2);
  builder.addLineOffset((int32_t)Control::pageLineHeight());

  ComboBoxOption grainOptions[]{ ComboBoxOption(localizedText->getMessage(CommonMessages::disabled), 0/*TMP*/),
                                 ComboBoxOption(localizedText->getMessage(SmoothingUpscalingMessages::grain_photo), 1/*TMP*/),
                                 ComboBoxOption(localizedText->getMessage(SmoothingUpscalingMessages::grain_gauss), 2/*TMP*/) };
  const char16_t* grainTooltip = localizedText->getMessage(SmoothingUpscalingMessages::grain_tooltip);

  builder.addSlider(0, localizedText->getMessage(SmoothingUpscalingMessages::screenGrain), grainTooltip, grainModeWidth,
                    grainOptions, sizeof(grainOptions)/sizeof(*grainOptions), 0, screenGrain);
  builder.addRuler(0, localizedText->getMessage(SmoothingUpscalingMessages::screenBlur), nullptr,
                   localizedText->getMessage(SmoothingUpscalingMessages::screenBlur_tooltip), rulerWidth,
                   MAX_BLUR_VALUE, screenBlurValue, screenBlur);

  // texture group
  builder.addFieldset(localizedText->getMessage(SmoothingUpscalingMessages::textureGroup), 4, 0, textureGroup);
  constexpr const uint32_t textureUpscalingValue = 0;
  textureSplattingValue = 0;

  scaleOptionsCount = fillUpscalingFactors((UpscalingType)textureUpscalingValue, scaleOptions);
  builder.addDoubleComboBox(TEXTURE_UPSCALING_ID, localizedText->getMessage(SmoothingUpscalingMessages::textureUpscaling),
                            upscalingTooltip, upscalingOptions, (size_t)UpscalingType::COUNT, textureUpscalingValue, textureUpscaling,
                            0, upscalingFactorWidth, scaleOptions, scaleOptionsCount, 0, textureUpscalingFactor);

  ComboBoxOption smoothingOptions[(size_t)InterpolationType::COUNT];
  for (uint32_t i = 0; i < (uint32_t)InterpolationType::COUNT; ++i) {
    smoothingOptions[i] = ComboBoxOption(localizedText->getMessage((InterpolationType)i), i);
  }
  const char16_t* smoothingTooltip = localizedText->getMessage(SmoothingUpscalingMessages::smoothing_tooltip);

  builder.addComboBox(0, localizedText->getMessage(SmoothingUpscalingMessages::textureSmoothing), smoothingTooltip,
                      smoothingModeWidth, smoothingOptions, (size_t)InterpolationType::COUNT, 1,
                      textureSmoothing);
  builder.addSlider(0, localizedText->getMessage(SmoothingUpscalingMessages::textureGrain), grainTooltip, grainModeWidth,
                    grainOptions, sizeof(grainOptions)/sizeof(*grainOptions), 0, textureGrain);
  builder.addRuler(0, localizedText->getMessage(SmoothingUpscalingMessages::textureSplatting), nullptr,
                   localizedText->getMessage(SmoothingUpscalingMessages::textureSplatting_tooltip), rulerWidth,
                   MAX_SPLATTING_VALUE, textureSplattingValue, textureSplatting);

  // sprite group
  builder.addFieldset(localizedText->getMessage(SmoothingUpscalingMessages::spriteGroup), 4, 0, spriteGroup);
  useTextureSettingsForSprites = true;
  allowSpriteSettings = !useTextureSettingsForSprites;
  constexpr const uint32_t spriteUpscalingValue = 0;

  builder.addCheckBox(SPRITE_SAME_TEXTURE_ID, localizedText->getMessage(SmoothingUpscalingMessages::spriteSameAsTexture),
                      localizedText->getMessage(SmoothingUpscalingMessages::spriteSameAsTexture_tooltip),
                      useTextureSettingsForSprites, spriteTextureSettings);
  builder.setEnabler(allowSpriteSettings);

  scaleOptionsCount = fillUpscalingFactors((UpscalingType)spriteUpscalingValue, scaleOptions);
  builder.addDoubleComboBox(SPRITE_UPSCALING_ID, localizedText->getMessage(SmoothingUpscalingMessages::spriteUpscaling),
                            upscalingTooltip, upscalingOptions, (size_t)UpscalingType::COUNT, spriteUpscalingValue, spriteUpscaling,
                            0, upscalingFactorWidth, scaleOptions, scaleOptionsCount, 0, spriteUpscalingFactor);
  builder.addComboBox(0, localizedText->getMessage(SmoothingUpscalingMessages::spriteSmoothing), smoothingTooltip,
                      smoothingModeWidth, smoothingOptions, (size_t)InterpolationType::COUNT, 1,
                      spriteSmoothing);
  builder.addSlider(0, localizedText->getMessage(SmoothingUpscalingMessages::spriteGrain), grainTooltip, grainModeWidth,
                    grainOptions, sizeof(grainOptions)/sizeof(*grainOptions), 0, spriteGrain);

  // control registry
  Page::moveScrollbarThumb(builder.linePositionY());
  registerControls(std::move(builder.controlRegistry()));
}

SmoothingUpscaling::~SmoothingUpscaling() noexcept {
  localizedText = nullptr;
  title.release();

  framebufferGroup.release();
  internalResolutionX.release();
  internalResolutionY.release();
  framebufferResolutionInfo.release();
  mdecMovieFilter.release();
  antiAliasing.release();

  screenGroup.release();
  screenUpscaling.release();
  screenUpscalingFactor.release();
  displaySizeInfo.release();
  screenGrain.release();
  screenBlur.release();

  textureGroup.release();
  textureUpscaling.release();
  textureUpscalingFactor.release();
  textureSmoothing.release();
  textureGrain.release();
  textureSplatting.release();

  spriteGroup.release();
  spriteTextureSettings.release();
  spriteUpscaling.release();
  spriteUpscalingFactor.release();
  spriteSmoothing.release();
  spriteGrain.release();
}


// -- window events -- ---------------------------------------------------------

void SmoothingUpscaling::move(int32_t x, int32_t y, uint32_t width, uint32_t height) {
  Page::moveBase(x, y, width, height);
  PageContentMover mover(*context, x, y, width);
  const int32_t offsetX = mover.linePositionX() - internalResolutionX.x();

  mover.moveTitle(title);

  // framebuffer group
  mover.moveFieldset(framebufferGroup);

  mover.moveDoubleTextBox(internalResolutionX, internalResolutionY);
  mover.addLineOffset(-(int32_t)(Control::pageLineHeight() >> 2));
  framebufferResolutionInfo.move(context->renderer(), context->pixelSizeX(), context->pixelSizeY(),
                                 internalResolutionX.controlX(), mover.linePositionY() + 1);
  mover.addLineOffset((int32_t)Control::pageLineHeight());

  mover.moveComboBox(antiAliasing);
  mover.moveComboBox(mdecMovieFilter);

  // screen group
  mover.moveFieldset(screenGroup);

  mover.moveDoubleComboBox(screenUpscaling, screenUpscalingFactor);
  mover.addLineOffset(-(int32_t)(Control::pageLineHeight() >> 2));
  displaySizeInfo.move(context->renderer(), context->pixelSizeX(), context->pixelSizeY(),
                       screenUpscaling.controlX(), mover.linePositionY() + 2);
  mover.addLineOffset((int32_t)Control::pageLineHeight());

  mover.moveSlider(screenGrain);
  mover.moveRuler(screenBlur);

  // texture group
  mover.moveFieldset(textureGroup);

  mover.moveDoubleComboBox(textureUpscaling, textureUpscalingFactor);
  mover.moveComboBox(textureSmoothing);
  mover.moveSlider(textureGrain);
  mover.moveRuler(textureSplatting);

  // sprite group
  mover.moveFieldset(spriteGroup);

  mover.moveCheckBox(spriteTextureSettings);
  mover.moveDoubleComboBox(spriteUpscaling, spriteUpscalingFactor);
  mover.moveComboBox(spriteSmoothing);
  mover.moveSlider(spriteGrain);

  Page::moveScrollbarThumb(mover.linePositionY()); // required after a move
  Page::moveRegisteredControls(offsetX);
}

#define MAX_INT_RES_X  (uint32_t)MAX_TEXTURE_SIZE/2048u
#define MAX_INT_RES_Y  (uint32_t)MAX_TEXTURE_SIZE/1024u

void SmoothingUpscaling::onChange(uint32_t id) {
  uint32_t resX = internalResolutionX.valueInteger();
  uint32_t resY = internalResolutionY.valueInteger();
  uint32_t upscalingFactor = (screenUpscalingFactor.getSelectedIndex() >= 0) ? *screenUpscalingFactor.getSelectedValue() : 1;
  if (id == INTERNAL_RES_X_ID) {
    if (resX > MAX_INT_RES_X || resX == 0) {
      resX = (resX == 0) ? 1u : MAX_INT_RES_X;
      internalResolutionX.replaceValueInteger(*context, resX);
    }
    const uint32_t maxUpscalingX = MAX_TEXTURE_SIZE / (resX * MAX_FRAMEBUFFER_SIZE_X);
    while (upscalingFactor > maxUpscalingX) {
      screenUpscalingFactor.setSelectedIndex(*context, screenUpscalingFactor.getSelectedIndex() - 1, false);
      upscalingFactor = (screenUpscalingFactor.getSelectedIndex() >= 0) ? *screenUpscalingFactor.getSelectedValue() : 1;
    }
  }
  else if (id == INTERNAL_RES_Y_ID) {
    if (resY > MAX_INT_RES_Y || resY == 0) {
      resY = (resY == 0) ? 1u : MAX_INT_RES_Y;
      internalResolutionY.replaceValueInteger(*context, resY);
    }
    const uint32_t maxUpscalingY = MAX_TEXTURE_SIZE / (resY * MAX_FRAMEBUFFER_SIZE_Y);
    while (upscalingFactor > maxUpscalingY) {
      screenUpscalingFactor.setSelectedIndex(*context, screenUpscalingFactor.getSelectedIndex() - 1, false);
      upscalingFactor = (screenUpscalingFactor.getSelectedIndex() >= 0) ? *screenUpscalingFactor.getSelectedValue() : 1;
    }
  }
  else { assert(false); return; }

  char16_t resolutionValue[INT_RES_VALUE_BUFFER_SIZE];
  fillFramebufferResolutionInfo(resX, resY, *localizedText, resolutionValue);
  framebufferResolutionInfo = TextMesh(context->renderer(), context->getFont(FontType::inputText),
                                       resolutionValue, context->pixelSizeX(), context->pixelSizeY(),
                                       framebufferResolutionInfo.x(), framebufferResolutionInfo.y());

  const auto* factorValue = screenUpscalingFactor.getSelectedValue();
  fillDisplaySizeValue(internalResolutionX.valueInteger(), internalResolutionY.valueInteger(),
                       factorValue ? *factorValue : 1, *localizedText, resolutionValue);
  displaySizeInfo = TextMesh(context->renderer(), context->getFont(FontType::inputText), resolutionValue, context->pixelSizeX(), context->pixelSizeY(),
                             displaySizeInfo.x(), displaySizeInfo.y());
}

void SmoothingUpscaling::onValueChange(uint32_t id, uint32_t value) {
  ComboBoxOption scaleOptions[UPSCALING_FACTORS_MAX_ARRAY_SIZE];
  switch (id) {
    case SCREEN_UPSCALING_ID: {
      const auto* currentValue = screenUpscalingFactor.getSelectedValue();
      size_t scaleOptionsCount = fillUpscalingFactors((UpscalingType)getSelectedValue(screenUpscaling, 0), scaleOptions);
      int32_t selectedIndex = 0;
      if (currentValue) {
        for (size_t i = 0; i < scaleOptionsCount; ++i) {
          if (scaleOptions[i].value() == *currentValue) {
            selectedIndex = (uint32_t)i;
            break;
          }
        }
      }
      screenUpscalingFactor.replaceValues(*context, scaleOptions, scaleOptionsCount, selectedIndex);

      const auto* factorValue = screenUpscalingFactor.getSelectedValue();
      onValueChange(SCREEN_UPSCALING_FACTOR_ID, factorValue ? *factorValue : 1);
      break;
    }
    case TEXTURE_UPSCALING_ID: {
      const auto* currentValue = textureUpscalingFactor.getSelectedValue();
      size_t scaleOptionsCount = fillUpscalingFactors((UpscalingType)getSelectedValue(textureUpscaling, 0), scaleOptions);
      int32_t selectedIndex = 0;
      if (currentValue) {
        for (size_t i = 0; i < scaleOptionsCount; ++i) {
          if (scaleOptions[i].value() == *currentValue) {
            selectedIndex = (uint32_t)i;
            break;
          }
        }
      }
      textureUpscalingFactor.replaceValues(*context, scaleOptions, scaleOptionsCount, selectedIndex);
      break;
    }
    case SPRITE_UPSCALING_ID: {
      const auto* currentValue = spriteUpscalingFactor.getSelectedValue();
      size_t scaleOptionsCount = fillUpscalingFactors((UpscalingType)getSelectedValue(spriteUpscaling, 0), scaleOptions);
      int32_t selectedIndex = 0;
      if (currentValue) {
        for (size_t i = 0; i < scaleOptionsCount; ++i) {
          if (scaleOptions[i].value() == *currentValue) {
            selectedIndex = (uint32_t)i;
            break;
          }
        }
      }
      spriteUpscalingFactor.replaceValues(*context, scaleOptions, scaleOptionsCount, selectedIndex);
      break;
    }
    case SCREEN_UPSCALING_FACTOR_ID: {
      bool isInternalResChanged = false;
      uint32_t upscalingFactor = (screenUpscalingFactor.getSelectedIndex() >= 0) ? *screenUpscalingFactor.getSelectedValue() : 1;
      if (upscalingFactor > MAX_TEXTURE_SIZE / (internalResolutionX.valueInteger() * MAX_FRAMEBUFFER_SIZE_X)) {
        internalResolutionX.replaceValueInteger(*context, MAX_TEXTURE_SIZE / (upscalingFactor * MAX_FRAMEBUFFER_SIZE_X));
        isInternalResChanged = true;
      }
      if (upscalingFactor > MAX_TEXTURE_SIZE / (internalResolutionY.valueInteger() * MAX_FRAMEBUFFER_SIZE_Y)) {
        internalResolutionY.replaceValueInteger(*context, MAX_TEXTURE_SIZE / (upscalingFactor * MAX_FRAMEBUFFER_SIZE_Y));
        isInternalResChanged = true;
      }

      char16_t resolutionValue[INT_RES_VALUE_BUFFER_SIZE];
      if (isInternalResChanged) {
        fillFramebufferResolutionInfo(internalResolutionX.valueInteger(), internalResolutionY.valueInteger(), *localizedText, resolutionValue);
        framebufferResolutionInfo = TextMesh(context->renderer(), context->getFont(FontType::inputText),
                                             resolutionValue, context->pixelSizeX(), context->pixelSizeY(),
                                             framebufferResolutionInfo.x(), framebufferResolutionInfo.y());
      }
      fillDisplaySizeValue(internalResolutionX.valueInteger(), internalResolutionY.valueInteger(),
                           value, *localizedText, resolutionValue);
      displaySizeInfo = TextMesh(context->renderer(), context->getFont(FontType::inputText), resolutionValue, context->pixelSizeX(), context->pixelSizeY(),
                                 displaySizeInfo.x(), displaySizeInfo.y());
      break;
    }
    case SPRITE_SAME_TEXTURE_ID: {
      allowSpriteSettings = !useTextureSettingsForSprites;
      break;
    }
    default: assert(false); break;
  }
}


// -- rendering -- -------------------------------------------------------------

void SmoothingUpscaling::drawIcons() {
  // scrollable geometry
  buffers->bindScrollLocationBuffer(context->renderer(), ScissorRectangle(x(), y(), width(), contentHeight()));

  auto* hoverControl = getActiveControl();
  spriteTextureSettings.drawIcon(*context, *buffers, (hoverControl == &spriteTextureSettings));
}

void SmoothingUpscaling::drawPageBackgrounds(int32_t mouseX, int32_t mouseY) {
  // scrollable geometry
  if (buffers->isFixedLocationBuffer())
    buffers->bindScrollLocationBuffer(context->renderer(), ScissorRectangle(x(), y(), width(), contentHeight()));

  framebufferGroup.drawBackground(*context, *buffers);
  textureGroup.drawBackground(*context, *buffers);
  spriteGroup.drawBackground(*context, *buffers);
  screenGroup.drawBackground(*context, *buffers);

  auto* hoverControl = getActiveControl();
  internalResolutionX.drawBackground(*context, mouseX, mouseY, *buffers, (hoverControl == &internalResolutionX));
  internalResolutionY.drawBackground(*context, mouseX, mouseY, *buffers, (hoverControl == &internalResolutionY));

  mdecMovieFilter.drawBackground(*context, *buffers, (hoverControl == &mdecMovieFilter));
  antiAliasing.drawBackground(*context, *buffers, (hoverControl == &antiAliasing));
  screenUpscaling.drawBackground(*context, *buffers, (hoverControl == &screenUpscaling));
  screenUpscalingFactor.drawBackground(*context, *buffers, (hoverControl == &screenUpscalingFactor));
  textureUpscaling.drawBackground(*context, *buffers, (hoverControl == &textureUpscaling));
  textureUpscalingFactor.drawBackground(*context, *buffers, (hoverControl == &textureUpscalingFactor));
  textureSmoothing.drawBackground(*context, *buffers, (hoverControl == &textureSmoothing));
  spriteUpscaling.drawBackground(*context, *buffers, (hoverControl == &spriteUpscaling));
  spriteUpscalingFactor.drawBackground(*context, *buffers, (hoverControl == &spriteUpscalingFactor));
  spriteSmoothing.drawBackground(*context, *buffers, (hoverControl == &spriteSmoothing));

  textureGrain.drawBackground(*context, mouseX, *buffers, (hoverControl == &textureGrain));
  spriteGrain.drawBackground(*context, mouseX, *buffers, (hoverControl == &spriteGrain));
  screenGrain.drawBackground(*context, mouseX, *buffers, (hoverControl == &screenGrain));
  textureGrain.drawBackground(*context, mouseX, *buffers, (hoverControl == &textureGrain));
  screenBlur.drawBackground(*context, *buffers);
  textureSplatting.drawBackground(*context, *buffers);
}

void SmoothingUpscaling::drawPageLabels() {
  // scrollable geometry
  auto& renderer = context->renderer();
  if (buffers->isFixedLocationBuffer())
    buffers->bindScrollLocationBuffer(renderer, ScissorRectangle(x(), y(), width(), contentHeight()));

  buffers->bindLabelBuffer(renderer, LabelBufferType::title);
  title.draw(renderer);
  buffers->bindLabelBuffer(renderer, LabelBufferType::textInputDisabled);
  framebufferResolutionInfo.draw(renderer);
  displaySizeInfo.draw(renderer);

  framebufferGroup.drawLabel(*context, *buffers);
  textureGroup.drawLabel(*context, *buffers);
  spriteGroup.drawLabel(*context, *buffers);
  screenGroup.drawLabel(*context, *buffers);

  auto* hoverControl = getActiveControl();
  internalResolutionX.drawLabels(*context, *buffers, (hoverControl == &internalResolutionX));
  internalResolutionY.drawLabels(*context, *buffers, (hoverControl == &internalResolutionY));
  spriteTextureSettings.drawLabel(*context, *buffers, (hoverControl == &spriteTextureSettings));

  mdecMovieFilter.drawLabels(*context, *buffers, (hoverControl == &mdecMovieFilter));
  antiAliasing.drawLabels(*context, *buffers, (hoverControl == &antiAliasing));
  screenUpscaling.drawLabels(*context, *buffers, (hoverControl == &screenUpscaling));
  screenUpscalingFactor.drawLabels(*context, *buffers, (hoverControl == &screenUpscalingFactor));
  textureUpscaling.drawLabels(*context, *buffers, (hoverControl == &textureUpscaling));
  textureUpscalingFactor.drawLabels(*context, *buffers, (hoverControl == &textureUpscalingFactor));
  textureSmoothing.drawLabels(*context, *buffers, (hoverControl == &textureSmoothing));
  spriteUpscaling.drawLabels(*context, *buffers, (hoverControl == &spriteUpscaling));
  spriteUpscalingFactor.drawLabels(*context, *buffers, (hoverControl == &spriteUpscalingFactor));
  spriteSmoothing.drawLabels(*context, *buffers, (hoverControl == &spriteSmoothing));

  screenGrain.drawLabels(*context, *buffers, (hoverControl == &screenGrain));
  textureGrain.drawLabels(*context, *buffers, (hoverControl == &textureGrain));
  spriteGrain.drawLabels(*context, *buffers, (hoverControl == &spriteGrain));
  screenBlur.drawLabels(*context, *buffers, (hoverControl == &screenBlur));
  textureSplatting.drawLabels(*context, *buffers, (hoverControl == &textureSplatting));
}
