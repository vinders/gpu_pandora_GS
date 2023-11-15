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
#include <vector>
#include <unordered_map>
#include "menu/controls/geometry_generator.h"
#include "menu/pages/general_settings.h"

using namespace video_api;
using namespace pandora::hardware;
using namespace display;
using namespace display::controls;
using namespace menu::controls;
using namespace menu::pages;
using namespace menu;

#if !defined(_CPP_REVISION) || _CPP_REVISION != 14
# define SWITCH_FALLTHROUGH  [[fallthrough]]
#else
# define SWITCH_FALLTHROUGH
#endif


// -- helpers -- ---------------------------------------------------------------

#define RESOLUTION_BUFFER_SIZE 14
#define RATE_BUFFER_SIZE 12

static char16_t* FormatInteger(uint32_t value, char16_t* buffer) {
  const size_t offset = (value >= 1000) ? ((value < 10000) ? 3 : 4)
                                        : ((value >= 100) ? 2 : 1);
  for (char16_t* it = buffer + (intptr_t)offset; it >= buffer; --it) {
    *it = u'0' + static_cast<char16_t>(value % 10u);
    value /= 10u;
  }
  return buffer + (intptr_t)(offset + 1u);
}

static void FormatResolution(uint32_t width, uint32_t height, char16_t outFormatted[RESOLUTION_BUFFER_SIZE]) {
  char16_t* it = FormatInteger(width, outFormatted);
  *it = u' ';
  *(++it) = u'x';
  *(++it) = u' ';
  it = FormatInteger(height, ++it);
  *it = u'\0';
}

static void FormatRate(uint32_t value, char16_t outFormatted[RATE_BUFFER_SIZE]) {
  switch (value) {
    // predefined values
    case 60000:  memcpy(outFormatted, u"60 Hz",  6*sizeof(char16_t)); break;
    case 59940:  memcpy(outFormatted, u"59.94 Hz", 9*sizeof(char16_t)); break;
    case 120000: memcpy(outFormatted, u"120 Hz", 7*sizeof(char16_t)); break;
    case 144000: memcpy(outFormatted, u"144 Hz", 7*sizeof(char16_t)); break;
    case 240000: memcpy(outFormatted, u"240 Hz", 7*sizeof(char16_t)); break;
    // custom values
    default: {
      auto* it = FormatInteger(value / 1000u, outFormatted); // mHz to Hz -- integer part
      value %= 1000u;
      if (value) { // decimals
        const auto* decimalIt = it;
        *it = u'.'; *(++it) = u'0'; *(++it) = u'0'; *(++it) = u'0';
        for (char16_t* revIt = it; revIt > decimalIt; --revIt) {
          *it = u'0' + static_cast<char16_t>(value % 10u);
          value /= 10u;
        }
        ++it;
      }
      *it = u' '; *(++it) = u'H'; *(++it) = u'z';*(++it) = (char16_t)0;
      break;
    }
  }
}

// ---

static constexpr inline uint64_t ToResolutionId(uint32_t width, uint32_t height) noexcept {
  return (((uint64_t)width << 32) | (uint64_t)height);
}

