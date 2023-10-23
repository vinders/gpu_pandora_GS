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
#include "menu/general_settings_page.h"

using namespace video_api;
using namespace display;
using namespace display::controls;
using namespace menu::controls;
using namespace menu;


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

// ---

static inline void setControlVertex(ControlVertex& outVertex, const float rgba[4], float x, float y) {
  float* position = outVertex.position;
  *position = x;
  *(++position) = y;
  *(++position) = 0.f; // z
  *(++position) = 1.f; // w
  memcpy(outVertex.color, rgba, 4*sizeof(float));
}


// -- page -- ------------------------------------------------------------------

#define PAGE_HEIGHT      1200
#define LINE_HEIGHT      26
#define FIELDSET_PAD_Y   10
#define FIELDSET_FIRST_Y 5
#define FIELDSETS_MARGIN 12
#define LABEL_WIDTH      200
#define VALUE_WIDTH      300

#define DISPLAY_MODE_ID      1
#define FULLSCREEN_SIZE_ID   2
#define FULLSCREEN_RATE_ID   3
#define SUBPRECISION_MODE_ID 4
#define FRAMERATE_LIMIT_ID   5

GeneralSettingsPage::GeneralSettingsPage(std::shared_ptr<RendererContext> context_, const ColorTheme& theme,
                                         const pandora::hardware::DisplayMonitor& monitor,
                                         int32_t x, int32_t y, uint32_t width, uint32_t height)
  : Page(context_, x, y, width, height, 1200),
    context(std::move(context_)),
    x(x),
    width(width) {
  constexpr const uint32_t fieldsetPaddingX = 30;
  const int32_t leftX = (width >= LABEL_WIDTH + VALUE_WIDTH + 70) ? (x + 50) : (x + 30);

  // --- page decoration ---
  auto& labelFont = context->getFont(FontType::labels);
  scrollbar = ScrollBar(*context, theme.scrollbarControl, theme.scrollbarThumb,
                        x + (int32_t)width - (int32_t)theme.scrollbarWidth, y, theme.scrollbarWidth, height,
                        std::bind(&GeneralSettingsPage::onScroll,this,std::placeholders::_1), height, PAGE_HEIGHT, (LINE_HEIGHT >> 1));
  title = TextMesh(context->renderer(), context->getFont(FontType::titles), U"General settings",
                   context->pixelSizeX(), context->pixelSizeY(), x + (int32_t)fieldsetPaddingX, y + 24, TextAlignment::left);
  tooltip = TextMesh(context->renderer(), labelFont, U"-", context->pixelSizeX(), context->pixelSizeY(),
                     x + (int32_t)fieldsetPaddingX, y + height - 20, TextAlignment::left);

  const uint32_t tooltipAreaWidth = scrollbar.isEnabled() ? (width - scrollbar.width()) : width;
  const uint32_t tooltipAreaHeight = 41 - (int32_t)tooltip.height();
  const float tooltipBackDarker[4]{ theme.tooltipBackground[0]*0.85f, theme.tooltipBackground[1]*0.85f,
                                    theme.tooltipBackground[2]*0.85f, theme.tooltipBackground[3] };
  std::vector<ControlVertex> tooltipAreaVertices;
  tooltipAreaVertices.resize(4);
  ControlVertex* vertexIt = &tooltipAreaVertices[0];
  setControlVertex(*vertexIt,     theme.tooltipBackground, 0.f,                     0.f);
  setControlVertex(*(++vertexIt), theme.tooltipBackground, (float)tooltipAreaWidth, 0.f);
  setControlVertex(*(++vertexIt), tooltipBackDarker,       0.f,                     -(float)tooltipAreaHeight);
  setControlVertex(*(++vertexIt), tooltipBackDarker,       (float)tooltipAreaWidth, -(float)tooltipAreaHeight);
  std::vector<uint32_t> tooltipAreaIndices{ 0,1,2, 2,1,3 };
  tooltipBar = ControlMesh(context->renderer(), std::move(tooltipAreaVertices), tooltipAreaIndices,
                           context->pixelSizeX(), context->pixelSizeY(), x, y + (int32_t)height - (int32_t)tooltipAreaHeight,
                           tooltipAreaWidth, tooltipAreaHeight);
  
  scroll = 0;
  ScrollUniform scrollLocation{ { 0.f, 0.f, 0.f, 0.f } };
  scrollPosition = Buffer<ResourceUsage::staticGpu>(context->renderer(), BufferType::uniform, sizeof(ScrollUniform), &scrollLocation);
  scrollPositionStaging = Buffer<ResourceUsage::staging>(context->renderer(), BufferType::uniform, sizeof(ScrollUniform), &scrollLocation);

  const uint32_t hoverAreaWidth = (LABEL_WIDTH + VALUE_WIDTH + 26);
  std::vector<ControlVertex> hoverAreaVertices;
  hoverAreaVertices.resize(8);
  vertexIt = &hoverAreaVertices[0];
  setControlVertex(*vertexIt,     theme.hoverArea, 3.f,                         0.f);
  setControlVertex(*(++vertexIt), theme.hoverArea, (float)(hoverAreaWidth - 3), 0.f);
  setControlVertex(*(++vertexIt), theme.hoverArea, 0.f,                   -3.f);
  setControlVertex(*(++vertexIt), theme.hoverArea, (float)hoverAreaWidth, -3.f);
  setControlVertex(*(++vertexIt), theme.hoverArea, 0.f,                   -(float)(LINE_HEIGHT - 3));
  setControlVertex(*(++vertexIt), theme.hoverArea, (float)hoverAreaWidth, -(float)(LINE_HEIGHT - 3));
  setControlVertex(*(++vertexIt), theme.hoverArea, 3.f,                         -(float)LINE_HEIGHT);
  setControlVertex(*(++vertexIt), theme.hoverArea, (float)(hoverAreaWidth - 3), -(float)LINE_HEIGHT);
  std::vector<uint32_t> hoverAreaIndices{ 0,1,2, 2,1,3,  2,3,4, 4,3,5,  4,5,6, 6,5,7 };
  lineHoverArea = ControlMesh(context->renderer(), std::move(hoverAreaVertices), hoverAreaIndices,
                              context->pixelSizeX(), context->pixelSizeY(), leftX - 10, 0, hoverAreaWidth, LINE_HEIGHT);
  activeLineMiddleY = activeLineSelector = noLineSelection();

  int32_t currentLineY = title.y() + (int32_t)title.height() + LINE_HEIGHT;
  auto changeHandler = std::bind(&GeneralSettingsPage::onChange,this,std::placeholders::_1,std::placeholders::_2);

  // --- display group ---
  displayGroup = Fieldset(*context, U"Output settings", theme.fieldsetStyle, theme.fieldsetControl,
                          x + (int32_t)fieldsetPaddingX, currentLineY, 12, 10, width - (fieldsetPaddingX << 1), LINE_HEIGHT*3 + FIELDSET_PAD_Y);
  currentLineY += LINE_HEIGHT + FIELDSET_FIRST_Y;

  // display mode
  ComboBoxOption displayModeOptions[]{ ComboBoxOption(U"Fullscreen",  0/*TMP*/),
                                       ComboBoxOption(U"Borderless",  1/*TMP*/),
                                       ComboBoxOption(U"Window mode", 2/*TMP*/) };
  ControlStyle sliderStyle(theme.sliderArrows, LABEL_WIDTH, 14, 6);
  displayMode = Slider(*context, U"Display mode", leftX, currentLineY, sliderStyle, VALUE_WIDTH, DISPLAY_MODE_ID,
                       changeHandler, displayModeOptions, sizeof(displayModeOptions)/sizeof(*displayModeOptions), 0);
  isFullscreenMode = true;
  isWindowMode = false;
  enableWidescreenMode = false;
  currentLineY += LINE_HEIGHT;

  // fullscreen resolution/rate
  std::vector<ComboBoxOption> fullscreenSizeOptions;
  uint32_t selectedFullSize = ListFullscreenResolutions(monitor, fullscreenSizeOptions, fullscreenRatesPerSize);

  ControlStyle comboStyle(theme.comboBoxControl, LABEL_WIDTH, 10, 7);
  const uint32_t fullscreenSizeWidth = VALUE_WIDTH - 113;
  fullscreenSize = ComboBox(*context, U"Fullscreen resolution", leftX, currentLineY, comboStyle, fullscreenSizeWidth,
                            theme.comboBoxDropdown, FULLSCREEN_SIZE_ID, changeHandler,
                            fullscreenSizeOptions.data(), fullscreenSizeOptions.size(), selectedFullSize, &isFullscreenMode);

  std::vector<ComboBoxOption> fullscreenRateOptions;
  uint32_t selectedFullRate = ListFullscreenRates(fullscreenRatesPerSize[fullscreenSizeOptions[selectedFullSize].value], 60000, fullscreenRateOptions);
  comboStyle.minLabelWidth = 0;
  fullscreenRate = ComboBox(*context, nullptr, fullscreenSize.x() + (int32_t)fullscreenSize.width() + 3, currentLineY, comboStyle, 110,
                            theme.comboBoxDropdown, FULLSCREEN_RATE_ID, changeHandler,
                            fullscreenRateOptions.data(), fullscreenRateOptions.size(), selectedFullRate, &isFullscreenMode);
  currentLineY += LINE_HEIGHT;

  // window mode size
  uint32_t windowHeightValue = 720u;
  ControlStyle textBoxStyle(theme.textBoxControl, LABEL_WIDTH + (fullscreenSizeWidth >> 1), 10, 6);
  windowHeight = TextBox(*context, U"Window size", nullptr, leftX, currentLineY, textBoxStyle, (fullscreenSizeWidth >> 1), windowHeightValue, 4u, &isWindowMode);

  char32_t windowWidthBuffer[14];
  memcpy(windowWidthBuffer, U"            x", 14*sizeof(char32_t));
  FormatInteger(GetWindowWidth(windowHeightValue, enableWidescreenMode), windowWidthBuffer);
  auto& inputFont = context->getFont(FontType::inputText);
  windowSize = TextMesh(context->renderer(), inputFont, windowWidthBuffer,
                        context->pixelSizeX(), context->pixelSizeY(), windowHeight.controlX() - 8,
                        currentLineY + (int32_t)(labelFont.XHeight() - inputFont.XHeight() + 1)/2, TextAlignment::right);
  currentLineY += LINE_HEIGHT;

  // --- compatibility group ---
  currentLineY += FIELDSETS_MARGIN;
  compatibilityGroup = Fieldset(*context, U"Emulator compatibility", theme.fieldsetStyle, theme.fieldsetControl,
                                x + (int32_t)fieldsetPaddingX, currentLineY, 12, 10, width - (fieldsetPaddingX << 1), LINE_HEIGHT*2 + FIELDSET_PAD_Y);
  currentLineY += LINE_HEIGHT + FIELDSET_FIRST_Y;

  ComboBoxOption subprecisionOptions[]{ ComboBoxOption(U"Original (integer)",      0/*TMP*/),
                                        ComboBoxOption(U"Subprecision (smoother)", 1/*TMP*/) };
  subprecisionMode = Slider(*context, U"Pixel precision", leftX, currentLineY, sliderStyle, VALUE_WIDTH, SUBPRECISION_MODE_ID,
                            changeHandler, subprecisionOptions, sizeof(subprecisionOptions)/sizeof(*subprecisionOptions), 0);
  currentLineY += LINE_HEIGHT;

  widescreenMode = CheckBox(*context, U"Widescreen hack", leftX, currentLineY, true, LABEL_WIDTH, enableWidescreenMode);
  currentLineY += LINE_HEIGHT;

  // --- framerate group ---
  currentLineY += FIELDSETS_MARGIN;
  framerateGroup = Fieldset(*context, U"Frame rate", theme.fieldsetStyle, theme.fieldsetControl,
                            x + (int32_t)fieldsetPaddingX, currentLineY, 12, 10, width - (fieldsetPaddingX << 1), LINE_HEIGHT*4 + FIELDSET_PAD_Y);
  currentLineY += LINE_HEIGHT + FIELDSET_FIRST_Y;

  ComboBoxOption frameLimitOptions[]{ ComboBoxOption(U"Disabled",              0/*TMP*/),
                                      ComboBoxOption(U"Autodetect (NTSC/PAL)", 1/*TMP*/),
                                      ComboBoxOption(U"Fixed limit (custom)",  2/*TMP*/) };
  framerateLimit = Slider(*context, U"Frame rate limit", leftX, currentLineY, sliderStyle, VALUE_WIDTH, FRAMERATE_LIMIT_ID,
                          changeHandler, frameLimitOptions, sizeof(frameLimitOptions)/sizeof(*frameLimitOptions), 1);
  isFramerateLimit = true;
  isFixedFramerate = false;
  currentLineY += LINE_HEIGHT;

  double fixedRateValue = 59.94;
  textBoxStyle.minLabelWidth = LABEL_WIDTH;
  fixedFramerate = TextBox(*context, U"Custom frame rate", U"fps", leftX, currentLineY, textBoxStyle, 80u, fixedRateValue, 6u, &isFixedFramerate);
  currentLineY += LINE_HEIGHT;

  isFrameSkipping = false;
  frameSkipping = CheckBox(*context, U"Frame skipping", leftX, currentLineY, true, LABEL_WIDTH, isFrameSkipping, &isFramerateLimit);
  currentLineY += LINE_HEIGHT;

  enableVsync = false;
  vsync = CheckBox(*context, U"Vertical sync", leftX, currentLineY, true, LABEL_WIDTH, enableVsync, &isFramerateLimit);
  //currentLineY += LINE_HEIGHT;
}

