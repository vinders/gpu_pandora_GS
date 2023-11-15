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
#include "menu/pages/smoothing_upscaling.h"

using namespace video_api;
using namespace pandora::hardware;
using namespace display;
using namespace display::controls;
using namespace menu::controls;
using namespace menu::pages;
using namespace menu;


// -- helpers -- ---------------------------------------------------------------

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

#define INT_RES_VALUE_BUFFER_SIZE 64
#define MAX_FRAMEBUFFER_SIZE_X 640u
#define MAX_FRAMEBUFFER_SIZE_Y 256u

static void fillInternalResolutionValue(uint32_t resX, uint32_t resY, const MessageResources& localizedText,
                                        char16_t outValue[INT_RES_VALUE_BUFFER_SIZE]) {
  outValue = copyString(localizedText.getMessage(SmoothingUpscalingMessages::internalResolution_prefix), outValue);
  outValue = resolutionToString(resX*320u, outValue);
  *outValue = u'x';
  outValue = resolutionToString(resY*224u, outValue + 1);
  outValue = copyString(localizedText.getMessage(CommonMessages::to), outValue);
  outValue = resolutionToString(resX*MAX_FRAMEBUFFER_SIZE_X, outValue);
  *outValue = u'x';
  outValue = resolutionToString(resY*MAX_FRAMEBUFFER_SIZE_Y, outValue + 1);
  outValue = copyString(localizedText.getMessage(SmoothingUpscalingMessages::internalResolution_suffix), outValue);
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
  outValue = copyString(localizedText.getMessage(SmoothingUpscalingMessages::internalResolution_suffix), outValue);
  *outValue = u'\0';
}

#define UPSCALING_MODES_ARRAY_SIZE 7
#define UPSCALING_FACTORS_MAX_ARRAY_SIZE 4

static void fillUpscalingModes(const char16_t* disabledLabel, ComboBoxOption options[UPSCALING_MODES_ARRAY_SIZE]) {
  *options = ComboBoxOption(disabledLabel, 0/*TMP*/);
  *(++options) = ComboBoxOption(u"SaI", 1/*TMP*/);
  *(++options) = ComboBoxOption(u"HQx", 2/*TMP*/);
  *(++options) = ComboBoxOption(u"xBR", 3/*TMP*/);
  *(++options) = ComboBoxOption(u"xBRZ (slower)", 4/*TMP*/);
  *(++options) = ComboBoxOption(u"Super-xBR (slower)", 5/*TMP*/);
  *(++options) = ComboBoxOption(u"NNEDI3 (slower)",    6/*TMP*/);
}