static void ListFullscreenModes(const DisplayMonitor& monitor, std::vector<ScreenResolution>& outFullscreenResolutions,
                                std::vector<std::vector<uint32_t> >& outRatesPerSize) {
  auto displayModes = monitor.listAvailableDisplayModes();
  outFullscreenResolutions.reserve(displayModes.size());
  outRatesPerSize.reserve(displayModes.size());

  std::unordered_map<uint64_t,size_t> currentResolutions;
  if (!monitor.attributes().id.empty()) {
    auto desktopMode = monitor.getDisplayMode();
    currentResolutions.emplace(ToResolutionId(desktopMode.width, desktopMode.height), (size_t)0);
    outFullscreenResolutions.emplace_back(desktopMode.width, desktopMode.height);
    outRatesPerSize.emplace_back(std::vector<uint32_t>{ desktopMode.refreshRate });
  }
  for (const auto& mode : displayModes) {
    if (mode.width >= 640u && mode.height >= 480u && mode.bitDepth >= 32u && mode.refreshRate >= 25u
    && (mode.width < 1100u || mode.width > 1200u)) { // remove nonsense resolutions to reduce list length
      uint64_t resolutionId = ToResolutionId(mode.width, mode.height);

      // resolution not yet defined -> new vector entries
      auto existing = currentResolutions.find(resolutionId);
      if (existing == currentResolutions.end()) {
        if (resolutionId == ToResolutionId(1920, 1080) && outFullscreenResolutions.size() > (size_t)1u) {
          // insert fullHD on top of the list (below desktop resolution)
          for (auto& it : currentResolutions) {
            if (it.second != 0)
              ++(it.second);
          }
          currentResolutions.emplace(resolutionId, (size_t)1);
          outFullscreenResolutions.insert(outFullscreenResolutions.begin() + 1, ScreenResolution(mode.width, mode.height));
          outRatesPerSize.insert(outRatesPerSize.begin() + 1, std::vector<uint32_t>{ mode.refreshRate });
        }
        else {
          currentResolutions.emplace(resolutionId, outFullscreenResolutions.size());
          outFullscreenResolutions.emplace_back(mode.width, mode.height);
          outRatesPerSize.emplace_back(std::vector<uint32_t>{ mode.refreshRate });
        }
      }
      else { // insert rate in order (existing resolution)
        auto& rates = outRatesPerSize[existing->second];
        auto it = rates.begin();
        while (it != rates.end() && *it < mode.refreshRate)
          ++it;
        if (it != rates.end()) { // insert (if not a duplicate)
          if (*it != mode.refreshRate)
            rates.insert(it, mode.refreshRate);
        }
        else rates.emplace_back(mode.refreshRate); // append
      }
    }
  }
}

// ---

static constexpr inline uint32_t GetWindowWidth(uint32_t windowHeight, bool isWidescreen) noexcept {
  return isWidescreen ? (((windowHeight*16u + 6u)/9u) & ~(uint32_t)0x1) : (windowHeight*4u/3u);
}

static uint32_t GetFullscreenResolutionValues(const std::vector<ScreenResolution>& fullscreenResolutions,
                                          std::vector<ComboBoxOption>& outResolutionOptions) {
  outResolutionOptions.reserve(fullscreenResolutions.size());

  uint32_t index = 0;
  char16_t formattedResolution[RESOLUTION_BUFFER_SIZE];
  for (const auto& resolution : fullscreenResolutions) {
    FormatResolution(resolution.width, resolution.height, formattedResolution);
    outResolutionOptions.emplace_back(formattedResolution, (ComboValue)index);
    ++index;
  }
  return 0;
}

static uint32_t GetFullscreenRateValues(std::vector<uint32_t>& fullscreenRates, uint32_t previousRate,
                                        std::vector<ComboBoxOption>& fullscreenRateOptions) {
  uint32_t selectedIndex = 0, currentIndex = 0;
  char16_t formattedRate[RATE_BUFFER_SIZE];
  for (auto rate : fullscreenRates) {
    if (rate == previousRate)
      selectedIndex = currentIndex;

    FormatRate(rate, formattedRate);
    fullscreenRateOptions.emplace_back(formattedRate, (ComboValue)rate);
    ++currentIndex;
  }
  return selectedIndex;
}


// -- page -- ------------------------------------------------------------------

#define DISPLAY_MODE_ID      1
#define FULLSCREEN_SIZE_ID   2
#define WINDOW_SIZE_ID       3
#define WIDESCREEN_HACK_ID   4
#define FRAMERATE_LIMIT_ID   5
#define FRAMERATE_FIXED_ID   6

GeneralSettings::GeneralSettings(std::shared_ptr<RendererContext> context_, std::shared_ptr<RendererStateBuffers> buffers_,
                                 const std::shared_ptr<ColorTheme>& theme_, const std::shared_ptr<MessageResources>& localizedText_,
                                 const pandora::hardware::DisplayMonitor& monitor, int32_t x, int32_t y,
                                 uint32_t width, uint32_t height, std::function<void()> onThemeChange_)
  : Page(std::move(context_), std::move(buffers_), *theme_, x, y, width, height, true),
    theme(theme_),
    localizedText(localizedText_),
    onThemeChange(std::move(onThemeChange_)) {
  ListFullscreenModes(monitor, fullscreenResolutions, fullscreenRatesPerSize);
  init(x, y, width);
}