void GeneralSettingsPage::release() noexcept {
  fullscreenRatesPerSize.clear();
  scrollPosition.release();
  scrollPositionStaging.release();

  scrollbar.release();
  title.release();
  tooltipBar.release();
  tooltip.release();
  lineHoverArea.release();

  displayGroup.release();
  displayMode.release();
  fullscreenSize.release();
  fullscreenRate.release();
  windowHeight.release();
  windowSize.release();

  compatibilityGroup.release();
  subprecisionMode.release();
  widescreenMode.release();

  framerateGroup.release();
  framerateLimit.release();
  frameSkipping.release();
  fixedFramerate.release();
  vsync.release();

  context = nullptr;
}


// -- window event --

void GeneralSettingsPage::move(int32_t x_, int32_t y, uint32_t width_, uint32_t height) {
  this->x = x_;
  this->width = width_;
  const int32_t leftX = (width >= LABEL_WIDTH + VALUE_WIDTH + 70) ? (x + 50) : (x + 30);
  constexpr const uint32_t fieldsetPaddingX = 30;

  // page decoration
  auto& labelFont = context->getFont(FontType::labels);
  scrollbar.move(*context, x + (int32_t)width - (int32_t)scrollbar.width(), y, scrollbar.width(), height, PAGE_HEIGHT);
  title.move(context->renderer(), context->pixelSizeX(), context->pixelSizeY(), x + (int32_t)fieldsetPaddingX, y + 24);
  tooltip.move(context->renderer(), context->pixelSizeX(), context->pixelSizeY(), x + (int32_t)fieldsetPaddingX, y + height - 20);

  const uint32_t tooltipAreaWidth = scrollbar.isEnabled() ? (width - scrollbar.width()) : width;
  const uint32_t tooltipAreaHeight = 41 - (int32_t)tooltip.height();
  std::vector<ControlVertex> tooltipAreaVertices = tooltipBar.relativeVertices();
  tooltipAreaVertices[1].position[0] = (float)tooltipAreaWidth;
  tooltipAreaVertices[3].position[0] = (float)tooltipAreaWidth;
  tooltipBar.update(context->renderer(), std::move(tooltipAreaVertices), context->pixelSizeX(), context->pixelSizeY(),
                    x, y + (int32_t)height - (int32_t)tooltipAreaHeight, tooltipAreaWidth, tooltipAreaHeight);

  if (scroll > PAGE_HEIGHT - height)
    onScroll(PAGE_HEIGHT - height);

  lineHoverArea.move(context->renderer(), context->pixelSizeX(), context->pixelSizeY(), x + (int32_t)fieldsetPaddingX + 10, 0);

  int32_t currentLineY = title.y() + (int32_t)title.height() + LINE_HEIGHT;
  activeLineSelector = noLineSelection();
  onHover(noLineSelection(), 0, nullptr);

  // display group
  displayGroup.move(*context, x + (int32_t)fieldsetPaddingX, currentLineY, width - (fieldsetPaddingX << 1), LINE_HEIGHT*3 + FIELDSET_PAD_Y);
  currentLineY += LINE_HEIGHT + FIELDSET_FIRST_Y;

  displayMode.move(*context, leftX, currentLineY);
  currentLineY += LINE_HEIGHT;
  fullscreenSize.move(*context, leftX, currentLineY);
  fullscreenRate.move(*context, fullscreenSize.x() + (int32_t)fullscreenSize.width() + 3, currentLineY);
  currentLineY += LINE_HEIGHT;
  const int32_t windowSizeOffsetX = windowSize.x() - windowHeight.controlX();
  const int32_t windowSizeOffsetY = windowSize.y() - (windowHeight.y() + windowHeight.paddingTop());
  windowHeight.move(*context, leftX, currentLineY);
  windowSize.move(context->renderer(), context->pixelSizeX(), context->pixelSizeY(),
                  windowHeight.controlX() + windowSizeOffsetX, currentLineY + windowSizeOffsetY);
  currentLineY += LINE_HEIGHT;

  // compatibility group
  currentLineY += FIELDSETS_MARGIN;
  compatibilityGroup.move(*context, x + (int32_t)fieldsetPaddingX, currentLineY, width - (fieldsetPaddingX << 1), LINE_HEIGHT*2 + FIELDSET_PAD_Y);
  currentLineY += LINE_HEIGHT + FIELDSET_FIRST_Y;

  subprecisionMode.move(*context, leftX, currentLineY);
  currentLineY += LINE_HEIGHT;
  widescreenMode.move(*context, leftX, currentLineY);
  currentLineY += LINE_HEIGHT;

  // framerate group
  currentLineY += FIELDSETS_MARGIN;
  framerateGroup.move(*context, x + (int32_t)fieldsetPaddingX, currentLineY, width - (fieldsetPaddingX << 1), LINE_HEIGHT*4 + FIELDSET_PAD_Y);
  currentLineY += LINE_HEIGHT + FIELDSET_FIRST_Y;

  framerateLimit.move(*context, leftX, currentLineY);
  currentLineY += LINE_HEIGHT;
  fixedFramerate.move(*context, leftX, currentLineY);
  currentLineY += LINE_HEIGHT;
  frameSkipping.move(*context, leftX, currentLineY);
  currentLineY += LINE_HEIGHT;
  vsync.move(*context, leftX, currentLineY);
  //currentLineY += LINE_HEIGHT;
}