static size_t fillUpscalingFactors(uint32_t upscalingMode, ComboBoxOption options[UPSCALING_FACTORS_MAX_ARRAY_SIZE]) {
  switch (upscalingMode) {
    case 1: // SaI
    case 5: // Super-xBR
    case 6: // NNEDI3
      *options = ComboBoxOption(u"2x", 2);
      *(++options) = ComboBoxOption(u"4x", 4);
      *(++options) = ComboBoxOption(u"8x", 8);
      return 3;
    case 2: // HQx
    case 3: // xBR
      *options = ComboBoxOption(u"2x", 2);
      *(++options) = ComboBoxOption(u"3x", 3);
      *(++options) = ComboBoxOption(u"4x", 4);
      return 3;
    case 4: // xBRZ
      *options = ComboBoxOption(u"2x", 2);
      *(++options) = ComboBoxOption(u"3x", 3);
      *(++options) = ComboBoxOption(u"4x", 4);
      *(++options) = ComboBoxOption(u"5x", 5);
      return 4;
    case 0: // OFF
    default:
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

void SmoothingUpscaling::init(const ColorTheme& theme, int32_t x, int32_t y, uint32_t width) {
  const uint32_t fieldsetPaddingX = Control::fieldsetMarginX(width);
  const int32_t controlX = x + (int32_t)fieldsetPaddingX + (int32_t)Control::fieldsetContentMarginX(width);
  uint32_t fieldsetWidth = width - (fieldsetPaddingX << 1);
  if (fieldsetWidth > Control::fieldsetMaxWidth())
    fieldsetWidth = Control::fieldsetMaxWidth();

  title = TextMesh(context->renderer(), context->getFont(FontType::titles), localizedText->getMessage(SmoothingUpscalingMessages::title),
                   context->pixelSizeX(), context->pixelSizeY(), x + (int32_t)fieldsetPaddingX, y + Control::titleMarginTop(), TextAlignment::left);

  std::vector<ControlRegistration> registry;
  registry.reserve(17);
  int32_t currentLineY = title.y() + (int32_t)title.height() + Control::pageLineHeight();
  auto changeHandler = std::bind(&SmoothingUpscaling::onChange,this,std::placeholders::_1,std::placeholders::_2);

  // --- framebuffer group ---
  frameBufferGroup = Fieldset(*context, localizedText->getMessage(SmoothingUpscalingMessages::framebufferGroup), theme.fieldsetStyle(),
                              theme.fieldsetControlColor(), x + (int32_t)fieldsetPaddingX, currentLineY, fieldsetWidth,
                              Control::fieldsetContentHeight(2) + Control::pageLineHeight() - (Control::pageLineHeight() >> 2));
  currentLineY += Control::pageLineHeight() + Control::fieldsetContentPaddingTop();

  auto internalResChangeHandler = std::bind(&SmoothingUpscaling::onInternalResolutionChange,this,std::placeholders::_1);
  internalResolutionX = TextBox(*context, localizedText->getMessage(SmoothingUpscalingMessages::internalResolution), nullptr,
                                controlX, currentLineY, Control::pageLabelWidth(), 80, theme.textBoxControlColor(),
                                INTERNAL_RES_X_ID, internalResChangeHandler, (uint32_t)4u, 2u, true);
  registry.emplace_back(internalResolutionX, true, localizedText->getMessage(SmoothingUpscalingMessages::internalResolution_tooltip));
  internalResolutionY = TextBox(*context, nullptr, nullptr, internalResolutionX.x() + (int32_t)internalResolutionX.width() + 3,
                                currentLineY, 0, 80, theme.textBoxControlColor(), INTERNAL_RES_Y_ID, internalResChangeHandler, (uint32_t)4u, 2u, true);
  registry.emplace_back(internalResolutionY, true, localizedText->getMessage(SmoothingUpscalingMessages::internalResolution_tooltip));
  currentLineY += Control::pageLineHeight() - (Control::pageLineHeight() >> 2);;

  char16_t resolutionValue[INT_RES_VALUE_BUFFER_SIZE];
  fillInternalResolutionValue(internalResolutionX.valueInteger(), internalResolutionY.valueInteger(), *localizedText, resolutionValue);
  internalResolutionValue = TextMesh(context->renderer(), context->getFont(FontType::inputText), resolutionValue, context->pixelSizeX(), context->pixelSizeY(),
                                     internalResolutionX.x() + (int32_t)Control::pageLabelWidth() + (int32_t)Control::labelMargin(), currentLineY + 1);
  currentLineY += Control::pageLineHeight();

  isMdecFilter = true;
  mdecMovieFilter = CheckBox(*context, localizedText->getMessage(SmoothingUpscalingMessages::mdecMovieFilter), controlX,
                             currentLineY, Control::pageLabelWidth(), 0, nullptr, isMdecFilter);
  registry.emplace_back(mdecMovieFilter, true, localizedText->getMessage(SmoothingUpscalingMessages::mdecMovieFilter_tooltip));
  currentLineY += Control::pageLineHeight() + Control::fieldsetContentMarginBottom();

  // --- screen group ---
  screenGroup = Fieldset(*context, localizedText->getMessage(SmoothingUpscalingMessages::screenGroup),
                         theme.fieldsetStyle(), theme.fieldsetControlColor(), x + (int32_t)fieldsetPaddingX,
                         currentLineY, fieldsetWidth, Control::fieldsetContentHeight(3) + Control::pageLineHeight() - (Control::pageLineHeight() >> 2));
  currentLineY += Control::pageLineHeight() + Control::fieldsetContentPaddingTop();

  ComboBoxOption upscalingOptions[UPSCALING_MODES_ARRAY_SIZE];
  constexpr const uint32_t upscalingControlWidth = Control::pageControlWidth() - (Control::pageControlWidth() >> 2) - 3u;
  constexpr const uint32_t smoothingGrainWidth = upscalingControlWidth + Control::comboBoxPaddingY();
  fillUpscalingModes(localizedText->getMessage(CommonMessages::disabled), upscalingOptions);
  screenUpscaling = ComboBox(*context, localizedText->getMessage(SmoothingUpscalingMessages::screenUpscaling), controlX,
                             currentLineY, Control::pageLabelWidth(), upscalingControlWidth, ComboBoxStyle::classic,
                             theme.comboBoxColorParams(), SCREEN_UPSCALING_ID, changeHandler, upscalingOptions,
                             sizeof(upscalingOptions)/sizeof(*upscalingOptions), 0);
  registry.emplace_back(screenUpscaling, true, localizedText->getMessage(SmoothingUpscalingMessages::upscaling_tooltip));
  {
    ComboBoxOption scaleOptions[UPSCALING_FACTORS_MAX_ARRAY_SIZE];
    size_t scaleOptionsCount = fillUpscalingFactors((screenUpscaling.getSelectedIndex() >= 0) ? *screenUpscaling.getSelectedValue() : 0, scaleOptions);
    screenUpscalingFactor = ComboBox(*context, nullptr, screenUpscaling.x() + (int32_t)screenUpscaling.width() + 3,
                                     currentLineY, 0, (Control::pageControlWidth() >> 2), ComboBoxStyle::cutCorner,
                                     theme.comboBoxColorParams(), SCREEN_UPSCALING_FACTOR_ID, changeHandler, scaleOptions, scaleOptionsCount, 0);
    registry.emplace_back(screenUpscalingFactor, true, localizedText->getMessage(SmoothingUpscalingMessages::upscaling_tooltip), 3);
  }
  currentLineY += Control::pageLineHeight() - (Control::pageLineHeight() >> 2);;

  const auto* factorValue = screenUpscalingFactor.getSelectedValue();
  fillDisplaySizeValue(internalResolutionX.valueInteger(), internalResolutionY.valueInteger(),
                       factorValue ? *factorValue : 1, *localizedText, resolutionValue);
  displaySize = TextMesh(context->renderer(), context->getFont(FontType::inputText), resolutionValue, context->pixelSizeX(), context->pixelSizeY(),
                         screenUpscaling.x() + (int32_t)Control::pageLabelWidth() + (int32_t)Control::labelMargin(), currentLineY + 2);
  currentLineY += Control::pageLineHeight();

  ComboBoxOption grainOptions[]{ ComboBoxOption(localizedText->getMessage(CommonMessages::disabled), 0/*TMP*/),
                                 ComboBoxOption(localizedText->getMessage(SmoothingUpscalingMessages::grain_photo), 1/*TMP*/),
                                 ComboBoxOption(localizedText->getMessage(SmoothingUpscalingMessages::grain_gauss), 2/*TMP*/) };
  screenGrain = Slider(*context, localizedText->getMessage(SmoothingUpscalingMessages::screenGrain), controlX, currentLineY,
                       Control::pageLabelWidth(), smoothingGrainWidth, theme.sliderArrowColor(), 0, nullptr,
                       grainOptions, sizeof(grainOptions)/sizeof(*grainOptions), 0);
  registry.emplace_back(screenGrain, true, localizedText->getMessage(SmoothingUpscalingMessages::grain_tooltip));
  currentLineY += Control::pageLineHeight();

  screenBlurValue = 0;
  screenBlur = Ruler(*context, localizedText->getMessage(SmoothingUpscalingMessages::screenBlur), nullptr,
                     FontType::labels, TextAlignment::left, controlX, currentLineY, Control::pageLabelWidth(),
                     (Control::pageControlWidth() >> 2) - 1u, theme.rulerColorParams(), 0, nullptr, 0, 3, 1, screenBlurValue);
  registry.emplace_back(screenBlur, true, localizedText->getMessage(SmoothingUpscalingMessages::screenBlur_tooltip));
  currentLineY += Control::pageLineHeight() + Control::fieldsetContentMarginBottom();

  // --- texture group ---
  textureGroup = Fieldset(*context, localizedText->getMessage(SmoothingUpscalingMessages::textureGroup),
                          theme.fieldsetStyle(), theme.fieldsetControlColor(), x + (int32_t)fieldsetPaddingX,
                          currentLineY, fieldsetWidth, Control::fieldsetContentHeight(4));
  currentLineY += Control::pageLineHeight() + Control::fieldsetContentPaddingTop();

  textureUpscaling = ComboBox(*context, localizedText->getMessage(SmoothingUpscalingMessages::textureUpscaling), controlX,
                              currentLineY, Control::pageLabelWidth(), upscalingControlWidth, ComboBoxStyle::classic,
                              theme.comboBoxColorParams(), TEXTURE_UPSCALING_ID, changeHandler, upscalingOptions,
                              sizeof(upscalingOptions)/sizeof(*upscalingOptions), 0);
  registry.emplace_back(textureUpscaling, true, localizedText->getMessage(SmoothingUpscalingMessages::upscaling_tooltip));
  {
    ComboBoxOption scaleOptions[UPSCALING_FACTORS_MAX_ARRAY_SIZE];
    size_t scaleOptionsCount = fillUpscalingFactors((textureUpscaling.getSelectedIndex() >= 0) ? *textureUpscaling.getSelectedValue() : 0, scaleOptions);
    textureUpscalingFactor = ComboBox(*context, nullptr, textureUpscaling.x() + (int32_t)textureUpscaling.width() + 3,
                                      currentLineY, 0, (Control::pageControlWidth() >> 2), ComboBoxStyle::cutCorner,
                                      theme.comboBoxColorParams(), 0, nullptr, scaleOptions, scaleOptionsCount, 0);
    registry.emplace_back(textureUpscalingFactor, true, localizedText->getMessage(SmoothingUpscalingMessages::upscaling_tooltip), 3);
    currentLineY += Control::pageLineHeight();
  }

  ComboBoxOption smoothingOptions[]{ ComboBoxOption(localizedText->getMessage(SmoothingUpscalingMessages::smoothing_nearest), 0/*TMP*/),
                                     ComboBoxOption(localizedText->getMessage(SmoothingUpscalingMessages::smoothing_bilinear), 1/*TMP*/),
                                     ComboBoxOption(localizedText->getMessage(SmoothingUpscalingMessages::smoothing_bicubic), 2/*TMP*/),
                                     ComboBoxOption(localizedText->getMessage(SmoothingUpscalingMessages::smoothing_lanczos), 3/*TMP*/) };
  textureSmoothing = ComboBox(*context, localizedText->getMessage(SmoothingUpscalingMessages::textureSmoothing), controlX,
                              currentLineY, Control::pageLabelWidth(), smoothingGrainWidth,
                              ComboBoxStyle::cutCorner, theme.comboBoxColorParams(), 0, nullptr, smoothingOptions,
                              sizeof(smoothingOptions)/sizeof(*smoothingOptions), 1);
  registry.emplace_back(textureSmoothing, true, localizedText->getMessage(SmoothingUpscalingMessages::smoothing_tooltip));
  currentLineY += Control::pageLineHeight();
  
  textureGrain = Slider(*context, localizedText->getMessage(SmoothingUpscalingMessages::textureGrain), controlX, currentLineY,
                        Control::pageLabelWidth(), smoothingGrainWidth, theme.sliderArrowColor(), 0, nullptr,
                        grainOptions, sizeof(grainOptions)/sizeof(*grainOptions), 0);
  registry.emplace_back(textureGrain, true, localizedText->getMessage(SmoothingUpscalingMessages::grain_tooltip));
  currentLineY += Control::pageLineHeight();

  textureSplattingValue = 0;
  textureSplatting = Ruler(*context, localizedText->getMessage(SmoothingUpscalingMessages::textureSplatting), nullptr,
                           FontType::labels, TextAlignment::left, controlX, currentLineY, Control::pageLabelWidth(),
                           (Control::pageControlWidth() >> 2) - 1u, theme.rulerColorParams(), 0, nullptr, 0, 3, 1, textureSplattingValue);
  registry.emplace_back(textureSplatting, true, localizedText->getMessage(SmoothingUpscalingMessages::textureSplatting_tooltip));
  currentLineY += Control::pageLineHeight() + Control::fieldsetContentMarginBottom();

  // --- sprite group ---
  spriteGroup = Fieldset(*context, localizedText->getMessage(SmoothingUpscalingMessages::spriteGroup),
                         theme.fieldsetStyle(), theme.fieldsetControlColor(), x + (int32_t)fieldsetPaddingX,
                         currentLineY, fieldsetWidth, Control::fieldsetContentHeight(4));
  currentLineY += Control::pageLineHeight() + Control::fieldsetContentPaddingTop();

  useTextureSettingsForSprites = true;
  spriteTextureSettings = CheckBox(*context, localizedText->getMessage(SmoothingUpscalingMessages::spriteSameAsTexture), controlX,
                                   currentLineY, Control::pageLabelWidth(), 0, [this](uint32_t, uint32_t) {
                                     this->allowSpriteSettings = !this->useTextureSettingsForSprites;
                                   }, useTextureSettingsForSprites);
  registry.emplace_back(spriteTextureSettings, true, localizedText->getMessage(SmoothingUpscalingMessages::spriteSameAsTexture_tooltip));
  currentLineY += Control::pageLineHeight();

  spriteUpscaling = ComboBox(*context, localizedText->getMessage(SmoothingUpscalingMessages::spriteUpscaling), controlX,
                             currentLineY, Control::pageLabelWidth(), upscalingControlWidth, ComboBoxStyle::classic,
                             theme.comboBoxColorParams(), SPRITE_UPSCALING_ID, changeHandler, upscalingOptions,
                             sizeof(upscalingOptions)/sizeof(*upscalingOptions), 0, &allowSpriteSettings);
  registry.emplace_back(spriteUpscaling, true, localizedText->getMessage(SmoothingUpscalingMessages::upscaling_tooltip));
  {
    ComboBoxOption scaleOptions[UPSCALING_FACTORS_MAX_ARRAY_SIZE];
    size_t scaleOptionsCount = fillUpscalingFactors((spriteUpscaling.getSelectedIndex() >= 0) ? *spriteUpscaling.getSelectedValue() : 0, scaleOptions);
    spriteUpscalingFactor = ComboBox(*context, nullptr, spriteUpscaling.x() + (int32_t)spriteUpscaling.width() + 3,
                                     currentLineY, 0, (Control::pageControlWidth() >> 2), ComboBoxStyle::cutCorner,
                                     theme.comboBoxColorParams(), 0, nullptr, scaleOptions, scaleOptionsCount, 0, &allowSpriteSettings);
    registry.emplace_back(spriteUpscalingFactor, true, localizedText->getMessage(SmoothingUpscalingMessages::upscaling_tooltip), 3);
    currentLineY += Control::pageLineHeight();
  }

  spriteSmoothing = ComboBox(*context, localizedText->getMessage(SmoothingUpscalingMessages::spriteSmoothing), controlX,
                             currentLineY, Control::pageLabelWidth(), smoothingGrainWidth,
                             ComboBoxStyle::cutCorner, theme.comboBoxColorParams(), 0, nullptr, smoothingOptions,
                             sizeof(smoothingOptions)/sizeof(*smoothingOptions), 1, &allowSpriteSettings);
  registry.emplace_back(spriteSmoothing, true, localizedText->getMessage(SmoothingUpscalingMessages::smoothing_tooltip));
  currentLineY += Control::pageLineHeight();

  spriteGrain = Slider(*context, localizedText->getMessage(SmoothingUpscalingMessages::spriteGrain), controlX, currentLineY,
                       Control::pageLabelWidth(), smoothingGrainWidth, theme.sliderArrowColor(), 0, nullptr,
                       grainOptions, sizeof(grainOptions)/sizeof(*grainOptions), 0, &allowSpriteSettings);
  registry.emplace_back(spriteGrain, true, localizedText->getMessage(SmoothingUpscalingMessages::grain_tooltip));
  currentLineY += Control::pageLineHeight();//+ Control::fieldsetContentMarginBottom();

  // --- control registry ---
  if (currentLineY > y + (int32_t)contentHeight())
    Page::moveScrollbarThumb(currentLineY);
  registerControls(std::move(registry));
}

SmoothingUpscaling::~SmoothingUpscaling() noexcept {
  localizedText = nullptr;
  title.release();

  frameBufferGroup.release();
  internalResolutionX.release();
  internalResolutionY.release();
  internalResolutionValue.release();
  mdecMovieFilter.release();

  screenGroup.release();
  screenUpscaling.release();
  screenUpscalingFactor.release();
  displaySize.release();
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
  const uint32_t fieldsetPaddingX = Control::fieldsetMarginX(width);
  const int32_t controlX = x + (int32_t)fieldsetPaddingX + (int32_t)Control::fieldsetContentMarginX(width);
  uint32_t fieldsetWidth = width - (fieldsetPaddingX << 1);
  if (fieldsetWidth > Control::fieldsetMaxWidth())
    fieldsetWidth = Control::fieldsetMaxWidth();

  title.move(context->renderer(), context->pixelSizeX(), context->pixelSizeY(), x + (int32_t)fieldsetPaddingX, y + Control::titleMarginTop());

  //TODO:
  // - refactor -> avoir méthodes dans PageBuilder -> addComboBox, addTextBox, addFieldset, addTitle... (et y stocker context + currentLineY + changeHandler + theme + registry)
  //            -> éviter de réécrire tout dans chaque page (sauf cas particuliers comme stretching)
  //            -> idem pour moveComboBox, moveTextBox... (mais inline -> autre classe pour ne pas avoir handler/theme/registry ?)
  //            -> y stocker aussi les helpers communs en static (intToStr, ...)
  //            -> page_builder.h
  // - draw foreground -> accès openControl pour n'appeler que lui?
  // - main/frame: n'appeler draw que si invalidé (event mouse/key/window)

  // framebuffer group
  int32_t currentLineY = title.y() + (int32_t)title.height() + Control::pageLineHeight();
  frameBufferGroup.move(*context, x + (int32_t)fieldsetPaddingX, currentLineY, fieldsetWidth,
                        Control::fieldsetContentHeight(2) + Control::pageLineHeight() - (Control::pageLineHeight() >> 2));
  currentLineY += Control::pageLineHeight() + Control::fieldsetContentPaddingTop();

  internalResolutionX.move(*context, controlX, currentLineY);
  internalResolutionY.move(*context, internalResolutionX.x() + (int32_t)internalResolutionX.width() + 3, currentLineY);
  currentLineY += Control::pageLineHeight() - (Control::pageLineHeight() >> 2);
  internalResolutionValue.move(context->renderer(), context->pixelSizeX(), context->pixelSizeY(),
                               internalResolutionX.x() + (int32_t)Control::pageLabelWidth() + (int32_t)Control::labelMargin(), currentLineY + 1);
  currentLineY += Control::pageLineHeight();
  mdecMovieFilter.move(*context, controlX, currentLineY);
  currentLineY += Control::pageLineHeight() + Control::fieldsetContentMarginBottom();

  // screen group
  screenGroup.move(*context, x + (int32_t)fieldsetPaddingX, currentLineY, fieldsetWidth,
                   Control::fieldsetContentHeight(3) + Control::pageLineHeight() - (Control::pageLineHeight() >> 2));
  currentLineY += Control::pageLineHeight() + Control::fieldsetContentPaddingTop();

  screenUpscaling.move(*context, controlX, currentLineY);
  screenUpscalingFactor.move(*context, screenUpscaling.x() + (int32_t)screenUpscaling.width() + 3, currentLineY);
  currentLineY += Control::pageLineHeight() - (Control::pageLineHeight() >> 2);
  displaySize.move(context->renderer(), context->pixelSizeX(), context->pixelSizeY(),
                   screenUpscaling.x() + (int32_t)Control::pageLabelWidth() + (int32_t)Control::labelMargin(), currentLineY + 2);
  currentLineY += Control::pageLineHeight();
  screenGrain.move(*context, controlX, currentLineY);
  currentLineY += Control::pageLineHeight();
  screenBlur.move(*context, controlX, currentLineY, TextAlignment::left);
  currentLineY += Control::pageLineHeight() + Control::fieldsetContentMarginBottom();

  // texture group
  textureGroup.move(*context, x + (int32_t)fieldsetPaddingX, currentLineY, fieldsetWidth, Control::fieldsetContentHeight(4));
  currentLineY += Control::pageLineHeight() + Control::fieldsetContentPaddingTop();

  textureUpscaling.move(*context, controlX, currentLineY);
  textureUpscalingFactor.move(*context, textureUpscaling.x() + (int32_t)textureUpscaling.width() + 3, currentLineY);
  currentLineY += Control::pageLineHeight();
  textureSmoothing.move(*context, controlX, currentLineY);
  currentLineY += Control::pageLineHeight();
  textureGrain.move(*context, controlX, currentLineY);
  currentLineY += Control::pageLineHeight();
  textureSplatting.move(*context, controlX, currentLineY, TextAlignment::left);
  currentLineY += Control::pageLineHeight() + Control::fieldsetContentMarginBottom();

  // sprite group
  spriteGroup.move(*context, x + (int32_t)fieldsetPaddingX, currentLineY, fieldsetWidth, Control::fieldsetContentHeight(4));
  currentLineY += Control::pageLineHeight() + Control::fieldsetContentPaddingTop();

  spriteTextureSettings.move(*context, controlX, currentLineY);
  currentLineY += Control::pageLineHeight();
  spriteUpscaling.move(*context, controlX, currentLineY);
  spriteUpscalingFactor.move(*context, spriteUpscaling.x() + (int32_t)spriteUpscaling.width() + 3, currentLineY);
  currentLineY += Control::pageLineHeight();
  spriteSmoothing.move(*context, controlX, currentLineY);
  currentLineY += Control::pageLineHeight();
  spriteGrain.move(*context, controlX, currentLineY);
  currentLineY += Control::pageLineHeight();//+ Control::fieldsetContentMarginBottom();

  Page::moveScrollbarThumb(currentLineY); // required after a move
}

void SmoothingUpscaling::onChange(uint32_t id, uint32_t value) {
  ComboBoxOption scaleOptions[UPSCALING_FACTORS_MAX_ARRAY_SIZE];
  switch (id) {
    case SCREEN_UPSCALING_ID: {
      const auto* currentValue = screenUpscalingFactor.getSelectedValue();
      size_t scaleOptionsCount = fillUpscalingFactors((screenUpscaling.getSelectedIndex() >= 0) ? *screenUpscaling.getSelectedValue() : 0, scaleOptions);
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
      onChange(SCREEN_UPSCALING_FACTOR_ID, factorValue ? *factorValue : 1);
      break;
    }
    case TEXTURE_UPSCALING_ID: {
      const auto* currentValue = textureUpscalingFactor.getSelectedValue();
      size_t scaleOptionsCount = fillUpscalingFactors((textureUpscaling.getSelectedIndex() >= 0) ? *textureUpscaling.getSelectedValue() : 0, scaleOptions);
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
      size_t scaleOptionsCount = fillUpscalingFactors((spriteUpscaling.getSelectedIndex() >= 0) ? *spriteUpscaling.getSelectedValue() : 0, scaleOptions);
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
      uint32_t upscalingFactor = (screenUpscalingFactor.getSelectedIndex() >= 0) ? *screenUpscalingFactor.getSelectedValue() : 1;
      if (upscalingFactor > MAX_TEXTURE_SIZE / (internalResolutionX.valueInteger() * MAX_FRAMEBUFFER_SIZE_X))
        internalResolutionX.replaceValueInteger(*context, MAX_TEXTURE_SIZE / (upscalingFactor * MAX_FRAMEBUFFER_SIZE_X));
      if (upscalingFactor > MAX_TEXTURE_SIZE / (internalResolutionY.valueInteger() * MAX_FRAMEBUFFER_SIZE_Y))
        internalResolutionY.replaceValueInteger(*context, MAX_TEXTURE_SIZE / (upscalingFactor * MAX_FRAMEBUFFER_SIZE_Y));

      char16_t resolutionValue[INT_RES_VALUE_BUFFER_SIZE];
      fillDisplaySizeValue(internalResolutionX.valueInteger(), internalResolutionY.valueInteger(),
                           value, *localizedText, resolutionValue);
      displaySize = TextMesh(context->renderer(), context->getFont(FontType::inputText), resolutionValue, context->pixelSizeX(), context->pixelSizeY(),
                             displaySize.x(), displaySize.y());
      break;
    }
    default: assert(false); break;
  }
}

#define MAX_INT_RES_X  (uint32_t)MAX_TEXTURE_SIZE/2048u
#define MAX_INT_RES_Y  (uint32_t)MAX_TEXTURE_SIZE/1024u

void SmoothingUpscaling::onInternalResolutionChange(uint32_t id) {
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
  fillInternalResolutionValue(resX, resY, *localizedText, resolutionValue);
  internalResolutionValue = TextMesh(context->renderer(), context->getFont(FontType::inputText),
                                     resolutionValue, context->pixelSizeX(), context->pixelSizeY(),
                                     internalResolutionValue.x(), internalResolutionValue.y());

  const auto* factorValue = screenUpscalingFactor.getSelectedValue();
  fillDisplaySizeValue(internalResolutionX.valueInteger(), internalResolutionY.valueInteger(),
                       factorValue ? *factorValue : 1, *localizedText, resolutionValue);
  displaySize = TextMesh(context->renderer(), context->getFont(FontType::inputText), resolutionValue, context->pixelSizeX(), context->pixelSizeY(),
                         displaySize.x(), displaySize.y());
}


// -- rendering -- -------------------------------------------------------------

void SmoothingUpscaling::drawIcons() {
  // scrollable geometry
  buffers->bindScrollLocationBuffer(context->renderer(), ScissorRectangle(x(), y(), width(), contentHeight()));

  auto* hoverControl = getActiveControl();
  mdecMovieFilter.drawIcon(*context, *buffers, (hoverControl == &mdecMovieFilter));
  spriteTextureSettings.drawIcon(*context, *buffers, (hoverControl == &spriteTextureSettings));
}

bool SmoothingUpscaling::drawPageBackgrounds(int32_t mouseX, int32_t mouseY) {
  // scrollable geometry
  if (buffers->isFixedLocationBuffer())
    buffers->bindScrollLocationBuffer(context->renderer(), ScissorRectangle(x(), y(), width(), contentHeight()));

  frameBufferGroup.drawBackground(*context, *buffers);
  textureGroup.drawBackground(*context, *buffers);
  spriteGroup.drawBackground(*context, *buffers);
  screenGroup.drawBackground(*context, *buffers);

  auto* hoverControl = getActiveControl();
  internalResolutionX.drawBackground(*context, mouseX, mouseY, *buffers, (hoverControl == &internalResolutionX));
  internalResolutionY.drawBackground(*context, mouseX, mouseY, *buffers, (hoverControl == &internalResolutionY));

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
  return false;
}

void SmoothingUpscaling::drawPageLabels() {
  // scrollable geometry
  auto& renderer = context->renderer();
  if (buffers->isFixedLocationBuffer())
    buffers->bindScrollLocationBuffer(renderer, ScissorRectangle(x(), y(), width(), contentHeight()));

  buffers->bindLabelBuffer(renderer, LabelBufferType::title);
  title.draw(renderer);
  buffers->bindLabelBuffer(renderer, LabelBufferType::textInputDisabled);
  internalResolutionValue.draw(renderer);
  displaySize.draw(renderer);

  frameBufferGroup.drawLabel(*context, *buffers);
  textureGroup.drawLabel(*context, *buffers);
  spriteGroup.drawLabel(*context, *buffers);
  screenGroup.drawLabel(*context, *buffers);

  auto* hoverControl = getActiveControl();
  internalResolutionX.drawLabels(*context, *buffers, (hoverControl == &internalResolutionX));
  internalResolutionY.drawLabels(*context, *buffers, (hoverControl == &internalResolutionY));
  mdecMovieFilter.drawLabel(*context, *buffers, (hoverControl == &mdecMovieFilter));
  spriteTextureSettings.drawLabel(*context, *buffers, (hoverControl == &spriteTextureSettings));

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

void SmoothingUpscaling::drawForegrounds() {
  auto& renderer = context->renderer();

  ScissorRectangle fullWindowArea(0, 0, context->clientWidth(), context->clientHeight());
  buffers->bindScrollLocationBuffer(renderer, fullWindowArea); // visible outside of scroll area -> full window

  screenUpscaling.drawDropdown(*context, *buffers);
  screenUpscalingFactor.drawDropdown(*context, *buffers);
  textureUpscaling.drawDropdown(*context, *buffers);
  textureUpscalingFactor.drawDropdown(*context, *buffers);
  textureSmoothing.drawDropdown(*context, *buffers);
  spriteUpscaling.drawDropdown(*context, *buffers);
  spriteUpscalingFactor.drawDropdown(*context, *buffers);
  spriteSmoothing.drawDropdown(*context, *buffers);
}

void SmoothingUpscaling::drawForegroundLabels() {
  ScissorRectangle fullWindowArea(0, 0, context->clientWidth(), context->clientHeight());
  buffers->bindScrollLocationBuffer(context->renderer(), fullWindowArea); // visible outside of scroll area -> full window

  screenUpscaling.drawOptions(*context, *buffers);
  screenUpscalingFactor.drawOptions(*context, *buffers);
  textureUpscaling.drawOptions(*context, *buffers);
  textureUpscalingFactor.drawOptions(*context, *buffers);
  textureSmoothing.drawOptions(*context, *buffers);
  spriteUpscaling.drawOptions(*context, *buffers);
  spriteUpscalingFactor.drawOptions(*context, *buffers);
  spriteSmoothing.drawOptions(*context, *buffers);
}