void GeneralSettings::init(int32_t x, int32_t y, uint32_t width) {
  const uint32_t fieldsetPaddingX = Control::fieldsetMarginX(width);
  const int32_t controlX = x + (int32_t)fieldsetPaddingX + (int32_t)Control::fieldsetContentMarginX(width);
  uint32_t fieldsetWidth = width - (fieldsetPaddingX << 1);
  if (fieldsetWidth > Control::fieldsetMaxWidth())
    fieldsetWidth = Control::fieldsetMaxWidth();

  title = TextMesh(context->renderer(), context->getFont(FontType::titles), localizedText->getMessage(GeneralSettingsMessages::title),
                   context->pixelSizeX(), context->pixelSizeY(), x + (int32_t)fieldsetPaddingX, y + Control::titleMarginTop(), TextAlignment::left);

  std::vector<ControlRegistration> registry;
  registry.reserve(12);
  int32_t currentLineY = title.y() + (int32_t)title.height() + Control::pageLineHeight();
  auto changeHandler = std::bind(&GeneralSettings::onChange,this,std::placeholders::_1,std::placeholders::_2);

  // --- window group ---
  windowGroup = Fieldset(*context, localizedText->getMessage(GeneralSettingsMessages::windowGroup), theme->fieldsetStyle(),
                         theme->fieldsetControlColor(), x + (int32_t)fieldsetPaddingX, currentLineY, fieldsetWidth,
                         Control::fieldsetContentHeight(3));
  currentLineY += Control::pageLineHeight() + Control::fieldsetContentPaddingTop();

  // display mode
  {
    ComboBoxOption displayModeOptions[]{ ComboBoxOption(localizedText->getMessage(GeneralSettingsMessages::displayMode_fullscreen), 0/*TMP*/),
                                         ComboBoxOption(localizedText->getMessage(GeneralSettingsMessages::displayMode_borderless), 1/*TMP*/),
                                         ComboBoxOption(localizedText->getMessage(GeneralSettingsMessages::displayMode_window),     2/*TMP*/) };
    displayMode = Slider(*context, localizedText->getMessage(GeneralSettingsMessages::displayMode), controlX, currentLineY,
                         Control::pageLabelWidth(), Control::pageControlWidth(), theme->sliderArrowColor(), DISPLAY_MODE_ID,
                         changeHandler, displayModeOptions, sizeof(displayModeOptions)/sizeof(*displayModeOptions), 0);
    registry.emplace_back(displayMode, true, localizedText->getMessage(GeneralSettingsMessages::displayMode_tooltip));
    isFullscreenMode = true;
    isWindowMode = enableWidescreenMode = false;
    currentLineY += Control::pageLineHeight();
  }
  // fullscreen resolution/rate
  constexpr const uint32_t fullscreenSizeWidth = Control::pageControlWidth() - 108;
  constexpr const uint32_t fullscreenRateWidth = 105;
  constexpr const int32_t fullscreenRateMargin = 3;
  {
    std::vector<ComboBoxOption> fullscreenSizeOptions;
    uint32_t selectedResolutionIndex = GetFullscreenResolutionValues(fullscreenResolutions, fullscreenSizeOptions);

    fullscreenSize = ComboBox(*context, localizedText->getMessage(GeneralSettingsMessages::resolution), controlX, currentLineY,
                              Control::pageLabelWidth(), fullscreenSizeWidth, ComboBoxStyle::classic, theme->comboBoxColorParams(),
                              FULLSCREEN_SIZE_ID, changeHandler, fullscreenSizeOptions.data(), fullscreenSizeOptions.size(),
                              selectedResolutionIndex, &isFullscreenMode);
    registry.emplace_back(fullscreenSize, true, localizedText->getMessage(GeneralSettingsMessages::resolution_tooltip));

    std::vector<ComboBoxOption> fullscreenRateOptions;
    uint32_t selectedFullRate = GetFullscreenRateValues(fullscreenRatesPerSize[selectedResolutionIndex], 60000, fullscreenRateOptions);
    fullscreenRate = ComboBox(*context, nullptr, fullscreenSize.x() + (int32_t)fullscreenSize.width() + fullscreenRateMargin,
                              currentLineY, 0, fullscreenRateWidth, ComboBoxStyle::cutCorner, theme->comboBoxColorParams(), 0, nullptr,
                              fullscreenRateOptions.data(), fullscreenRateOptions.size(), selectedFullRate, &isFullscreenMode);
    registry.emplace_back(fullscreenRate, true, localizedText->getMessage(GeneralSettingsMessages::refreshRate_tooltip));
    currentLineY += Control::pageLineHeight();
  }
  // window mode size
  constexpr const uint32_t windowHeightValue = 720u;
  constexpr const uint32_t windowHeightBoxOffsetX = Control::pageLabelWidth() + 11 + ((fullscreenSizeWidth+1) >> 1);
  windowHeight = TextBox(*context, localizedText->getMessage(GeneralSettingsMessages::windowSize), nullptr, controlX,
                         currentLineY, windowHeightBoxOffsetX, (fullscreenSizeWidth >> 1) - 11, theme->textBoxControlColor(),
                         WINDOW_SIZE_ID, [this](uint32_t id){ this->onChange(id, this->windowHeight.valueInteger()); },
                         windowHeightValue, 4u, false, &isWindowMode);
  registry.emplace_back(windowHeight, true, localizedText->getMessage(GeneralSettingsMessages::windowSize_tooltip));

  onChange(WINDOW_SIZE_ID, windowHeightValue); // fill 'windowSize' text indicator
  currentLineY += Control::pageLineHeight() + Control::fieldsetContentMarginBottom();

  // --- compatibility group ---
  compatibilityGroup = Fieldset(*context, localizedText->getMessage(GeneralSettingsMessages::emulatorGroup),
                                theme->fieldsetStyle(), theme->fieldsetControlColor(),
                                x + (int32_t)fieldsetPaddingX, currentLineY, fieldsetWidth,
                                Control::fieldsetContentHeight(4));
  currentLineY += Control::pageLineHeight() + Control::fieldsetContentPaddingTop();

  // subprecision modes
  {
    ComboBoxOption subprecisionOptions[]{ ComboBoxOption(localizedText->getMessage(GeneralSettingsMessages::precision_original),     0/*TMP*/),
                                          ComboBoxOption(localizedText->getMessage(GeneralSettingsMessages::precision_subprecision), 1/*TMP*/) };
    subprecisionMode = Slider(*context, localizedText->getMessage(GeneralSettingsMessages::precision), controlX, currentLineY,
                              Control::pageLabelWidth(), Control::pageControlWidth(), theme->sliderArrowColor(), 0, nullptr,
                              subprecisionOptions, sizeof(subprecisionOptions)/sizeof(*subprecisionOptions), 0);
    registry.emplace_back(subprecisionMode, true, localizedText->getMessage(GeneralSettingsMessages::precision_tooltip));
    currentLineY += Control::pageLineHeight();
  }
  // widescreen hack
  widescreenMode = CheckBox(*context, localizedText->getMessage(GeneralSettingsMessages::widescreen), controlX,
                            currentLineY, Control::pageLabelWidth(), WIDESCREEN_HACK_ID, changeHandler, enableWidescreenMode);
  registry.emplace_back(widescreenMode, true, localizedText->getMessage(GeneralSettingsMessages::widescreen_tooltip));
  currentLineY += Control::pageLineHeight();

  isAutosaved = isAutoloaded = false;
  autosaveOnExit = CheckBox(*context, localizedText->getMessage(GeneralSettingsMessages::autosaveOnExit), controlX, currentLineY,
                            Control::pageLabelWidth(), 0, nullptr, isAutosaved);
  registry.emplace_back(autosaveOnExit, true, localizedText->getMessage(GeneralSettingsMessages::autosaveOnExit_tooltip));
  currentLineY += Control::pageLineHeight();

  autoloadOnStart = CheckBox(*context, localizedText->getMessage(GeneralSettingsMessages::autoloadOnStart), controlX, currentLineY,
                             Control::pageLabelWidth(), 0, nullptr, isAutoloaded, &isAutosaved);
  registry.emplace_back(autoloadOnStart, true, localizedText->getMessage(GeneralSettingsMessages::autoloadOnStart_tooltip));
  currentLineY += Control::pageLineHeight() + Control::fieldsetContentMarginBottom();

  // --- framerate group ---
  framerateGroup = Fieldset(*context, localizedText->getMessage(GeneralSettingsMessages::rateGroup), theme->fieldsetStyle(),
                            theme->fieldsetControlColor(), x + (int32_t)fieldsetPaddingX, currentLineY, fieldsetWidth,
                            Control::fieldsetContentHeight(3));
  currentLineY += Control::pageLineHeight() + Control::fieldsetContentPaddingTop();

  // frame rate limit mode
  {
    ComboBoxOption frameLimitOptions[]{ ComboBoxOption(localizedText->getMessage(CommonMessages::disabled),   0/*TMP*/),
                                        ComboBoxOption(localizedText->getMessage(GeneralSettingsMessages::rateLimit_autodetect), 1/*TMP*/),
                                        ComboBoxOption(localizedText->getMessage(GeneralSettingsMessages::rateLimit_custom),     2/*TMP*/) };
    framerateLimit = Slider(*context, localizedText->getMessage(GeneralSettingsMessages::rateLimit), controlX, currentLineY,
                            Control::pageLabelWidth(), Control::pageControlWidth(), theme->sliderArrowColor(),FRAMERATE_LIMIT_ID,
                            changeHandler, frameLimitOptions, sizeof(frameLimitOptions)/sizeof(*frameLimitOptions), 1);
    registry.emplace_back(framerateLimit, true, localizedText->getMessage(GeneralSettingsMessages::rateLimit_tooltip));
    isFramerateLimit = true;
    isFixedFramerate = isFrameSkipping = false;
    currentLineY += Control::pageLineHeight();
  }
  // custom frame rate
  {
    double fixedRateValue = 59.94;
    fixedFramerate = TextBox(*context, localizedText->getMessage(GeneralSettingsMessages::customRate),
                             localizedText->getMessage(CommonMessages::fps),
                             controlX, currentLineY, Control::pageLabelWidth(), 80u, theme->textBoxControlColor(),
                             FRAMERATE_FIXED_ID, [this](uint32_t id) { this->onChange(id, 0); },
                             fixedRateValue, 6u, &isFixedFramerate);
    registry.emplace_back(fixedFramerate, true, localizedText->getMessage(GeneralSettingsMessages::customRate_tooltip));
    currentLineY += Control::pageLineHeight();
  }
  // other frame rate settings
  frameSkipping = CheckBox(*context, localizedText->getMessage(GeneralSettingsMessages::frameSkip), controlX,
                           currentLineY, Control::pageLabelWidth(), 0, nullptr, isFrameSkipping, &isFramerateLimit);
  registry.emplace_back(frameSkipping, true, localizedText->getMessage(GeneralSettingsMessages::frameSkip_tooltip));
  currentLineY += Control::pageLineHeight() + Control::fieldsetContentMarginBottom();

  // --- user interface group ---
  userInterfaceGroup = Fieldset(*context, localizedText->getMessage(GeneralSettingsMessages::uiGroup), theme->fieldsetStyle(),
                                theme->fieldsetControlColor(), x + (int32_t)fieldsetPaddingX, currentLineY, fieldsetWidth,
                                Control::fieldsetContentHeight(2));
  currentLineY += Control::pageLineHeight() + Control::fieldsetContentPaddingTop();

  // interface color
  {
    ComboBoxOption colorOptions[]{ ComboBoxOption(localizedText->getMessage(GeneralSettingsMessages::theme_blue),
                                                  (ComboValue)ColorThemeType::blue),
                                   ComboBoxOption(localizedText->getMessage(GeneralSettingsMessages::theme_green),
                                                  (ComboValue)ColorThemeType::green),
                                   ComboBoxOption(localizedText->getMessage(GeneralSettingsMessages::theme_scifi),
                                                  (ComboValue)ColorThemeType::darkGreen),
                                   ComboBoxOption(localizedText->getMessage(GeneralSettingsMessages::theme_yellow),
                                                  (ComboValue)ColorThemeType::yellow) };
    interfaceColor = Slider(*context, localizedText->getMessage(GeneralSettingsMessages::theme),
                            controlX, currentLineY, Control::pageLabelWidth(),
                            Control::pageControlWidth(), theme->sliderArrowColor(), 0,
                            [this](uint32_t, uint32_t theme){
                              this->theme->updateTheme((ColorThemeType)theme);
                              this->buffers->updateColorBuffers(this->context->renderer(), *(this->theme));
                              Page::updateColors(*(this->theme));
                              init(this->x(), this->y(), this->width());
                            },
                            colorOptions, sizeof(colorOptions)/sizeof(*colorOptions), (uint32_t)theme->themeType());
    registry.emplace_back(interfaceColor, true, localizedText->getMessage(GeneralSettingsMessages::theme_tooltip));
    currentLineY += Control::pageLineHeight();
  }
  // interface language
  {
    ComboBoxOption languageOptions[(size_t)LocalizationType::COUNT];
    for (size_t i = 0; i < (size_t)LocalizationType::COUNT; ++i)
      languageOptions[i] = ComboBoxOption(LocalizationTypeHelper::toLanguageName((LocalizationType)i), (ComboValue)i);
    interfaceLanguage = ComboBox(*context, localizedText->getMessage(GeneralSettingsMessages::language),
                                 controlX, currentLineY, Control::pageLabelWidth(),
                                 105u, ComboBoxStyle::cutCorner, theme->comboBoxColorParams(), 0,
                                 [this](uint32_t, uint32_t language){
                                   this->localizedText->updateLocalization((LocalizationType)language);
                                   init(this->x(), this->y(), this->width());
                                 },
                                 languageOptions, sizeof(languageOptions)/sizeof(*languageOptions), (uint32_t)localizedText->language());
    registry.emplace_back(interfaceLanguage, true, localizedText->getMessage(GeneralSettingsMessages::language_tooltip));
  }
  currentLineY += Control::pageLineHeight();// + Control::fieldsetContentMarginBottom();

  // --- control registry ---
  if (currentLineY > y + (int32_t)contentHeight())
    Page::moveScrollbarThumb(currentLineY);
  registerControls(std::move(registry));
}