// -- user interactions --

void GeneralSettingsPage::mouseDown(int32_t mouseX, int32_t mouseY) {
  // click elsewhere with combo-box/text-box active, close it
  if (fullscreenSize.isOpen()) {
    if (fullscreenSize.isHover(mouseX, mouseY + (int32_t)scroll) && mouseX >= fullscreenSize.controlX())
      fullscreenSize.click(*context);
    else
      fullscreenSize.close();
    return;
  }
  else if (fullscreenRate.isOpen()) {
    if (fullscreenRate.isHover(mouseX, mouseY + (int32_t)scroll) && mouseX >= fullscreenRate.controlX())
      fullscreenRate.click(*context);
    else
      fullscreenRate.close();
    return;
  }
  else if (windowHeight.isEditMode() && !windowHeight.isHover(mouseX, mouseY + (int32_t)scroll)) {
    windowHeight.close();

    char32_t windowWidthBuffer[14];
    memcpy(windowWidthBuffer, U"            x", 14*sizeof(char32_t));
    FormatInteger(GetWindowWidth(windowHeight.valueInteger(), enableWidescreenMode), windowWidthBuffer);
    windowSize = TextMesh(context->renderer(), context->getFont(FontType::inputText), windowWidthBuffer,
                          context->pixelSizeX(), context->pixelSizeY(), windowHeight.controlX() - 8, windowSize.y(), TextAlignment::right);
  }
  else if (fixedFramerate.isEditMode() && !fixedFramerate.isHover(mouseX, mouseY + (int32_t)scroll))
    fixedFramerate.close();

  // click on scrollbar
  if (mouseX >= scrollbar.x()) {
    if (scrollbar.isEnabled() && scrollbar.isHover(mouseX, mouseY))
      scrollbar.click(*context, mouseY, true);
    return;
  }

  // page control click detection (dichotomy)
  mouseY += (int32_t)scroll;
  if (mouseY < compatibilityGroup.y()) {
    if (mouseY < windowHeight.y()) {
      if (mouseY < displayMode.y())
        return;
      if (mouseY < fullscreenSize.y()) {
        if (displayMode.isHover(mouseX, mouseY))
          displayMode.click(mouseX);
      }
      else if (fullscreenSize.isHover(mouseX, mouseY))
        fullscreenSize.click(*context);
      else if (fullscreenRate.isHover(mouseX, mouseY))
        fullscreenRate.click(*context);
    }
    else if (windowHeight.isHover(mouseX, mouseY))
      windowHeight.click(*context, mouseX);
  }
  else if (mouseY < framerateGroup.y()) {
    if (subprecisionMode.isHover(mouseX, mouseY))
      subprecisionMode.click(mouseX);
    else if (widescreenMode.isHover(mouseX, mouseY)) {
      widescreenMode.click();

      char32_t windowWidthBuffer[14];
      memcpy(windowWidthBuffer, U"            x", 14*sizeof(char32_t));
      FormatInteger(GetWindowWidth(windowHeight.valueInteger(), enableWidescreenMode), windowWidthBuffer);
      windowSize = TextMesh(context->renderer(), context->getFont(FontType::inputText), windowWidthBuffer,
                            context->pixelSizeX(), context->pixelSizeY(), windowHeight.controlX() - 8, windowSize.y(), TextAlignment::right);
    }
  }
  else { // mouseY >= framerateGroup.y()
    if (mouseY < frameSkipping.y()) {
      if (framerateLimit.isHover(mouseX, mouseY))
        framerateLimit.click(mouseX);
      else if (fixedFramerate.isHover(mouseX, mouseY))
        fixedFramerate.click(*context, mouseX);
    }
    else {
      if (frameSkipping.isHover(mouseX, mouseY))
        frameSkipping.click();
      else if (vsync.isHover(mouseX, mouseY))
        vsync.click();
    }
  }
}

