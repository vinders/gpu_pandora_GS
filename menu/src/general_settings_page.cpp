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
#include <video/window_keycodes.h>
#include "menu/controls/geometry_generator.h"
#include "menu/general_settings_page.h"

using namespace video_api;
using namespace display;
using namespace display::controls;
using namespace menu::controls;
using namespace menu;

#if !defined(_CPP_REVISION) || _CPP_REVISION != 14
# define SWITCH_FALLTHROUGH  [[fallthrough]]
#else
# define SWITCH_FALLTHROUGH
#endif


// -- helpers -- ---------------------------------------------------------------

static char32_t* FormatInteger(uint32_t value, char32_t* buffer) {
  const size_t offset = (value >= 1000) ? ((value < 10000) ? 3 : 4)
                                        : ((value >= 100) ? 2 : 1);
  for (char32_t* it = buffer + (intptr_t)offset; it >= buffer; --it) {
    *it = U'0' + static_cast<char32_t>(value % 10u);
    value /= 10u;
  }
  return buffer + (intptr_t)(offset + 1u);
}

static std::unique_ptr<char32_t[]> FormatResolution(uint32_t width, uint32_t height) {
  std::unique_ptr<char32_t[]> formatted(new char32_t[14]);
  char32_t* it = FormatInteger(width, formatted.get());
  *it = U' ';
  *(++it) = U'x';
  *(++it) = U' ';
  it = FormatInteger(height, ++it);
  *it = (char32_t)0;

# if !defined(_CPP_REVISION) || _CPP_REVISION != 14
  return formatted;
# else
  return std::move(formatted);
# endif
}

static std::unique_ptr<char32_t[]> FormatRate(uint32_t value) {
  std::unique_ptr<char32_t[]> formatted(new char32_t[12]);
  switch (value) {
    // predefined values
    case 60000:  memcpy(formatted.get(), U"60 Hz",  6*sizeof(char32_t)); break;
    case 59940:  memcpy(formatted.get(), U"59.94 Hz", 9*sizeof(char32_t)); break;
    case 120000: memcpy(formatted.get(), U"120 Hz", 7*sizeof(char32_t)); break;
    case 144000: memcpy(formatted.get(), U"144 Hz", 7*sizeof(char32_t)); break;
    case 240000: memcpy(formatted.get(), U"240 Hz", 7*sizeof(char32_t)); break;
    // custom values
    default: {
      auto* it = FormatInteger(value / 1000u, formatted.get()); // mHz to Hz -- integer part
      value %= 1000u;
      if (value) { // decimals
        const auto* decimalIt = it;
        *it = U'.'; *(++it) = U'0'; *(++it) = U'0'; *(++it) = U'0';
        for (char32_t* revIt = it; revIt > decimalIt; --revIt) {
          *it = U'0' + static_cast<char32_t>(value % 10u);
          value /= 10u;
        }
        ++it;
      }
      *it = U' '; *(++it) = U'H'; *(++it) = U'z';*(++it) = (char32_t)0;
      break;
    }
  }
# if !defined(_CPP_REVISION) || _CPP_REVISION != 14
  return formatted;
# else
  return std::move(formatted);
# endif
}

// ---

static constexpr inline uint32_t ToResolutionId(uint32_t width, uint32_t height) noexcept {
  return ((width << 16) | height);
}
static inline void FromResolutionId(uint32_t id, uint32_t& width, uint32_t& height) noexcept {
  width = ((id >> 16) & 0xFFFFu);
  height = (id & 0xFFFFu);
}
static constexpr inline uint32_t GetWindowWidth(uint32_t windowHeight, bool isWidescreen) noexcept {
  return isWidescreen ? (((windowHeight*16u + 6u)/9u) & ~(uint32_t)0x1) : (windowHeight*4u/3u);
}

// ---