GeneralSettings::~GeneralSettings() noexcept {
  title.release();

  windowGroup.release();
  displayMode.release();
  fullscreenSize.release();
  fullscreenRate.release();
  windowHeight.release();
  windowSize.release();
  fullscreenRatesPerSize.clear();

  compatibilityGroup.release();
  subprecisionMode.release();
  widescreenMode.release();
  autosaveOnExit.release();
  autoloadOnStart.release();

  framerateGroup.release();
  framerateLimit.release();
  frameSkipping.release();
  fixedFramerate.release();

  userInterfaceGroup.release();
  interfaceColor.release();
  interfaceLanguage.release();
}


// -- window events -- ---------------------------------------------------------

void GeneralSettings::move(int32_t x, int32_t y, uint32_t width, uint32_t height) {
  Page::moveBase(x, y, width, height);
  const uint32_t fieldsetPaddingX = Control::fieldsetMarginX(width);
  const int32_t controlX = x + (int32_t)fieldsetPaddingX + (int32_t)Control::fieldsetContentMarginX(width);
  uint32_t fieldsetWidth = width - (fieldsetPaddingX << 1);
  if (fieldsetWidth > Control::fieldsetMaxWidth())
    fieldsetWidth = Control::fieldsetMaxWidth();

  title.move(context->renderer(), context->pixelSizeX(), context->pixelSizeY(), x + (int32_t)fieldsetPaddingX, y + Control::titleMarginTop());

  // display group
  int32_t currentLineY = title.y() + (int32_t)title.height() + Control::pageLineHeight();
  windowGroup.move(*context, x + (int32_t)fieldsetPaddingX, currentLineY, fieldsetWidth, Control::fieldsetContentHeight(3));
  currentLineY += Control::pageLineHeight() + Control::fieldsetContentPaddingTop();

  displayMode.move(*context, controlX, currentLineY);
  currentLineY += Control::pageLineHeight();
  fullscreenSize.move(*context, controlX, currentLineY);
  fullscreenRate.move(*context, fullscreenSize.x() + (int32_t)fullscreenSize.width() + 3, currentLineY);
  currentLineY += Control::pageLineHeight();
  const int32_t windowSizeOffsetX = windowSize.x() - windowHeight.controlX();
  const int32_t windowSizeOffsetY = windowSize.y() - (windowHeight.y() + Control::textBoxPaddingY());
  windowHeight.move(*context, controlX, currentLineY);
  windowSize.move(context->renderer(), context->pixelSizeX(), context->pixelSizeY(),
                  windowHeight.controlX() + windowSizeOffsetX, currentLineY + windowSizeOffsetY);
  currentLineY += Control::pageLineHeight() + Control::fieldsetContentMarginBottom();

  // compatibility group
  compatibilityGroup.move(*context, x + (int32_t)fieldsetPaddingX, currentLineY, fieldsetWidth, Control::fieldsetContentHeight(4));
  currentLineY += Control::pageLineHeight() + Control::fieldsetContentPaddingTop();

  subprecisionMode.move(*context, controlX, currentLineY);
  currentLineY += Control::pageLineHeight();
  widescreenMode.move(*context, controlX, currentLineY);
  currentLineY += Control::pageLineHeight();
  autosaveOnExit.move(*context, controlX, currentLineY);
  currentLineY += Control::pageLineHeight();
  autoloadOnStart.move(*context, controlX, currentLineY);
  currentLineY += Control::pageLineHeight() + Control::fieldsetContentMarginBottom();

  // framerate group
  framerateGroup.move(*context, x + (int32_t)fieldsetPaddingX, currentLineY, fieldsetWidth, Control::fieldsetContentHeight(3));
  currentLineY += Control::pageLineHeight() + Control::fieldsetContentPaddingTop();

  framerateLimit.move(*context, controlX, currentLineY);
  currentLineY += Control::pageLineHeight();
  fixedFramerate.move(*context, controlX, currentLineY);
  currentLineY += Control::pageLineHeight();
  frameSkipping.move(*context, controlX, currentLineY);
  currentLineY += Control::pageLineHeight() + Control::fieldsetContentMarginBottom();

  // user interface group
  userInterfaceGroup.move(*context, x + (int32_t)fieldsetPaddingX, currentLineY, fieldsetWidth, Control::fieldsetContentHeight(2));
  currentLineY += Control::pageLineHeight() + Control::fieldsetContentPaddingTop();

  interfaceColor.move(*context, controlX, currentLineY);
  currentLineY += Control::pageLineHeight();
  interfaceLanguage.move(*context, controlX, currentLineY);
  currentLineY += Control::pageLineHeight();// + Control::fieldsetContentMarginBottom();

  Page::moveScrollbarThumb(currentLineY); // required after a move
}