void GeneralSettingsPage::mouseMove(int32_t mouseX, int32_t mouseY) {
  activeLineSelector = noLineSelection();
  if (scrollbar.isDragged()) {
    scrollbar.mouseMove(*context, mouseY);
  }
  else if (fullscreenSize.isOpen()) {
    if (fullscreenSize.isHover(mouseX, mouseY))
      fullscreenSize.mouseMove(*context, mouseY);
  }
  else if (fullscreenRate.isOpen()) {
    if (fullscreenRate.isHover(mouseX, mouseY))
      fullscreenRate.mouseMove(*context, mouseY);
  }
  else { // update hover line + tooltip
    mouseY += (int32_t)scroll;
    int32_t lineMiddleY = noLineSelection();
    uint32_t lineControlWidth = 0;
    const char32_t* tooltipValue = nullptr;

    if (mouseX > displayGroup.x() && mouseX < displayGroup.x() + (int32_t)displayGroup.width() && mouseY >= displayMode.y()) {
      if (mouseY < compatibilityGroup.y()) {
        if (mouseY < windowHeight.y()) {
          if (mouseY < fullscreenSize.y()) {
            if (displayMode.isHover(mouseX, mouseY)) {
              lineMiddleY = displayMode.middleY();
              lineControlWidth = LABEL_WIDTH + VALUE_WIDTH + 6;
              tooltipValue = U"Fullscreen / borderless (fullscreen window) / window mode (resizable)";
            }
          }
          else if (fullscreenSize.isHover(mouseX, mouseY)) {
            lineMiddleY = fullscreenSize.middleY();
            lineControlWidth = LABEL_WIDTH + VALUE_WIDTH + 6;
            tooltipValue = U"Display output resolution (pixels) in fullscreen mode";
          }
          else if (fullscreenRate.isHover(mouseX, mouseY)) {
            lineMiddleY = fullscreenRate.middleY();
            lineControlWidth = LABEL_WIDTH + VALUE_WIDTH + 6;
            tooltipValue = U"Display output refresh rate (Hz) in fullscreen mode";
          }
        }
        else if (windowHeight.isHover(mouseX, mouseY)) {
          lineMiddleY = windowHeight.middleY();
          lineControlWidth = windowHeight.width();
          tooltipValue = U"Display output height (pixels) in window mode";
        }
      }
      else if (mouseY < framerateGroup.y()) {
        if (subprecisionMode.isHover(mouseX, mouseY)) {
          lineMiddleY = subprecisionMode.middleY();
          lineControlWidth = LABEL_WIDTH + VALUE_WIDTH + 6;
          tooltipValue = U"Anti-jitter geometry subprecision (needed if GTE-subprecision enabled in emulator)";
        }
        else if (widescreenMode.isHover(mouseX, mouseY)) {
          lineMiddleY = widescreenMode.middleY();
          lineControlWidth = widescreenMode.width();
          tooltipValue = U"Treat geometry coords as 16:9 (needed if widescreen hack enabled in emulator)";
        }
      }
      else { // mouseY >= framerateGroup.y()
        if (mouseY < frameSkipping.y()) {
          if (framerateLimit.isHover(mouseX, mouseY)) {
            lineMiddleY = framerateLimit.middleY();
            lineControlWidth = LABEL_WIDTH + VALUE_WIDTH + 6;
            tooltipValue = U"Enable frame rate limiter (recommended with most emulators)";
          }
          else if (fixedFramerate.isHover(mouseX, mouseY)) {
            lineMiddleY = fixedFramerate.middleY();
            lineControlWidth = fixedFramerate.width();
            tooltipValue = U"Custom frame rate limit value (frames per second)";
          }
        }
        else {
          if (frameSkipping.isHover(mouseX, mouseY)) {
            lineMiddleY = frameSkipping.middleY();
            lineControlWidth = frameSkipping.width();
            tooltipValue = U"Allow frame skipping to always keep a constant game speed";
          }
          else if (vsync.isHover(mouseX, mouseY)) {
            lineMiddleY = vsync.middleY();
            lineControlWidth = vsync.width();
            tooltipValue = U"Fixes screen tearing and reduces power consumption (but may increase input delay)";
          }
        }
      }
    }
    onHover(lineMiddleY, lineControlWidth, tooltipValue);
  }
}

