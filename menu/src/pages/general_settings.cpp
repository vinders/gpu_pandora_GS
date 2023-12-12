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
#include "menu/pages/page_content_builder.h"
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

static char16_t* formatInteger(uint32_t value, char16_t* buffer) {
  const size_t offset = (value >= 1000) ? ((value < 10000) ? 3 : 4)
                                        : ((value >= 100) ? 2 : 1);
  for (char16_t* it = buffer + (intptr_t)offset; it >= buffer; --it) {
    *it = u'0' + static_cast<char16_t>(value % 10u);
    value /= 10u;
  }
  return buffer + (intptr_t)(offset + 1u);
}

static void formatResolution(uint32_t width, uint32_t height, char16_t outFormatted[RESOLUTION_BUFFER_SIZE]) {
  char16_t* it = formatInteger(width, outFormatted);
  *it = u' ';
  *(++it) = u'x';
  *(++it) = u' ';
  it = formatInteger(height, ++it);
  *it = u'\0';
}

static void formatRate(uint32_t value, char16_t outFormatted[RATE_BUFFER_SIZE]) {
  switch (value) {
    // predefined values
    case 60000:  memcpy(outFormatted, u"60 Hz",  6*sizeof(char16_t)); break;
    case 59940:  memcpy(outFormatted, u"59.94 Hz", 9*sizeof(char16_t)); break;
    case 120000: memcpy(outFormatted, u"120 Hz", 7*sizeof(char16_t)); break;
    case 144000: memcpy(outFormatted, u"144 Hz", 7*sizeof(char16_t)); break;
    case 240000: memcpy(outFormatted, u"240 Hz", 7*sizeof(char16_t)); break;
    // custom values
    default: {
      auto* it = formatInteger(value / 1000u, outFormatted); // mHz to Hz -- integer part
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

static constexpr inline uint64_t toResolutionId(uint32_t width, uint32_t height) noexcept {
  return (((uint64_t)width << 32) | (uint64_t)height);
}

static void listFullscreenModes(const DisplayMonitor& monitor, std::vector<ScreenResolution>& outFullscreenResolutions,
                                std::vector<std::vector<uint32_t> >& outRatesPerSize) {
  auto displayModes = monitor.listAvailableDisplayModes();
  outFullscreenResolutions.reserve(displayModes.size());
  outRatesPerSize.reserve(displayModes.size());

  std::unordered_map<uint64_t,size_t> currentResolutions;
  if (!monitor.attributes().id.empty()) {
    auto desktopMode = monitor.getDisplayMode();
    currentResolutions.emplace(toResolutionId(desktopMode.width, desktopMode.height), (size_t)0);
    outFullscreenResolutions.emplace_back(desktopMode.width, desktopMode.height);
    outRatesPerSize.emplace_back(std::vector<uint32_t>{ desktopMode.refreshRate });
  }
  for (const auto& mode : displayModes) {
    if (mode.width >= 640u && mode.height >= 480u && mode.bitDepth >= 32u && mode.refreshRate >= 25u
    && (mode.width < 1100u || mode.width > 1200u)) { // remove nonsense resolutions to reduce list length
      uint64_t resolutionId = toResolutionId(mode.width, mode.height);

      // resolution not yet defined -> new vector entries
      auto existing = currentResolutions.find(resolutionId);
      if (existing == currentResolutions.end()) {
        if (resolutionId == toResolutionId(1920, 1080) && outFullscreenResolutions.size() > (size_t)1u) {
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
                                              ScreenResolution previousSize, std::vector<ComboBoxOption>& outResolutionOptions) {
  outResolutionOptions.reserve(fullscreenResolutions.size());

  uint32_t selectedIndex = 0, currentIndex = 0;
  char16_t formattedResolution[RESOLUTION_BUFFER_SIZE];
  for (const auto& resolution : fullscreenResolutions) {
    if (resolution == previousSize)
      selectedIndex = currentIndex;

    formatResolution(resolution.width, resolution.height, formattedResolution);
    outResolutionOptions.emplace_back(formattedResolution, (ComboValue)currentIndex);
    ++currentIndex;
  }
  return selectedIndex;
}

static uint32_t GetFullscreenRateValues(std::vector<uint32_t>& fullscreenRates, uint32_t previousRate,
                                        std::vector<ComboBoxOption>& fullscreenRateOptions) {
  uint32_t selectedIndex = 0, currentIndex = 0;
  char16_t formattedRate[RATE_BUFFER_SIZE];
  for (auto rate : fullscreenRates) {
    if (rate == previousRate)
      selectedIndex = currentIndex;

    formatRate(rate, formattedRate);
    fullscreenRateOptions.emplace_back(formattedRate, (ComboValue)rate);
    ++currentIndex;
  }
  return selectedIndex;
}


// -- page -- ------------------------------------------------------------------

#define DISPLAY_MODE_ID       1
#define SCREEN_SELECT_ID      2
#define FULLSCREEN_SIZE_ID    3
#define WINDOW_HEIGHT_ID      4
#define WIDESCREEN_MODE_ID    5
#define FRAMERATE_LIMIT_ID    6
#define FIXED_FRAMERATE_ID    7
#define INTERFACE_COLOR_ID    8
#define INTERFACE_LANGUAGE_ID 9

#define SECONDARY_COMBOBOX_WIDTH 105
#define DEFAULT_DISPLAY_RATE 60000

GeneralSettings::GeneralSettings(std::shared_ptr<RendererContext> context_, std::shared_ptr<RendererStateBuffers> buffers_,
                                 const std::shared_ptr<ColorTheme>& theme_, const std::shared_ptr<MessageResources>& localizedText_,
                                 DisplayMonitor::Handle requestedMonitor, int32_t x, int32_t y,
                                 uint32_t width, uint32_t height, std::function<void()> onThemeChange_)
  : Page(std::move(context_), std::move(buffers_), *theme_, x, y, width, height, true, true),
    availableMonitors(DisplayMonitor::listAvailableMonitors()),
    theme(theme_),
    localizedText(localizedText_),
    onThemeChange(std::move(onThemeChange_)) {
  DisplayMonitor monitor(requestedMonitor, true);
  targetMonitor = monitor.handle(); // requested -or- primary (if not found)
  listFullscreenModes(monitor, fullscreenResolutions, fullscreenRatesPerSize);
  if (availableMonitors.empty())
    availableMonitors.emplace_back(std::move(monitor));

  init(x, y, width);
}

void GeneralSettings::init(int32_t x, int32_t y, uint32_t width) {
  PageContentBuilder builder(*context, *theme, x, y, width, 12,
                             std::bind(&GeneralSettings::onChange,this,std::placeholders::_1),
                             std::bind(&GeneralSettings::onValueChange,this,std::placeholders::_1,std::placeholders::_2));

  builder.addTitle(localizedText->getMessage(GeneralSettingsMessages::title), title);

  // window group
  builder.addFieldset(localizedText->getMessage(GeneralSettingsMessages::windowGroup), (availableMonitors.size() > (size_t)1u) ? 4 : 3, 0, windowGroup);
  isFullscreenMode = true;
  isWindowMode = enableWidescreenMode = false;

  ComboBoxOption displayModeOptions[]{ ComboBoxOption(localizedText->getMessage(GeneralSettingsMessages::displayMode_fullscreen), 0/*TMP*/),
                                       ComboBoxOption(localizedText->getMessage(GeneralSettingsMessages::displayMode_borderless), 1/*TMP*/),
                                       ComboBoxOption(localizedText->getMessage(GeneralSettingsMessages::displayMode_window),     2/*TMP*/) };
  builder.addSlider(DISPLAY_MODE_ID, localizedText->getMessage(GeneralSettingsMessages::displayMode),
                    localizedText->getMessage(GeneralSettingsMessages::displayMode_tooltip), Control::pageControlWidth(),
                    displayModeOptions, sizeof(displayModeOptions)/sizeof(*displayModeOptions), 0, displayMode);

  if (availableMonitors.size() > (size_t)1u) {
    uint32_t screenSelectIndex = 0, currentScreenIndex = 0;
    std::vector<ComboBoxOption> screenSelectOptions;
    screenSelectOptions.reserve(availableMonitors.size());
    for (auto& monitor : availableMonitors) {
      if (monitor.handle() == targetMonitor)
        screenSelectIndex = currentScreenIndex;

      const wchar_t* screenName = monitor.attributes().id.data();
      if (screenName) { // skip system/path prefix
        while (*screenName && (*screenName == L'\\' || *screenName == L'/' || *screenName == L'.' || *screenName == L':'))
          ++screenName;
      }
      screenSelectOptions.emplace_back(screenName, (ComboValue)currentScreenIndex);
      ++currentScreenIndex;
    }

    builder.addComboBox(SCREEN_SELECT_ID, localizedText->getMessage(GeneralSettingsMessages::screenSelection),
                        localizedText->getMessage(GeneralSettingsMessages::screenSelection_tooltip), Control::pageControlWidth(),
                        screenSelectOptions.data(), screenSelectOptions.size(), screenSelectIndex, screenSelection);
  }
  
  std::vector<ComboBoxOption> fullscreenSizeOptions, fullscreenRateOptions;
  uint32_t selectedResolutionIndex = GetFullscreenResolutionValues(fullscreenResolutions, ScreenResolution{}, fullscreenSizeOptions);
  uint32_t selectedRateIndex = GetFullscreenRateValues(fullscreenRatesPerSize[selectedResolutionIndex], DEFAULT_DISPLAY_RATE, fullscreenRateOptions);
  constexpr const uint32_t windowHeightValue = 720u;

  builder.setEnabler(isFullscreenMode);
  builder.addDoubleComboBox(FULLSCREEN_SIZE_ID, localizedText->getMessage(GeneralSettingsMessages::resolution),
                            localizedText->getMessage(GeneralSettingsMessages::resolution_tooltip),
                            fullscreenSizeOptions.data(), fullscreenSizeOptions.size(), selectedResolutionIndex, fullscreenSize,
                            0, SECONDARY_COMBOBOX_WIDTH, fullscreenRateOptions.data(), fullscreenRateOptions.size(), selectedRateIndex, fullscreenRate);
  builder.setEnabler(isWindowMode);
  builder.addResolutionTextBox(WINDOW_HEIGHT_ID, localizedText->getMessage(GeneralSettingsMessages::windowSize),
                               localizedText->getMessage(GeneralSettingsMessages::windowSize_tooltip),
                               (fullscreenSize.controlWidth() >> 1) - 11u, ((fullscreenSize.controlWidth()+1u) >> 1) + 11u,
                               windowHeightValue, windowHeight);
  onValueChange(WINDOW_HEIGHT_ID, windowHeightValue); // fill 'windowSizeInfo' text indicator
  builder.resetEnabler();

  // compatibility group
  builder.addFieldset(localizedText->getMessage(GeneralSettingsMessages::emulatorGroup), 4, 0, compatibilityGroup);
  isAutosaved = isAutoloaded = false;

  ComboBoxOption subprecisionOptions[]{ ComboBoxOption(localizedText->getMessage(GeneralSettingsMessages::precision_original),     0/*TMP*/),
                                        ComboBoxOption(localizedText->getMessage(GeneralSettingsMessages::precision_subprecision), 1/*TMP*/) };
  builder.addSlider(0, localizedText->getMessage(GeneralSettingsMessages::precision),
                    localizedText->getMessage(GeneralSettingsMessages::precision_tooltip), Control::pageControlWidth(),
                    subprecisionOptions, sizeof(subprecisionOptions)/sizeof(*subprecisionOptions), 0, subprecisionMode);
  builder.addCheckBox(WIDESCREEN_MODE_ID, localizedText->getMessage(GeneralSettingsMessages::widescreen),
                      localizedText->getMessage(GeneralSettingsMessages::widescreen_tooltip), enableWidescreenMode, widescreenMode);

  builder.addCheckBox(0, localizedText->getMessage(GeneralSettingsMessages::autosaveOnExit),
                      localizedText->getMessage(GeneralSettingsMessages::autosaveOnExit_tooltip), isAutosaved, autosaveOnExit);
  builder.setEnabler(isAutosaved);
  builder.addCheckBox(0, localizedText->getMessage(GeneralSettingsMessages::autoloadOnStart),
                      localizedText->getMessage(GeneralSettingsMessages::autoloadOnStart_tooltip), isAutoloaded, autoloadOnStart);
  builder.resetEnabler();

  // framerate group
  builder.addFieldset(localizedText->getMessage(GeneralSettingsMessages::rateGroup), 3, 0, framerateGroup);
  isFramerateLimit = true;
  isFixedFramerate = isFrameSkipping = false;
  double fixedRateValue = 59.94;

  ComboBoxOption frameLimitOptions[]{ ComboBoxOption(localizedText->getMessage(CommonMessages::disabled),   0/*TMP*/),
                                      ComboBoxOption(localizedText->getMessage(GeneralSettingsMessages::rateLimit_autodetect), 1/*TMP*/),
                                      ComboBoxOption(localizedText->getMessage(GeneralSettingsMessages::rateLimit_custom),     2/*TMP*/) };
  builder.addSlider(FRAMERATE_LIMIT_ID, localizedText->getMessage(GeneralSettingsMessages::rateLimit),
                    localizedText->getMessage(GeneralSettingsMessages::rateLimit_tooltip), Control::pageControlWidth(),
                    frameLimitOptions, sizeof(frameLimitOptions)/sizeof(*frameLimitOptions), 1, framerateLimit);

  builder.setEnabler(isFixedFramerate);
  builder.addNumberTextBox(FIXED_FRAMERATE_ID, localizedText->getMessage(GeneralSettingsMessages::customRate),
                           localizedText->getMessage(CommonMessages::fps), localizedText->getMessage(GeneralSettingsMessages::customRate_tooltip),
                           80u, fixedRateValue, fixedFramerate);
  
  builder.setEnabler(isFramerateLimit);
  builder.addCheckBox(0, localizedText->getMessage(GeneralSettingsMessages::frameSkip),
                      localizedText->getMessage(GeneralSettingsMessages::frameSkip_tooltip), isFrameSkipping, frameSkipping);
  builder.resetEnabler();

  // user interface group
  builder.addFieldset(localizedText->getMessage(GeneralSettingsMessages::uiGroup), 2, 0, userInterfaceGroup);

  ComboBoxOption colorOptions[(size_t)ColorThemeType::COUNT];
  for (uint32_t i = 0; i < (uint32_t)ColorThemeType::COUNT; ++i) {
    const auto messageId = static_cast<GeneralSettingsMessages>((uint32_t)GeneralSettingsMessages::theme + 1u + i);
    colorOptions[i] = ComboBoxOption(localizedText->getMessage(messageId), (ComboValue)i);
  }
  builder.addSlider(INTERFACE_COLOR_ID, localizedText->getMessage(GeneralSettingsMessages::theme),
                    localizedText->getMessage(GeneralSettingsMessages::theme_tooltip), Control::pageControlWidth(),
                    colorOptions, sizeof(colorOptions)/sizeof(*colorOptions), (uint32_t)theme->themeType(), interfaceColor);

  ComboBoxOption languageOptions[(size_t)LocalizationType::COUNT];
  for (size_t i = 0; i < (size_t)LocalizationType::COUNT; ++i) {
    languageOptions[i] = ComboBoxOption(LocalizationTypeHelper::toLanguageName((LocalizationType)i), (ComboValue)i);
  }
  builder.addComboBox(INTERFACE_LANGUAGE_ID, localizedText->getMessage(GeneralSettingsMessages::language),
                      localizedText->getMessage(GeneralSettingsMessages::language_tooltip), SECONDARY_COMBOBOX_WIDTH,
                      languageOptions, sizeof(languageOptions)/sizeof(*languageOptions), (uint32_t)localizedText->language(), interfaceLanguage);

  // control registration
  Page::moveScrollbarThumb(builder.linePositionY());
  registerControls(std::move(builder.controlRegistry()));
}

GeneralSettings::~GeneralSettings() noexcept {
  title.release();

  windowGroup.release();
  displayMode.release();
  screenSelection.release();
  fullscreenSize.release();
  fullscreenRate.release();
  windowHeight.release();
  windowSizeInfo.release();
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
  PageContentMover mover(*context, x, y, width);
  const int32_t offsetX = mover.linePositionX() - displayMode.x();

  mover.moveTitle(title);

  // display group
  mover.moveFieldset(windowGroup);

  mover.moveSlider(displayMode);
  if (screenSelection.width())
    mover.moveComboBox(screenSelection);
  mover.moveDoubleComboBox(fullscreenSize, fullscreenRate);

  const int32_t windowSizeOffsetX = windowSizeInfo.x() - windowHeight.controlX();
  const int32_t windowSizeY = mover.linePositionY() + windowSizeInfo.y() - (windowHeight.y() + (int32_t)Control::textBoxPaddingY());
  mover.moveTextBox(windowHeight);
  windowSizeInfo.move(context->renderer(), context->pixelSizeX(), context->pixelSizeY(),
                      windowHeight.controlX() + windowSizeOffsetX, windowSizeY);

  // compatibility group
  mover.moveFieldset(compatibilityGroup);

  mover.moveSlider(subprecisionMode);
  mover.moveCheckBox(widescreenMode);
  mover.moveCheckBox(autosaveOnExit);
  mover.moveCheckBox(autoloadOnStart);

  // framerate group
  mover.moveFieldset(framerateGroup);

  mover.moveSlider(framerateLimit);
  mover.moveTextBox(fixedFramerate);
  mover.moveCheckBox(frameSkipping);

  // user interface group
  mover.moveFieldset(userInterfaceGroup);

  mover.moveSlider(interfaceColor);
  mover.moveComboBox(interfaceLanguage);

  Page::moveScrollbarThumb(mover.linePositionY()); // required after a move
  Page::moveRegisteredControls(offsetX);
}

void GeneralSettings::onChange(uint32_t id) {
  if (id == WINDOW_HEIGHT_ID)
    onValueChange(id, windowHeight.valueInteger());
  else if (id == FIXED_FRAMERATE_ID)
    onValueChange(id, 0);
}

void GeneralSettings::onValueChange(uint32_t id, uint32_t value) {
  switch (id) {
    case DISPLAY_MODE_ID: {
      isFullscreenMode = (value == 0/*TMP*/);
      isWindowMode = (value == 2/*TMP*/);
      break;
    }
    case SCREEN_SELECT_ID: {
      const auto* previousSizeIndex = fullscreenSize.getSelectedValue(); // copy old size before clearing current list
      ScreenResolution previousSize = previousSizeIndex ? fullscreenResolutions[*previousSizeIndex] : ScreenResolution{};

      fullscreenResolutions.clear();
      fullscreenRatesPerSize.clear();
      listFullscreenModes(availableMonitors[value], fullscreenResolutions, fullscreenRatesPerSize);
      targetMonitor = availableMonitors[value].handle();

      std::vector<ComboBoxOption> fullscreenSizeOptions, fullscreenRateOptions;
      uint32_t selectedResolutionIndex = GetFullscreenResolutionValues(fullscreenResolutions, previousSize, fullscreenSizeOptions);
      fullscreenSize.replaceValues(*context, fullscreenSizeOptions.data(), fullscreenSizeOptions.size(), selectedResolutionIndex);

      const auto* previousRate = fullscreenRate.getSelectedValue();
      uint32_t selectedRateIndex = GetFullscreenRateValues(fullscreenRatesPerSize[selectedResolutionIndex],
                                                           previousRate ? *previousRate : DEFAULT_DISPLAY_RATE, fullscreenRateOptions);
      fullscreenRate.replaceValues(*context, fullscreenRateOptions.data(), fullscreenRateOptions.size(), selectedRateIndex);
      break;
    }
    case FULLSCREEN_SIZE_ID: {
      std::vector<ComboBoxOption> fullscreenRateOptions;
      const auto* previousRate = fullscreenRate.getSelectedValue();
      uint32_t selectedFullRate = GetFullscreenRateValues(fullscreenRatesPerSize[value],
                                                          previousRate ? *previousRate : DEFAULT_DISPLAY_RATE, fullscreenRateOptions);
      fullscreenRate.replaceValues(*context, fullscreenRateOptions.data(), fullscreenRateOptions.size(), selectedFullRate);
      break;
    }
    case WIDESCREEN_MODE_ID:
      value = windowHeight.valueInteger();
      SWITCH_FALLTHROUGH;
    case WINDOW_HEIGHT_ID: {
      if (value < 480) {
        value = value ? 480 : 720;
        windowHeight.replaceValueInteger(*context, (uint32_t)value);
      }
      char16_t windowWidthBuffer[14];
      memcpy(windowWidthBuffer, u"           x", 13*sizeof(char16_t));
      formatInteger(GetWindowWidth(value, enableWidescreenMode), windowWidthBuffer);

      auto& inputFont = context->getFont(FontType::inputText);
      int32_t windowSizeY = windowSizeInfo.y();
      if (windowSizeY == 0) {
        auto& labelFont = context->getFont(FontType::labels);
        windowSizeY = windowHeight.labelY() + (int32_t)(labelFont.XHeight() - inputFont.XHeight() + 1)/2;
      }
      windowSizeInfo = TextMesh(context->renderer(), inputFont, windowWidthBuffer, context->pixelSizeX(), context->pixelSizeY(),
                                windowHeight.controlX() - 8, windowSizeY, TextAlignment::right);
      break;
    }
    case FRAMERATE_LIMIT_ID: {
      isFramerateLimit = (value != 0/*TMP*/);
      isFixedFramerate = (value == 2/*TMP*/);
      break;
    }
    case FIXED_FRAMERATE_ID: {
      if (fixedFramerate.valueNumber() == 0.0)
        fixedFramerate.replaceValueNumber(*context, 59.94);
      break;
    }
    case INTERFACE_COLOR_ID: {
      theme->updateTheme((ColorThemeType)value);
      buffers->updateColorBuffers(context->renderer(), *theme);
      Page::updateColors(*theme);
      init(x(), y(), width());
      selectControlIndex(static_cast<uint32_t>(countRegistryControls() - 2u));
      if (onThemeChange)
        onThemeChange();
      break;
    }
    case INTERFACE_LANGUAGE_ID: {
      localizedText->updateLocalization((LocalizationType)value);
      init(x(), y(), width());
      selectControlIndex(static_cast<uint32_t>(countRegistryControls() - 1u));
      if (onThemeChange)
        onThemeChange();
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

void GeneralSettings::drawPageBackgrounds(int32_t mouseX, int32_t mouseY) {
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

  if (screenSelection.width())
    screenSelection.drawBackground(*context, *buffers, (hoverControl == &screenSelection));
  fullscreenSize.drawBackground(*context, *buffers, (hoverControl == &fullscreenSize));
  fullscreenRate.drawBackground(*context, *buffers, (hoverControl == &fullscreenRate));
  interfaceLanguage.drawBackground(*context, *buffers, (hoverControl == &interfaceLanguage));
  windowHeight.drawBackground(*context, mouseX, mouseY, *buffers, (hoverControl == &windowHeight));
  fixedFramerate.drawBackground(*context, mouseX, mouseY, *buffers, (hoverControl == &fixedFramerate));
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

  if (screenSelection.width())
    screenSelection.drawLabels(*context, *buffers, (hoverControl == &screenSelection));
  fullscreenSize.drawLabels(*context, *buffers, (hoverControl == &fullscreenSize));
  fullscreenRate.drawLabels(*context, *buffers, (hoverControl == &fullscreenRate));
  interfaceLanguage.drawLabels(*context, *buffers, (hoverControl == &interfaceLanguage));
  windowHeight.drawLabels(*context, *buffers, (hoverControl == &windowHeight));
  fixedFramerate.drawLabels(*context, *buffers, (hoverControl == &fixedFramerate));
  buffers->bindLabelBuffer(renderer, windowHeight.isEnabled() ? LabelBufferType::regular : LabelBufferType::disabled);
  windowSizeInfo.draw(renderer);

  widescreenMode.drawLabel(*context, *buffers, (hoverControl == &widescreenMode));
  autosaveOnExit.drawLabel(*context, *buffers, (hoverControl == &autosaveOnExit));
  autoloadOnStart.drawLabel(*context, *buffers, (hoverControl == &autoloadOnStart));
  frameSkipping.drawLabel(*context, *buffers, (hoverControl == &frameSkipping));
}