void GeneralSettings::onChange(uint32_t id, uint32_t value) {
  switch (id) {
    case DISPLAY_MODE_ID: {
      isFullscreenMode = (value == 0/*TMP*/);
      isWindowMode = (value == 2/*TMP*/);
      break;
    }
    case FULLSCREEN_SIZE_ID: {
      std::vector<ComboBoxOption> fullscreenRateOptions;
      const auto* currentRate = fullscreenRate.getSelectedValue();
      uint32_t selectedFullRate = GetFullscreenRateValues(fullscreenRatesPerSize[value],
                                                          currentRate ? *currentRate : 60000, fullscreenRateOptions);
      fullscreenRate.replaceValues(*context, fullscreenRateOptions.data(), fullscreenRateOptions.size(), selectedFullRate);
      break;
    }
    case WIDESCREEN_HACK_ID:
      value = windowHeight.valueInteger();
      SWITCH_FALLTHROUGH;
    case WINDOW_SIZE_ID: {
      if (value < 480) {
        value = value ? 480 : 720;
        windowHeight.replaceValueInteger(*context, (uint32_t)value);
      }
      char16_t windowWidthBuffer[14];
      memcpy(windowWidthBuffer, u"           x", 13*sizeof(char16_t));
      FormatInteger(GetWindowWidth(value, enableWidescreenMode), windowWidthBuffer);

      auto& inputFont = context->getFont(FontType::inputText);
      int32_t windowSizeY = windowSize.y();
      if (windowSizeY == 0) {
        auto& labelFont = context->getFont(FontType::labels);
        windowSizeY = windowHeight.labelY() + (int32_t)(labelFont.XHeight() - inputFont.XHeight() + 1)/2;
      }
      windowSize = TextMesh(context->renderer(), inputFont, windowWidthBuffer, context->pixelSizeX(), context->pixelSizeY(),
                            windowHeight.controlX() - 8, windowSizeY, TextAlignment::right);
      break;
    }
    case FRAMERATE_LIMIT_ID: {
      isFramerateLimit = (value != 0/*TMP*/);
      isFixedFramerate = (value == 2/*TMP*/);
      break;
    }
    case FRAMERATE_FIXED_ID: {
      if (fixedFramerate.valueNumber() == 0.0)
        fixedFramerate.replaceValueNumber(*context, 59.94);
      break;
    }
    default: assert(false); break;
  }
}