void GeneralSettingsPage::mouseUp(int32_t mouseX, int32_t mouseY) {
  if (scrollbar.isDragged())
    scrollbar.mouseUp(*context, mouseY);
}

void GeneralSettingsPage::mouseScroll(int32_t deltaY) {
  if (scrollbar.isEnabled())
    scrollbar.scroll(*context, deltaY);
}

// ---

void GeneralSettingsPage::keyDown(char32_t keyCode) {
  if ((keyCode > U'\x1F' && keyCode < U'\x7F') || keyCode > U'\x9F') {
    if (windowHeight.isEditMode())
      windowHeight.addChar(*context, keyCode);
    else if (fixedFramerate.isEditMode())
      fixedFramerate.addChar(*context, keyCode);
  }
}

void GeneralSettingsPage::vkeyDown(uint32_t virtualKeyCode) {
  switch (virtualKeyCode) {
    case _P_VK_ENTER:
      if (activeLineSelector == 1)
        fullscreenSize.click(*context);
      else if (activeLineSelector == 2)
        fullscreenRate.click(*context);
      break;
    case _P_VK_DELETE:
      if (windowHeight.isEditMode()) {
        windowHeight.nextChar(*context);
        windowHeight.removeChar(*context);
      }
      else if (fixedFramerate.isEditMode()) {
        fixedFramerate.nextChar(*context);
        fixedFramerate.removeChar(*context);
      }
      break;
    case _P_VK_BACKSPACE:
      if (windowHeight.isEditMode())
        windowHeight.removeChar(*context);
      else if (fixedFramerate.isEditMode())
        fixedFramerate.removeChar(*context);
      break;
    case _P_VK_ARROW_LEFT:
      if (windowHeight.isEditMode())
        windowHeight.previousChar(*context);
      else if (fixedFramerate.isEditMode())
        fixedFramerate.previousChar(*context);
      break;
    case _P_VK_ARROW_RIGHT:
      if (windowHeight.isEditMode())
        windowHeight.nextChar(*context);
      else if (fixedFramerate.isEditMode())
        fixedFramerate.nextChar(*context);
      break;
    case _P_VK_ARROW_UP:
      if (fullscreenSize.isOpen())
        fullscreenSize.selectPrevious(*context);
      else if (fullscreenRate.isOpen())
        fullscreenRate.selectPrevious(*context);
      else {
        if (activeLineSelector == noLineSelection())
          activeLineSelector = 0;
        else
          --activeLineSelector;
        onLineSelection(activeLineSelector);
      }
      break;
    case _P_VK_TAB:
    case _P_VK_ARROW_DOWN:
      if (fullscreenSize.isOpen())
        fullscreenSize.selectNext(*context);
      else if (fullscreenRate.isOpen())
        fullscreenRate.selectNext(*context);
      else {
        if (activeLineSelector == noLineSelection())
          activeLineSelector = 0;
        else
          ++activeLineSelector;
        onLineSelection(activeLineSelector);
      }
      break;
    default: break;
  }
}