static uint32_t ListFullscreenResolutions(const pandora::hardware::DisplayMonitor& monitor,
                                        std::vector<ComboBoxOption>& fullscreenSizeOptions,
                                        std::unordered_map<uint32_t, std::vector<uint32_t> >& fullscreenRatesPerSize) {
  if (!monitor.attributes().id.empty()) {
    auto desktopMode = monitor.getDisplayMode();
    uint32_t resId = ToResolutionId(desktopMode.width, desktopMode.height);

    fullscreenSizeOptions.emplace_back(FormatResolution(desktopMode.width, desktopMode.height), (ComboValue)resId);
    fullscreenRatesPerSize[resId].emplace_back(desktopMode.refreshRate);
  }

  auto displayModes = monitor.listAvailableDisplayModes();
  fullscreenSizeOptions.reserve(displayModes.size());
  for (const auto& mode : displayModes) {
    if (mode.width >= 640u && mode.height >= 480u && mode.bitDepth >= 24u && mode.refreshRate >= 30u
    && (mode.width < 1100u || mode.width > 1200u)) { // remove nonsense resolutions to reduce list length
      uint32_t resId = ToResolutionId(mode.width, mode.height);

      auto ratePerSizeIt = fullscreenRatesPerSize.find(resId);
      if (ratePerSizeIt == fullscreenRatesPerSize.end()) { // resolution not yet defined -> add to size options
        if (resId == ToResolutionId(1920,1080) && fullscreenSizeOptions.size() > 1u) { // give priority to fullHD
          fullscreenSizeOptions.insert(fullscreenSizeOptions.begin() + 1,
                                       ComboBoxOption(FormatResolution(mode.width, mode.height), (ComboValue)resId));
        }
        else fullscreenSizeOptions.emplace_back(FormatResolution(mode.width, mode.height), (ComboValue)resId);

        fullscreenRatesPerSize[resId].emplace_back(mode.refreshRate); // create list of rates for this resolution
      }
      else { // insert rate in order (for this resolution)
        auto& rates = ratePerSizeIt->second;
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
  return 0;
}

static uint32_t ListFullscreenRates(std::vector<uint32_t>& fullscreenRates, uint32_t previousRate,
                                  std::vector<ComboBoxOption>& fullscreenRateOptions) {
  uint32_t selectedIndex = 0, currentIndex = 0;
  for (auto rate : fullscreenRates) {
    if (rate == previousRate)
      selectedIndex = currentIndex;

    fullscreenRateOptions.emplace_back(FormatRate(rate), (ComboValue)rate);
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

GeneralSettingsPage::GeneralSettingsPage(std::shared_ptr<RendererContext> context_, std::shared_ptr<RendererStateBuffers> buffers_,
                                         const ColorTheme& theme, const pandora::hardware::DisplayMonitor& monitor,
                                         int32_t x, int32_t y, uint32_t width, uint32_t height)
  : Page(std::move(context_), std::move(buffers_), theme, x, y, width, height, true) {
  const uint32_t fieldsetPaddingX = Control::fieldsetMarginX(width);
  const int32_t controlX = x + (int32_t)fieldsetPaddingX + (int32_t)Control::fieldsetContentMarginX(width);
  uint32_t fieldsetWidth = width - (fieldsetPaddingX << 1);
  if (fieldsetWidth > Control::fieldsetMaxWidth())
    fieldsetWidth = Control::fieldsetMaxWidth();

  title = TextMesh(context->renderer(), context->getFont(FontType::titles), U"General settings",
                   context->pixelSizeX(), context->pixelSizeY(), x + (int32_t)fieldsetPaddingX, y + 24, TextAlignment::left);

  std::vector<ControlRegistration> registry;
  registry.reserve(10);
  int32_t currentLineY = title.y() + (int32_t)title.height() + Control::pageLineHeight();
  auto changeHandler = std::bind(&GeneralSettingsPage::onChange,this,std::placeholders::_1,std::placeholders::_2);

  // --- display group ---
  displayGroup = Fieldset(*context, U"Output settings", theme.fieldsetStyle(), theme.fieldsetControlColor(),
                          x + (int32_t)fieldsetPaddingX, currentLineY, fieldsetWidth,
                          Control::pageLineHeight()*3 + (Control::fieldsetContentPaddingY() << 1));
  currentLineY += Control::pageLineHeight() + Control::fieldsetContentPaddingY();

  // display mode
  {
    ComboBoxOption displayModeOptions[]{ ComboBoxOption(U"Fullscreen",  0/*TMP*/),
                                         ComboBoxOption(U"Borderless",  1/*TMP*/),
                                         ComboBoxOption(U"Window mode", 2/*TMP*/) };
    displayMode = Slider(*context, U"Display mode", controlX, currentLineY, Control::pageLabelWidth(),
                         Control::pageControlWidth(), theme.sliderArrowColor(), DISPLAY_MODE_ID, changeHandler,
                         displayModeOptions, sizeof(displayModeOptions)/sizeof(*displayModeOptions), 0);
    registry.emplace_back(displayMode, true, U"Modes: fullscreen / borderless (fullscreen window) / window mode (resizable)");
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
    uint32_t selectedFullSize = ListFullscreenResolutions(monitor, fullscreenSizeOptions, fullscreenRatesPerSize);

    fullscreenSize = ComboBox(*context, U"Fullscreen resolution", controlX, currentLineY, Control::pageLabelWidth(),
                              fullscreenSizeWidth, ComboBoxStyle::classic, theme.comboBoxColorParams(), FULLSCREEN_SIZE_ID, changeHandler,
                              fullscreenSizeOptions.data(), fullscreenSizeOptions.size(), selectedFullSize, &isFullscreenMode);
    registry.emplace_back(fullscreenSize, true, U"Display output resolution (pixels) in fullscreen mode");

    std::vector<ComboBoxOption> fullscreenRateOptions;
    uint32_t selectedFullRate = ListFullscreenRates(fullscreenRatesPerSize[fullscreenSizeOptions[selectedFullSize].value],
                                                    60000, fullscreenRateOptions);
    fullscreenRate = ComboBox(*context, nullptr, fullscreenSize.x() + (int32_t)fullscreenSize.width() + fullscreenRateMargin,
                              currentLineY, 0, fullscreenRateWidth, ComboBoxStyle::cutCorner, theme.comboBoxColorParams(), 0, nullptr,
                              fullscreenRateOptions.data(), fullscreenRateOptions.size(), selectedFullRate, &isFullscreenMode);
    registry.emplace_back(fullscreenRate, true, U"Display output refresh rate (Hz) in fullscreen mode");
    currentLineY += Control::pageLineHeight();
  }
  // window mode size
  constexpr const uint32_t windowHeightValue = 720u;
  constexpr const uint32_t windowHeightBoxOffsetX = Control::pageLabelWidth() + 11 + ((fullscreenSizeWidth+1) >> 1);
  windowHeight = TextBox(*context, U"Window size", nullptr, controlX, currentLineY, windowHeightBoxOffsetX,
                         (fullscreenSizeWidth >> 1) - 11, theme.textBoxControlColor(), WINDOW_SIZE_ID,
                         [this](uint32_t id){ this->onChange(id, this->windowHeight.valueInteger()); },
                         windowHeightValue, 4u, &isWindowMode);
  registry.emplace_back(windowHeight, true, U"Display output height (pixels) in window mode");

  onChange(WINDOW_SIZE_ID, windowHeightValue); // fill 'windowSize' text indicator
  currentLineY += Control::pageLineHeight() + Control::fieldsetContentBottomMargin();

  // --- compatibility group ---
  compatibilityGroup = Fieldset(*context, U"Emulator compatibility", theme.fieldsetStyle(), theme.fieldsetControlColor(),
                                x + (int32_t)fieldsetPaddingX, currentLineY, fieldsetWidth,
                                Control::pageLineHeight()*2 + (Control::fieldsetContentPaddingY() << 1));
  currentLineY += Control::pageLineHeight() + Control::fieldsetContentPaddingY();

  // subprecision modes
  {
    ComboBoxOption subprecisionOptions[]{ ComboBoxOption(U"Original (integer)",      0/*TMP*/),
                                          ComboBoxOption(U"Subprecision (smoother)", 1/*TMP*/) };
    subprecisionMode = Slider(*context, U"Pixel precision", controlX, currentLineY, Control::pageLabelWidth(),
                              Control::pageControlWidth(), theme.sliderArrowColor(), 0, nullptr,
                              subprecisionOptions, sizeof(subprecisionOptions)/sizeof(*subprecisionOptions), 0);
    registry.emplace_back(subprecisionMode, true, U"Anti-jitter geometry subprecision (needed if GTE-subprecision is enabled in emulator)");
    currentLineY += Control::pageLineHeight();
  }
  // widescreen hack
  widescreenMode = CheckBox(*context, U"Widescreen hack", controlX, currentLineY, Control::pageLabelWidth(),
                            WIDESCREEN_HACK_ID, changeHandler, enableWidescreenMode);
  registry.emplace_back(widescreenMode, true, U"Treat geometry coords as 16:9 (needed if widescreen hack is enabled in emulator)");
  currentLineY += Control::pageLineHeight() + Control::fieldsetContentBottomMargin();

  // --- framerate group ---
  framerateGroup = Fieldset(*context, U"Frame rate", theme.fieldsetStyle(), theme.fieldsetControlColor(),
                            x + (int32_t)fieldsetPaddingX, currentLineY, fieldsetWidth,
                            Control::pageLineHeight()*4 + (Control::fieldsetContentPaddingY() << 1));
  currentLineY += Control::pageLineHeight() + Control::fieldsetContentPaddingY();

  // frame rate limit mode
  {
    ComboBoxOption frameLimitOptions[]{ ComboBoxOption(U"Disabled",              0/*TMP*/),
                                        ComboBoxOption(U"Autodetect (NTSC/PAL)", 1/*TMP*/),
                                        ComboBoxOption(U"Fixed limit (custom)",  2/*TMP*/) };
    framerateLimit = Slider(*context, U"Frame rate limit", controlX, currentLineY, Control::pageLabelWidth(),
                            Control::pageControlWidth(), theme.sliderArrowColor(),FRAMERATE_LIMIT_ID, changeHandler,
                            frameLimitOptions, sizeof(frameLimitOptions)/sizeof(*frameLimitOptions), 1);
    registry.emplace_back(framerateLimit, true, U"Enable frame rate limiter (recommended with most emulators)");
    isFramerateLimit = true;
    isFixedFramerate = isFrameSkipping = false;
    currentLineY += Control::pageLineHeight();
  }
  // custom frame rate
  {
    double fixedRateValue = 59.94;
    fixedFramerate = TextBox(*context, U"Custom frame rate", U"fps", controlX, currentLineY, Control::pageLabelWidth(), 80u,
                             theme.textBoxControlColor(), FRAMERATE_FIXED_ID, [this](uint32_t id) { this->onChange(id, 0); },
                             fixedRateValue, 6u, &isFixedFramerate);
    registry.emplace_back(fixedFramerate, true, U"Custom frame rate limit value (frames per second)");
    currentLineY += Control::pageLineHeight();
  }
  // other frame rate settings
  frameSkipping = CheckBox(*context, U"Frame skipping", controlX, currentLineY, Control::pageLabelWidth(),
                           0, nullptr, isFrameSkipping, &isFramerateLimit);
  registry.emplace_back(frameSkipping, true, U"Allow frame skipping to always keep a constant game speed");
  currentLineY += Control::pageLineHeight();

  enableVsync = false;
  vsync = CheckBox(*context, U"Vertical sync", controlX, currentLineY, Control::pageLabelWidth(),
                   0, nullptr, enableVsync, &isFramerateLimit);
  registry.emplace_back(vsync, true, U"Fixes screen tearing and reduces power consumption (but may increase input delay)");
  currentLineY += Control::pageLineHeight() + Control::fieldsetContentBottomMargin();

  // --- user interface group ---
  userInterfaceGroup = Fieldset(*context, U"User interface", theme.fieldsetStyle(), theme.fieldsetControlColor(),
                                x + (int32_t)fieldsetPaddingX, currentLineY, fieldsetWidth,
                                Control::pageLineHeight()*2 + (Control::fieldsetContentPaddingY() << 1));
  currentLineY += Control::pageLineHeight() + Control::fieldsetContentPaddingY();

  // interface color
  {
    ComboBoxOption colorOptions[]{ ComboBoxOption(U"Basic theme",  (ComboValue)ColorThemeType::white),
                                   ComboBoxOption(U"Blue theme",   (ComboValue)ColorThemeType::blue),
                                   ComboBoxOption(U"Green theme",   (ComboValue)ColorThemeType::green),
                                   ComboBoxOption(U"Dark green theme",  (ComboValue)ColorThemeType::darkGreen),
                                   ComboBoxOption(U"Dark yellow theme", (ComboValue)ColorThemeType::darkYellow) };
    interfaceColor = Slider(*context, U"Menu theme", controlX, currentLineY, Control::pageLabelWidth(),
                            Control::pageControlWidth(), theme.sliderArrowColor(), 0, nullptr,
                            colorOptions, sizeof(colorOptions)/sizeof(*colorOptions), 2);
    registry.emplace_back(interfaceColor, true, U"Choose user interface color theme");
    currentLineY += Control::pageLineHeight();
  }
  // interface language
  {
    ComboBoxOption languageOptions[]{ ComboBoxOption(U"English",  0/*TMP*/),
                                      ComboBoxOption(U"Français", 1/*TMP*/) };
    interfaceLanguage = ComboBox(*context, U"Language", controlX, currentLineY, Control::pageLabelWidth(),
                                 105u, ComboBoxStyle::cutCorner, theme.comboBoxColorParams(), 0, nullptr,
                                 languageOptions, sizeof(languageOptions)/sizeof(*languageOptions), 0);
    registry.emplace_back(interfaceLanguage, true, U"Choose user interface language");
  }
  currentLineY += Control::pageLineHeight();// + Control::fieldsetContentBottomMargin();

  // --- control registry ---
  if (currentLineY > y + (int32_t)contentHeight())
    Page::moveScrollbarThumb(currentLineY);
  registerControls(std::move(registry));
}

GeneralSettingsPage::~GeneralSettingsPage() noexcept {
  title.release();

  displayGroup.release();
  displayMode.release();
  fullscreenSize.release();
  fullscreenRate.release();
  windowHeight.release();
  windowSize.release();
  fullscreenRatesPerSize.clear();

  compatibilityGroup.release();
  subprecisionMode.release();
  widescreenMode.release();

  framerateGroup.release();
  framerateLimit.release();
  frameSkipping.release();
  fixedFramerate.release();
  vsync.release();

  userInterfaceGroup.release();
  interfaceColor.release();
  interfaceLanguage.release();
}


// -- window events -- ---------------------------------------------------------

void GeneralSettingsPage::move(int32_t x, int32_t y, uint32_t width, uint32_t height) {
  Page::moveBase(x, y, width, height);
  const uint32_t fieldsetPaddingX = Control::fieldsetMarginX(width);
  const int32_t controlX = x + (int32_t)fieldsetPaddingX + (int32_t)Control::fieldsetContentMarginX(width);
  uint32_t fieldsetWidth = width - (fieldsetPaddingX << 1);
  if (fieldsetWidth > Control::fieldsetMaxWidth())
    fieldsetWidth = Control::fieldsetMaxWidth();

  title.move(context->renderer(), context->pixelSizeX(), context->pixelSizeY(), x + (int32_t)fieldsetPaddingX, y + 24);

  // display group
  int32_t currentLineY = title.y() + (int32_t)title.height() + Control::pageLineHeight();
  displayGroup.move(*context, x + (int32_t)fieldsetPaddingX, currentLineY, fieldsetWidth,
                    Control::pageLineHeight()*3 + (Control::fieldsetContentPaddingY() << 1));
  currentLineY += Control::pageLineHeight() + Control::fieldsetContentPaddingY();

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
  currentLineY += Control::pageLineHeight() + Control::fieldsetContentBottomMargin();

  // compatibility group
  compatibilityGroup.move(*context, x + (int32_t)fieldsetPaddingX, currentLineY, fieldsetWidth,
                          Control::pageLineHeight()*2 + (Control::fieldsetContentPaddingY() << 1));
  currentLineY += Control::pageLineHeight() + Control::fieldsetContentPaddingY();

  subprecisionMode.move(*context, controlX, currentLineY);
  currentLineY += Control::pageLineHeight();
  widescreenMode.move(*context, controlX, currentLineY);
  currentLineY += Control::pageLineHeight() + Control::fieldsetContentBottomMargin();

  // framerate group
  framerateGroup.move(*context, x + (int32_t)fieldsetPaddingX, currentLineY, fieldsetWidth,
                      Control::pageLineHeight()*4 + (Control::fieldsetContentPaddingY() << 1));
  currentLineY += Control::pageLineHeight() + Control::fieldsetContentPaddingY();

  framerateLimit.move(*context, controlX, currentLineY);
  currentLineY += Control::pageLineHeight();
  fixedFramerate.move(*context, controlX, currentLineY);
  currentLineY += Control::pageLineHeight();
  frameSkipping.move(*context, controlX, currentLineY);
  currentLineY += Control::pageLineHeight();
  vsync.move(*context, controlX, currentLineY);
  currentLineY += Control::pageLineHeight() + Control::fieldsetContentBottomMargin();

  // user interface group
  userInterfaceGroup.move(*context, x + (int32_t)fieldsetPaddingX, currentLineY, fieldsetWidth,
                          Control::pageLineHeight()*2 + (Control::fieldsetContentPaddingY() << 1));
  currentLineY += Control::pageLineHeight() + Control::fieldsetContentPaddingY();

  interfaceColor.move(*context, controlX, currentLineY);
  currentLineY += Control::pageLineHeight();
  interfaceLanguage.move(*context, controlX, currentLineY);
  currentLineY += Control::pageLineHeight();// + Control::fieldsetContentBottomMargin();

  Page::moveScrollbarThumb(currentLineY); // required after a move
}

void GeneralSettingsPage::onChange(uint32_t id, uint32_t value) {
  switch (id) {
    case DISPLAY_MODE_ID: {
      isFullscreenMode = (value == 0/*TMP*/);
      isWindowMode = (value == 2/*TMP*/);
      break;
    }
    case FULLSCREEN_SIZE_ID: {
      std::vector<ComboBoxOption> fullscreenRateOptions;
      const auto* currentRate = fullscreenRate.getSelectedValue();
      uint32_t selectedFullRate = ListFullscreenRates(fullscreenRatesPerSize[value], currentRate ? *currentRate : 60000, fullscreenRateOptions);
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
      char32_t windowWidthBuffer[14];
      memcpy(windowWidthBuffer, U"           x", 13*sizeof(char32_t));
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

void GeneralSettingsPage::drawIcons() {
  // scrollable geometry
  buffers->bindScrollLocationBuffer(context->renderer(), ScissorRectangle(x(), y(), width(), contentHeight()));

  auto* hoverControl = getActiveControl();
  widescreenMode.drawIcon(*context, *buffers, (hoverControl == &widescreenMode));
  frameSkipping.drawIcon(*context, *buffers, (hoverControl == &frameSkipping));
  vsync.drawIcon(*context, *buffers, (hoverControl == &vsync));
}

bool GeneralSettingsPage::drawPageBackgrounds(int32_t mouseX, int32_t) {
  // scrollable geometry
  if (buffers->isFixedLocationBuffer())
    buffers->bindScrollLocationBuffer(context->renderer(), ScissorRectangle(x(), y(), width(), contentHeight()));

  auto* hoverControl = getActiveControl();
  displayGroup.drawBackground(*context, *buffers);
  compatibilityGroup.drawBackground(*context, *buffers);
  framerateGroup.drawBackground(*context, *buffers);
  userInterfaceGroup.drawBackground(*context, *buffers);

  displayMode.drawBackground(*context, mouseX, *buffers, (hoverControl == &displayMode));
  subprecisionMode.drawBackground(*context, mouseX, *buffers, (hoverControl == &subprecisionMode));
  framerateLimit.drawBackground(*context, mouseX, *buffers, (hoverControl == &framerateLimit));
  interfaceColor.drawBackground(*context, mouseX, *buffers, (hoverControl == &interfaceColor));

  fullscreenSize.drawBackground(*context, *buffers, (hoverControl == &fullscreenSize));
  fullscreenRate.drawBackground(*context, *buffers, (hoverControl == &fullscreenRate));
  interfaceLanguage.drawBackground(*context, *buffers, (hoverControl == &interfaceLanguage));
  windowHeight.drawBackground(*context, *buffers);
  fixedFramerate.drawBackground(*context, *buffers);
  return false;

  //float color[4]{ 0.4f,0.4f,0.4f,1.f };
  //float colorBorder[4]{ 0.3f,0.3f,0.3f,1.f };
  //Button testbtn(*context, U"Abcdef", 200, 500, ButtonStyle(color, FontType::inputText, ControlIconType::none, 180, 12, 7), 0, nullptr, colorBorder, nullptr);
  //testbtn.drawBackground(*context);
  //return hasForeground;

  //float color[4]{ 0.4f,0.4f,0.4f,1.f };
  //float colorBorder[4]{ 0.3f,0.3f,0.3f,1.f };
  //float thumbColor[4]{ 0.7f,0.7f,0.7f,1.f };
  //float fillColor[4]{ 0.f,83.f/255.f,166.f/255.f,1.f };
  //uint32_t value = 2;
  //Ruler ruler(*context, U"Abcdef", U"xyz", TextAlignment::right, 200, 500, ControlStyle(color, 0, 10, 4), 180, colorBorder, thumbColor, fillColor, 0, 4, 1, value);
  //ruler.drawBackground(*context);
  //return hasForeground;
}

void GeneralSettingsPage::drawPageLabels() {
  // scrollable geometry
  auto& renderer = context->renderer();
  if (buffers->isFixedLocationBuffer())
    buffers->bindScrollLocationBuffer(renderer, ScissorRectangle(x(), y(), width(), contentHeight()));

  buffers->bindLabelBuffer(renderer, LabelBufferType::title);
  title.draw(renderer);

  auto* hoverControl = getActiveControl();
  displayGroup.drawLabel(*context, *buffers);
  compatibilityGroup.drawLabel(*context, *buffers);
  framerateGroup.drawLabel(*context, *buffers);
  userInterfaceGroup.drawLabel(*context, *buffers);

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
  frameSkipping.drawLabel(*context, *buffers, (hoverControl == &frameSkipping));
  vsync.drawLabel(*context, *buffers, (hoverControl == &vsync));
}

void GeneralSettingsPage::drawForegrounds() {
  auto& renderer = context->renderer();

  ScissorRectangle fullWindowArea(0, 0, context->clientWidth(), context->clientHeight());
  buffers->bindScrollLocationBuffer(renderer, fullWindowArea); // visible outside of scroll area -> full window

  fullscreenSize.drawDropdown(*context, *buffers);
  fullscreenRate.drawDropdown(*context, *buffers);
  interfaceLanguage.drawDropdown(*context, *buffers);
}

void GeneralSettingsPage::drawForegroundLabels() {
  ScissorRectangle fullWindowArea(0, 0, context->clientWidth(), context->clientHeight());
  buffers->bindScrollLocationBuffer(context->renderer(), fullWindowArea); // visible outside of scroll area -> full window

  fullscreenSize.drawOptions(*context, *buffers);
  fullscreenRate.drawOptions(*context, *buffers);
  interfaceLanguage.drawOptions(*context, *buffers);
}