// -- rendering -- -------------------------------------------------------------

void GeneralSettings::drawIcons() {
  // scrollable geometry
  buffers->bindScrollLocationBuffer(context->renderer(), ScissorRectangle(x(), y(), width(), contentHeight()));

  auto* hoverControl = getActiveControl();
  widescreenMode.drawIcon(*context, *buffers, (hoverControl == &widescreenMode));
  autosaveOnExit.drawIcon(*context, *buffers, (hoverControl == &autosaveOnExit));
  autoloadOnStart.drawIcon(*context, *buffers, (hoverControl == &autoloadOnStart));
  frameSkipping.drawIcon(*context, *buffers, (hoverControl == &frameSkipping));
}

bool GeneralSettings::drawPageBackgrounds(int32_t mouseX, int32_t mouseY) {
  // scrollable geometry
  if (buffers->isFixedLocationBuffer())
    buffers->bindScrollLocationBuffer(context->renderer(), ScissorRectangle(x(), y(), width(), contentHeight()));

  windowGroup.drawBackground(*context, *buffers);
  compatibilityGroup.drawBackground(*context, *buffers);
  framerateGroup.drawBackground(*context, *buffers);
  userInterfaceGroup.drawBackground(*context, *buffers);

  auto* hoverControl = getActiveControl();
  displayMode.drawBackground(*context, mouseX, *buffers, (hoverControl == &displayMode));
  subprecisionMode.drawBackground(*context, mouseX, *buffers, (hoverControl == &subprecisionMode));
  framerateLimit.drawBackground(*context, mouseX, *buffers, (hoverControl == &framerateLimit));
  interfaceColor.drawBackground(*context, mouseX, *buffers, (hoverControl == &interfaceColor));

  fullscreenSize.drawBackground(*context, *buffers, (hoverControl == &fullscreenSize));
  fullscreenRate.drawBackground(*context, *buffers, (hoverControl == &fullscreenRate));
  interfaceLanguage.drawBackground(*context, *buffers, (hoverControl == &interfaceLanguage));
  windowHeight.drawBackground(*context, mouseX, mouseY, *buffers, (hoverControl == &windowHeight));
  fixedFramerate.drawBackground(*context, mouseX, mouseY, *buffers, (hoverControl == &fixedFramerate));
  return false;
}