void GeneralSettingsPage::padButtonDown(uint32_t virtualKeyCode) {

}


// -- rendering --

bool GeneralSettingsPage::drawBackgrounds(StateBuffers& buffers, int32_t mouseX, int32_t mouseY) {
  auto& renderer = context->renderer();

  // scrollable geometry
  ScissorRectangle scrollableArea(x, title.y() + title.height() + 2, width, (uint32_t)(tooltip.y() - scrollbar.y() - LINE_HEIGHT));
  BufferHandle bufferHandles[] { buffers.regularControl.handle(), scrollPosition.handle() };
  renderer.bindVertexUniforms(0, bufferHandles, sizeof(bufferHandles) / sizeof(*bufferHandles));
  renderer.setScissorRectangle(scrollableArea);
  
  if (activeLineMiddleY != noLineSelection())
    lineHoverArea.draw(renderer);

  displayGroup.drawBackground(*context);
  if (displayMode.drawBackground(*context, mouseX, mouseY, buffers.regularControl, buffers.activeControl, buffers.disabledControl)) {
    if (isFullscreenMode)
      renderer.bindVertexUniforms(0, buffers.regularControl.handlePtr(), 1);
  }
  if (!isFullscreenMode)
    renderer.bindVertexUniforms(0, buffers.disabledControl.handlePtr(), 1);

  bool hasForeground = false;
  if (!fullscreenSize.isOpen()) {
    fullscreenSize.drawBackground(*context);
    hasForeground = true;
  }
  if (!fullscreenRate.isOpen()) {
    fullscreenRate.drawBackground(*context);
    hasForeground = true;
  }
  if (!isFullscreenMode)
    renderer.bindVertexUniforms(0, buffers.regularControl.handlePtr(), 1);

  if (!isWindowMode) {
    renderer.bindVertexUniforms(0, buffers.disabledControl.handlePtr(), 1);
    windowHeight.drawBackground(*context);
    renderer.bindVertexUniforms(0, buffers.regularControl.handlePtr(), 1);
  }
  else windowHeight.drawBackground(*context);

  compatibilityGroup.drawBackground(*context);
  if (subprecisionMode.drawBackground(*context, mouseX, mouseY, buffers.regularControl, buffers.activeControl, buffers.disabledControl))
    renderer.bindVertexUniforms(0, buffers.regularControl.handlePtr(), 1);

  framerateGroup.drawBackground(*context);
  if (framerateLimit.drawBackground(*context, mouseX, mouseY, buffers.regularControl, buffers.activeControl, buffers.disabledControl))
    renderer.bindVertexUniforms(0, buffers.regularControl.handlePtr(), 1);
  if (!isFixedFramerate) {
    renderer.bindVertexUniforms(0, buffers.disabledControl.handlePtr(), 1);
    fixedFramerate.drawBackground(*context);
    renderer.bindVertexUniforms(0, buffers.regularControl.handlePtr(), 1);
  }
  else fixedFramerate.drawBackground(*context);
  
  // fixed geometry
  ScissorRectangle fullWindowArea(0, 0, context->clientWidth(), context->clientHeight());
  bufferHandles[1] = buffers.fixedPosition.handle();
  renderer.bindVertexUniforms(0, bufferHandles, sizeof(bufferHandles) / sizeof(*bufferHandles));
  renderer.setScissorRectangle(fullWindowArea);

  if (scrollbar.drawControl(*context, mouseX, mouseY, buffers.activeControl))
    renderer.bindVertexUniforms(0, buffers.regularControl.handlePtr(), 1);
  tooltipBar.draw(renderer);
  return hasForeground;
  
  //TODO: utiliser activeSelectorIndex à la fois pour mouse hover et pad selection
  // -> on se sert alors de cet index pour savoir quel contrôle dessiner en "hover"

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

void GeneralSettingsPage::drawForegrounds(StateBuffers& buffers, int32_t mouseX, int32_t mouseY) {
  auto& renderer = context->renderer();

  ScissorRectangle fullWindowArea(0, 0, context->clientWidth(), context->clientHeight());
  BufferHandle bufferHandles[] { buffers.regularControl.handle(), scrollPosition.handle() };
  renderer.bindVertexUniforms(0, bufferHandles, sizeof(bufferHandles) / sizeof(*bufferHandles));
  renderer.setScissorRectangle(fullWindowArea);

  if (fullscreenSize.isOpen())
    fullscreenSize.drawBackground(*context);
  if (fullscreenRate.isOpen())
    fullscreenRate.drawBackground(*context);
}

void GeneralSettingsPage::drawIcons(StateBuffers& buffers, int32_t mouseX, int32_t mouseY) {
  auto& renderer = context->renderer();

  ScissorRectangle scrollableArea(x, title.y() + title.height() + 2, width, (uint32_t)(tooltip.y() - scrollbar.y() - LINE_HEIGHT));
  renderer.bindVertexUniforms(1, scrollPosition.handlePtr(), 1);
  renderer.setScissorRectangle(scrollableArea);

  renderer.bindFragmentUniforms(0, buffers.regularIcon.handlePtr(), 1);
  widescreenMode.drawIcon(*context);

  if (!isFramerateLimit)
    renderer.bindFragmentUniforms(0, buffers.disabledIcon.handlePtr(), 1);
  frameSkipping.drawIcon(*context);
  vsync.drawIcon(*context);
}

void GeneralSettingsPage::drawLabels(StateBuffers& buffers, int32_t mouseX, int32_t mouseY) {
  auto& renderer = context->renderer();

  // scrollable geometry
  ScissorRectangle scrollableArea(x, title.y() + title.height() + 2, width, (uint32_t)(tooltip.y() - scrollbar.y() - LINE_HEIGHT));
  renderer.bindVertexUniforms(1, scrollPosition.handlePtr(), 1);
  renderer.setScissorRectangle(scrollableArea);

  renderer.bindFragmentUniforms(0, buffers.fieldsetLabel.handlePtr(), 1);
  displayGroup.drawLabel(*context);
  compatibilityGroup.drawLabel(*context);
  framerateGroup.drawLabel(*context);

  renderer.bindFragmentUniforms(0, buffers.regularLabel.handlePtr(), 1);
  displayMode.drawLabels(*context);
  fullscreenSize.drawLabel(*context);
  fullscreenRate.drawLabel(*context);
  windowHeight.drawLabel(*context);
  windowSize.draw(renderer);
  subprecisionMode.drawLabels(*context);
  widescreenMode.drawLabel(*context);
  framerateLimit.drawLabels(*context);
  frameSkipping.drawLabel(*context);
  fixedFramerate.drawLabel(*context);
  vsync.drawLabel(*context);

  renderer.bindFragmentUniforms(0, buffers.selectedValue.handlePtr(), 1);
  if (!fullscreenSize.isOpen())
    fullscreenSize.drawOptionNames(*context);
  if (!fullscreenRate.isOpen())
    fullscreenRate.drawOptionNames(*context);

  renderer.bindFragmentUniforms(0, buffers.textInput.handlePtr(), 1);
  windowHeight.drawInput(*context);
  fixedFramerate.drawInput(*context);

  // fixed geometry
  ScissorRectangle fullWindowArea(0, 0, context->clientWidth(), context->clientHeight());
  renderer.bindVertexUniforms(1, buffers.fixedPosition.handlePtr(), 1);
  renderer.bindFragmentUniforms(0, buffers.regularLabel.handlePtr(), 1);
  renderer.setScissorRectangle(fullWindowArea);

  title.draw(renderer);
  tooltip.draw(renderer);
}

void GeneralSettingsPage::drawForegroundLabels(StateBuffers& buffers, int32_t mouseX, int32_t mouseY) {
  auto& renderer = context->renderer();

  ScissorRectangle fullWindowArea(0, 0, context->clientWidth(), context->clientHeight());
  renderer.bindVertexUniforms(1, scrollPosition.handlePtr(), 1);
  renderer.setScissorRectangle(fullWindowArea);

  if (fullscreenSize.isOpen())
    fullscreenSize.drawOptionNames(*context);
  if (fullscreenRate.isOpen())
    fullscreenRate.drawOptionNames(*context);
}


// -- events --

void GeneralSettingsPage::onScroll(uint32_t visibleTopY) {
  if (scroll != visibleTopY) {
    scroll = visibleTopY;

    // set scroll buffer
    ScrollUniform scrollLocation{ { 0.f, context->pixelSizeY() * (float)visibleTopY, 0.f, 0.f } };
    scrollPositionStaging.write(&scrollLocation);
    scrollPosition.copy(scrollPositionStaging);
  }
}
void GeneralSettingsPage::onChange(uint32_t id, ComboValue value) {
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
    case FULLSCREEN_RATE_ID:
      break;
    case SUBPRECISION_MODE_ID:
      break;
    case FRAMERATE_LIMIT_ID: {
      isFramerateLimit = (value != 0/*TMP*/);
      isFixedFramerate = (value == 2/*TMP*/);
      break;
    }
    default: assert(false); break;
  }
}

void GeneralSettingsPage::onHover(int32_t lineMiddleY, uint32_t controlWidth, const char32_t* tooltipValue) {
  if (activeLineMiddleY != lineMiddleY) {
    activeLineMiddleY = lineMiddleY;

    // move hover rectangle
    if (lineMiddleY != noLineSelection()) {
      uint32_t hoverAreaWidth = controlWidth + 38;
      std::vector<ControlVertex> hoverAreaVertices = lineHoverArea.relativeVertices();
      hoverAreaVertices[1].position[0] = (float)(hoverAreaWidth - 3);
      hoverAreaVertices[3].position[0] = (float)hoverAreaWidth;
      hoverAreaVertices[5].position[0] = (float)hoverAreaWidth;
      hoverAreaVertices[7].position[0] = (float)(hoverAreaWidth - 3);
      lineHoverArea.update(context->renderer(), std::move(hoverAreaVertices), context->pixelSizeX(), context->pixelSizeY(),
                           displayMode.x() - 10, lineMiddleY - (LINE_HEIGHT >> 1), hoverAreaWidth, LINE_HEIGHT);
    }

    // replace tooltip content
    tooltip = TextMesh(context->renderer(), context->getFont(FontType::labels), tooltipValue ? tooltipValue : U"-",
                       context->pixelSizeX(), context->pixelSizeY(), tooltip.x(), tooltip.y(), TextAlignment::left);
  }
}