void GeneralSettings::drawPageLabels() {
  // scrollable geometry
  auto& renderer = context->renderer();
  if (buffers->isFixedLocationBuffer())
    buffers->bindScrollLocationBuffer(renderer, ScissorRectangle(x(), y(), width(), contentHeight()));

  buffers->bindLabelBuffer(renderer, LabelBufferType::title);
  title.draw(renderer);

  windowGroup.drawLabel(*context, *buffers);
  compatibilityGroup.drawLabel(*context, *buffers);
  framerateGroup.drawLabel(*context, *buffers);
  userInterfaceGroup.drawLabel(*context, *buffers);

  auto* hoverControl = getActiveControl();
  displayMode.drawLabels(*context, *buffers, (hoverControl == &displayMode));
  subprecisionMode.drawLabels(*context, *buffers, (hoverControl == &subprecisionMode));
  framerateLimit.drawLabels(*context, *buffers, (hoverControl == &framerateLimit));
  interfaceColor.drawLabels(*context, *buffers, (hoverControl == &interfaceColor));

  fullscreenSize.drawLabels(*context, *buffers, (hoverControl == &fullscreenSize));
  fullscreenRate.drawLabels(*context, *buffers, (hoverControl == &fullscreenRate));
  interfaceLanguage.drawLabels(*context, *buffers, (hoverControl == &interfaceLanguage));
  windowHeight.drawLabels(*context, *buffers, (hoverControl == &windowHeight));
  fixedFramerate.drawLabels(*context, *buffers, (hoverControl == &fixedFramerate));
  buffers->bindLabelBuffer(renderer, windowHeight.isEnabled() ? LabelBufferType::regular : LabelBufferType::disabled);
  windowSize.draw(renderer);

  widescreenMode.drawLabel(*context, *buffers, (hoverControl == &widescreenMode));
  autosaveOnExit.drawLabel(*context, *buffers, (hoverControl == &autosaveOnExit));
  autoloadOnStart.drawLabel(*context, *buffers, (hoverControl == &autoloadOnStart));
  frameSkipping.drawLabel(*context, *buffers, (hoverControl == &frameSkipping));
}

void GeneralSettings::drawForegrounds() {
  auto& renderer = context->renderer();

  ScissorRectangle fullWindowArea(0, 0, context->clientWidth(), context->clientHeight());
  buffers->bindScrollLocationBuffer(renderer, fullWindowArea); // visible outside of scroll area -> full window

  fullscreenSize.drawDropdown(*context, *buffers);
  fullscreenRate.drawDropdown(*context, *buffers);
  interfaceLanguage.drawDropdown(*context, *buffers);
}

void GeneralSettings::drawForegroundLabels() {
  ScissorRectangle fullWindowArea(0, 0, context->clientWidth(), context->clientHeight());
  buffers->bindScrollLocationBuffer(context->renderer(), fullWindowArea); // visible outside of scroll area -> full window

  fullscreenSize.drawOptions(*context, *buffers);
  fullscreenRate.drawOptions(*context, *buffers);
  interfaceLanguage.drawOptions(*context, *buffers);
}