void GeneralSettingsPage::onLineSelection(int32_t& lineIndex) {
  // close previous active control
  if (fullscreenSize.isOpen())
    fullscreenSize.close();
  else if (fullscreenRate.isOpen())
    fullscreenRate.close();
  else if (windowHeight.isEditMode()) {
    windowHeight.close();

    char32_t windowWidthBuffer[14];
    memcpy(windowWidthBuffer, U"            x", 14*sizeof(char32_t));
    FormatInteger(GetWindowWidth(windowHeight.valueInteger(), enableWidescreenMode), windowWidthBuffer);
    windowSize = TextMesh(context->renderer(), context->getFont(FontType::inputText), windowWidthBuffer,
                          context->pixelSizeX(), context->pixelSizeY(), windowHeight.controlX() - 8, windowSize.y(), TextAlignment::right);
  }
  else if (fixedFramerate.isEditMode())
    fixedFramerate.close();

  // select control by line index
  switch (lineIndex) {
    case 0: onHover(displayMode.middleY(), LABEL_WIDTH + VALUE_WIDTH + 6, U"Fullscreen / borderless (fullscreen window) / window mode (resizable)"); break;
    case 1: onHover(fullscreenSize.middleY(), LABEL_WIDTH + VALUE_WIDTH + 6, U"Display output resolution (pixels) in fullscreen mode"); break;
    case 2: onHover(fullscreenRate.middleY(), LABEL_WIDTH + VALUE_WIDTH + 6, U"Display output refresh rate (Hz) in fullscreen mode"); break;
    case 3: onHover(windowHeight.middleY(), windowHeight.width(), U"Display output height (pixels) in window mode");
      windowHeight.click(*context);
      break;
    case 4: onHover(subprecisionMode.middleY(), LABEL_WIDTH + VALUE_WIDTH + 6, U"Anti-jitter geometry subprecision (needed if GTE-subprecision enabled in emulator)"); break;
    case 5: onHover(widescreenMode.middleY(), widescreenMode.width(), U"Treat geometry coords as 16:9 (needed if widescreen hack enabled in emulator)"); break;
    case 6: onHover(framerateLimit.middleY(), LABEL_WIDTH + VALUE_WIDTH + 6, U"Enable frame rate limiter (recommended with most emulators)"); break;
    case 7: onHover(fixedFramerate.middleY(), fixedFramerate.width(), U"Allow frame skipping to always keep a constant game speed");
      fixedFramerate.click(*context);
      break;
    case 8: onHover(frameSkipping.middleY(), frameSkipping.width(), U"Custom frame rate limit value (frames per second)"); break;
    case 9: onHover(vsync.middleY(), vsync.width(), U"Fixes screen tearing and reduces power consumption (but may increase input delay)"); break;
    default: break;
  }
}
