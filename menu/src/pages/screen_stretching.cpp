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
#include "display/image_loader.h"
#include "menu/controls/geometry_generator.h"
#include "menu/pages/screen_stretching.h"

using namespace video_api;
using namespace display;
using namespace display::controls;
using namespace menu::controls;
using namespace menu::pages;
using namespace menu;


// -- ratio preview -- ---------------------------------------------------------

#define MAX_RULER_VALUE          8

#define RATIO_PREVIEW_WIDTH      190
#define RATIO_PREVIEW_HEIGHT     107
#define RATIO_PREVIEW_IMG_WIDTH  140
#define RATIO_PREVIEW_IMG_HEIGHT 105
#define RATIO_PREVIEW_OFFSET_Y   ((RATIO_PREVIEW_WIDTH*3/4) - RATIO_PREVIEW_HEIGHT)/2

static void computeImageSize(uint32_t stretchingValue, uint32_t croppingValue,
                             uint32_t frameWidth, uint32_t frameHeight, uint32_t displayWidth,
                             uint32_t& outImageWidth, uint32_t& outImageHeight) {
  const uint32_t maxValue = (stretchingValue >= croppingValue) ? stretchingValue : croppingValue;
  if (maxValue) {
    const double widthFillRatio = (maxValue < MAX_RULER_VALUE) ? ((double)maxValue / (double)MAX_RULER_VALUE) : 1.0;
    const double remainingBlankWidth = (double)(displayWidth - frameWidth) * (1.0 - widthFillRatio);
    outImageWidth = displayWidth - (static_cast<uint32_t>(remainingBlankWidth + 0.5) & ~(uint32_t)0x1u);
    outImageHeight = frameHeight;

    if (croppingValue) {
      const double croppingRatio = ((double)croppingValue / (double)(stretchingValue + croppingValue));
      const double croppingGainedWidth = (double)(outImageWidth - frameWidth) * croppingRatio;
      outImageHeight += (static_cast<uint32_t>(croppingGainedWidth * (double)frameHeight / (double)frameWidth + 0.5) & ~(uint32_t)0x1u);
    }
  }
  else {
    outImageWidth = frameWidth;
    outImageHeight = frameHeight;
  }
}

void ScreenStretching::generatePreview(const float* fieldsetControlRgba, int32_t x, int32_t y) {
  // scaled image
  uint32_t imageWidth, imageHeight;
  computeImageSize(stretchingValue, croppingValue, RATIO_PREVIEW_IMG_WIDTH, RATIO_PREVIEW_IMG_HEIGHT,
                   RATIO_PREVIEW_WIDTH-2, imageWidth, imageHeight);
  ratioPreviewImage = IconMesh(context->renderer(), context->ratioPreviewImage(), context->pixelSizeX(),
                               context->pixelSizeY(), x + ((RATIO_PREVIEW_WIDTH-2 - imageWidth) >> 1) + 1,
                               y + RATIO_PREVIEW_OFFSET_Y + 1 - (int32_t)((imageHeight - RATIO_PREVIEW_IMG_HEIGHT) >> 1),
                               imageWidth, imageHeight);

  std::vector<ControlVertex> vertices(static_cast<size_t>(24u));
  ControlVertex* vertexIt = vertices.data();

  // background + border
  const float borderColor[4]{ fieldsetControlRgba[0]*0.5f, fieldsetControlRgba[1]*0.5f,
                              fieldsetControlRgba[2]*0.5f, fieldsetControlRgba[3] };
  GeometryGenerator::fillRectangleVertices(vertexIt, borderColor, 0.f, (float)RATIO_PREVIEW_WIDTH,
                                           0.f, -(float)RATIO_PREVIEW_HEIGHT);
  vertexIt += 4;
  const float backColor[4]{ fieldsetControlRgba[0]*0.25f, fieldsetControlRgba[1]*0.25f,
                            fieldsetControlRgba[2]*0.25f, fieldsetControlRgba[3] };
  GeometryGenerator::fillRectangleVertices(vertexIt, backColor, 1.f, (float)(RATIO_PREVIEW_WIDTH-1),
                                           -1.f, -(float)(RATIO_PREVIEW_HEIGHT-1));
  vertexIt += 4;

  // truncated image representation
  const float truncBorderColor[4]{ (borderColor[0] + fieldsetControlRgba[0])*0.5f, (borderColor[1] + fieldsetControlRgba[1])*0.5f,
                                   (borderColor[2] + fieldsetControlRgba[2])*0.5f, borderColor[3] };
  const float outOfBoundsBeginX = (float)(ratioPreviewImage.x() - x);
  const float outOfBoundsEndX = outOfBoundsBeginX + (float)imageWidth;
  const uint32_t truncatedHeightHalf = (imageHeight - RATIO_PREVIEW_IMG_HEIGHT) >> 1;
  GeometryGenerator::fillRectangleVertices(vertexIt, truncBorderColor, outOfBoundsBeginX, (float)outOfBoundsEndX, // top
                                           (float)truncatedHeightHalf, 0.f);
  vertexIt += 4;
  GeometryGenerator::fillRectangleVertices(vertexIt, borderColor, outOfBoundsBeginX + 1.f, (float)outOfBoundsEndX - 1.f,
                                           (float)truncatedHeightHalf - 1.f, 0.f);
  vertexIt += 4;
  const float imageBottom = -(float)(RATIO_PREVIEW_HEIGHT + truncatedHeightHalf);
  GeometryGenerator::fillRectangleVertices(vertexIt, truncBorderColor, outOfBoundsBeginX, (float)outOfBoundsEndX, // bottom
                                           -(float)RATIO_PREVIEW_HEIGHT, imageBottom);
  vertexIt += 4;
  GeometryGenerator::fillRectangleVertices(vertexIt, borderColor, outOfBoundsBeginX + 1.f, (float)outOfBoundsEndX - 1.f,
                                           -(float)RATIO_PREVIEW_HEIGHT, imageBottom + 1.f);

  std::vector<uint32_t> indices{ 0,1,2,2,1,3,  4,5,6,6,5,7,  8,9,10,10,9,11,  12,13,14,14,13,15,  16,17,18,18,17,19,  20,21,22,22,21,23 };
  ratioPreviewScreen = ControlMesh(context->renderer(), std::move(vertices), indices, context->pixelSizeX(), context->pixelSizeY(),
                                   x, y + RATIO_PREVIEW_OFFSET_Y, RATIO_PREVIEW_WIDTH, RATIO_PREVIEW_HEIGHT);
  
}

void ScreenStretching::updatePreview() {
  // scaled image
  uint32_t imageWidth, imageHeight;
  computeImageSize(stretchingValue, croppingValue, RATIO_PREVIEW_IMG_WIDTH, RATIO_PREVIEW_IMG_HEIGHT,
                   RATIO_PREVIEW_WIDTH-2, imageWidth, imageHeight);
  ratioPreviewImage.resize(context->renderer(), context->pixelSizeX(), context->pixelSizeY(),
                           ratioPreviewScreen.x() + ((RATIO_PREVIEW_WIDTH-2 - imageWidth) >> 1) + 1,
                           ratioPreviewScreen.y() + 1 - (int32_t)((imageHeight - RATIO_PREVIEW_IMG_HEIGHT) >> 1), imageWidth, imageHeight);

  // truncated image representation
  std::vector<ControlVertex> vertices = ratioPreviewScreen.relativeVertices();
  ControlVertex* vertexIt = vertices.data() + 8;

  const float outOfBoundsBeginX = (float)(ratioPreviewImage.x() - ratioPreviewScreen.x());
  const float outOfBoundsEndX = outOfBoundsBeginX + (float)imageWidth;
  const uint32_t truncatedHeightHalf = (imageHeight - RATIO_PREVIEW_IMG_HEIGHT) >> 1;
  GeometryGenerator::moveRectangleVerticesX(vertexIt, outOfBoundsBeginX, (float)outOfBoundsEndX);
  GeometryGenerator::moveRectangleVerticesY(vertexIt, (float)truncatedHeightHalf, 0.f);
  vertexIt += 4;
  GeometryGenerator::moveRectangleVerticesX(vertexIt, outOfBoundsBeginX + 1.f, (float)outOfBoundsEndX - 1.f);
  GeometryGenerator::moveRectangleVerticesY(vertexIt, (float)truncatedHeightHalf - 1.f, 0.f);
  vertexIt += 4;
  const float imageBottom = -(float)(RATIO_PREVIEW_HEIGHT + truncatedHeightHalf);
  GeometryGenerator::moveRectangleVerticesX(vertexIt, outOfBoundsBeginX, (float)outOfBoundsEndX);
  GeometryGenerator::moveRectangleVerticesY(vertexIt, -(float)RATIO_PREVIEW_HEIGHT, imageBottom);
  vertexIt += 4;
  GeometryGenerator::moveRectangleVerticesX(vertexIt, outOfBoundsBeginX + 1.f, (float)outOfBoundsEndX - 1.f);
  GeometryGenerator::moveRectangleVerticesY(vertexIt, -(float)RATIO_PREVIEW_HEIGHT, imageBottom + 1.f);

  ratioPreviewScreen.update(context->renderer(), std::move(vertices), context->pixelSizeX(), context->pixelSizeY(), 
                            ratioPreviewScreen.x(), ratioPreviewScreen.y(), ratioPreviewScreen.width(), ratioPreviewScreen.height());
}


// -- page -- ------------------------------------------------------------------

#define ASPECT_RATIO_PRESET_ID  1
#define STRETCHING_ID           2
#define CROPPING_ID             3
#define MIRROR_ID               4

#define RATIO_PARAMS_OFFSET     (Control::pageLabelWidth() + 16u)
#define RATIO_PARAMS_MAX_WIDTH  (Control::pageControlWidth() - 14u)
#define RATIO_RULER_OFFSET      69
#define RULER_SIZE              118

enum class AspectRatioPreset : uint32_t {
  keepAspectRatio = 0,
  fillStretched,
  fillCropped,
  stretchedCropped,
  halfStretchedCropped,
  custom
};

void ScreenStretching::init(const ColorTheme& theme, const MessageResources& localizedText, int32_t x, int32_t y, uint32_t width) {
  const MessageResource* textResources = localizedText.screenStretchingMessageArray();
  const uint32_t fieldsetPaddingX = Control::fieldsetMarginX(width);
  const int32_t controlX = x + (int32_t)fieldsetPaddingX + (int32_t)Control::fieldsetContentMarginX(width);
  uint32_t fieldsetWidth = width - (fieldsetPaddingX << 1);
  if (fieldsetWidth > Control::fieldsetMaxWidth())
    fieldsetWidth = Control::fieldsetMaxWidth();

  title = TextMesh(context->renderer(), context->getFont(FontType::titles), GET_UI_MESSAGE(textResources,ScreenStretchingMessages::title),
                   context->pixelSizeX(), context->pixelSizeY(), x + (int32_t)fieldsetPaddingX, y + 24, TextAlignment::left);

  std::vector<ControlRegistration> registry;
  registry.reserve(11);
  int32_t currentLineY = title.y() + (int32_t)title.height() + Control::pageLineHeight();
  auto changeHandler = std::bind(&ScreenStretching::onChange,this,std::placeholders::_1,std::placeholders::_2);

  // --- aspect ratio group ---
  aspectRatioGroup = Fieldset(*context, GET_UI_MESSAGE(textResources,ScreenStretchingMessages::aspectRatioGroup),
                              theme.fieldsetStyle(), theme.fieldsetControlColor(), x + (int32_t)fieldsetPaddingX,
                              currentLineY, fieldsetWidth, Control::fieldsetContentHeight(5) + (Control::pageLineHeight() >> 1)
                                                         + Control::pageLineHeight() - (Control::pageLineHeight() >> 3));
  currentLineY += Control::pageLineHeight() + (Control::pageLineHeight() >> 1) - (Control::pageLineHeight() >> 3);

  // ratio preview
  generatePreview(theme.fieldsetControlColor(), controlX, currentLineY + 1);
  currentLineY += Control::fieldsetContentPaddingY();

  // aspect ratio presets
  const int32_t ratioControlX = controlX + RATIO_PARAMS_OFFSET;
  {
    ComboBoxOption presetOptions[]{
      ComboBoxOption(GET_UI_MESSAGE(textResources,ScreenStretchingMessages::aspectRatio_keep), (ComboValue)AspectRatioPreset::keepAspectRatio),
      ComboBoxOption(GET_UI_MESSAGE(textResources,ScreenStretchingMessages::aspectRatio_stretch), (ComboValue)AspectRatioPreset::fillStretched),
      ComboBoxOption(GET_UI_MESSAGE(textResources,ScreenStretchingMessages::aspectRatio_crop), (ComboValue)AspectRatioPreset::fillCropped),
      ComboBoxOption(GET_UI_MESSAGE(textResources,ScreenStretchingMessages::aspectRatio_both), (ComboValue)AspectRatioPreset::stretchedCropped),
      ComboBoxOption(GET_UI_MESSAGE(textResources,ScreenStretchingMessages::aspectRatio_half), (ComboValue)AspectRatioPreset::halfStretchedCropped),
      ComboBoxOption(GET_UI_MESSAGE(textResources,ScreenStretchingMessages::aspectRatio_custom), (ComboValue)AspectRatioPreset::custom) };
    aspectRatioPreset = ComboBox(*context, nullptr, ratioControlX, currentLineY, 0,
                                 (Control::pageControlWidth() >> 1) + (Control::pageControlWidth() >> 2) - 2u,
                                 ComboBoxStyle::cutCorner, theme.comboBoxColorParams(), ASPECT_RATIO_PRESET_ID, changeHandler,
                                 presetOptions, sizeof(presetOptions)/sizeof(*presetOptions), 0);
    registry.emplace_back(aspectRatioPreset, true, GET_UI_MESSAGE(textResources,ScreenStretchingMessages::aspectRatio_tooltip));
    currentLineY += Control::pageLineHeight() + (Control::pageLineHeight() >> 2);
  }
  // stretching/cropping
  int32_t ratioRulerLabelOffsetY = (context->getFont(FontType::labels).XHeight() - context->getFont(FontType::inputText).XHeight())/2;
  stretching = Ruler(*context, GET_UI_MESSAGE(textResources,ScreenStretchingMessages::keepRatio),
                     GET_UI_MESSAGE(textResources,ScreenStretchingMessages::stretch),
                     FontType::inputText, TextAlignment::right, ratioControlX + RATIO_RULER_OFFSET,
                     currentLineY + ratioRulerLabelOffsetY, 0, RULER_SIZE, theme.rulerColorParams(),
                     STRETCHING_ID, changeHandler, 0, MAX_RULER_VALUE, 1, stretchingValue);
  registry.emplace_back(stretching, true, GET_UI_MESSAGE(textResources,ScreenStretchingMessages::stretching_tooltip));
  currentLineY += Control::pageLineHeight();
  cropping = Ruler(*context, GET_UI_MESSAGE(textResources,ScreenStretchingMessages::entire),
                   GET_UI_MESSAGE(textResources,ScreenStretchingMessages::cropped),
                   FontType::inputText, TextAlignment::right, ratioControlX + RATIO_RULER_OFFSET,
                   currentLineY + ratioRulerLabelOffsetY, 0, RULER_SIZE, theme.rulerColorParams(),
                   CROPPING_ID, changeHandler, 0, MAX_RULER_VALUE, 1, croppingValue);
  registry.emplace_back(cropping, true, GET_UI_MESSAGE(textResources,ScreenStretchingMessages::cropping_tooltip));
  currentLineY += Control::pageLineHeight() + (Control::pageLineHeight() >> 2);
  // pixel ratio
  {
    ComboBoxOption pixelRatioOptions[]{ ComboBoxOption(GET_UI_MESSAGE(textResources,ScreenStretchingMessages::pixelRatio_square), 0/*TMP*/),
                                        ComboBoxOption(GET_UI_MESSAGE(textResources,ScreenStretchingMessages::pixelRatio_crt), 1/*TMP*/) };
    pixelRatio = Slider(*context, nullptr, ratioControlX, currentLineY, 0, RATIO_PARAMS_MAX_WIDTH, theme.sliderArrowColor(), 0,
                        nullptr, pixelRatioOptions, sizeof(pixelRatioOptions)/sizeof(*pixelRatioOptions), 0);
    registry.emplace_back(pixelRatio, true, GET_UI_MESSAGE(textResources,ScreenStretchingMessages::pixelRatio_tooltip));
    currentLineY += Control::pageLineHeight();
  }
  // mirror
  {
    ComboBoxOption mirrorOptions[]{ ComboBoxOption(GET_UI_MESSAGE(textResources,ScreenStretchingMessages::mirror_none), 0/*TMP*/),
                                    ComboBoxOption(GET_UI_MESSAGE(textResources,ScreenStretchingMessages::mirror_mirrorX), 1/*TMP*/) };
    mirror = Slider(*context, nullptr, ratioControlX, currentLineY, 0, RATIO_PARAMS_MAX_WIDTH, theme.sliderArrowColor(),
                    MIRROR_ID, changeHandler, mirrorOptions, sizeof(mirrorOptions)/sizeof(*mirrorOptions), 0);
    registry.emplace_back(mirror, true, GET_UI_MESSAGE(textResources,ScreenStretchingMessages::mirror_tooltip));
    currentLineY += Control::pageLineHeight() + Control::fieldsetContentBottomMargin() + (Control::pageLineHeight() >> 1);
  }

  // --- display adjustments group ---
  displayAdjustGroup = Fieldset(*context, GET_UI_MESSAGE(textResources,ScreenStretchingMessages::displayAdjustmentsGroup),
                                theme.fieldsetStyle(), theme.fieldsetControlColor(), x + (int32_t)fieldsetPaddingX,
                                currentLineY, fieldsetWidth, Control::fieldsetContentHeight(5) + 1u);
  currentLineY += Control::pageLineHeight() + Control::fieldsetContentPaddingY() + 1;

  // black borders
  blackBordersX = TextBox(*context, GET_UI_MESSAGE(textResources,ScreenStretchingMessages::blackBorders), nullptr, controlX,
                          currentLineY, Control::pageLabelWidth(), (Control::pageControlWidth() >> 2) - 1u,
                          theme.textBoxControlColor(), 0, nullptr, (uint32_t)0, 3u);
  registry.emplace_back(blackBordersX, true, GET_UI_MESSAGE(textResources,ScreenStretchingMessages::blackBorders_tooltip));
  blackBordersY = TextBox(*context, nullptr, U"px", blackBordersX.rightX() + 2, currentLineY, 0,
                          (Control::pageControlWidth() >> 2) - 1u, theme.textBoxControlColor(), 0, nullptr, (uint32_t)0, 3u);
  registry.emplace_back(blackBordersY, true, GET_UI_MESSAGE(textResources,ScreenStretchingMessages::blackBorders_tooltip));
  currentLineY += Control::pageLineHeight();

  // screen curvature
  screenCurvature = Ruler(*context, GET_UI_MESSAGE(textResources,ScreenStretchingMessages::screenCurvature),
                          GET_UI_MESSAGE(textResources,ScreenStretchingMessages::maxCurved), FontType::labels,
                          TextAlignment::left, controlX, currentLineY, Control::pageLabelWidth(), RULER_SIZE,
                          theme.rulerColorParams(), 0, nullptr, 0, MAX_RULER_VALUE, 1, screenCurvatureValue);
  registry.emplace_back(screenCurvature, true, GET_UI_MESSAGE(textResources,ScreenStretchingMessages::screenCurvature_tooltip));
  currentLineY += Control::pageLineHeight();

  // overscan/center
  showOverscanArea = CheckBox(*context, GET_UI_MESSAGE(textResources,ScreenStretchingMessages::showOverscanArea), controlX,
                              currentLineY, Control::pageLabelWidth(), 0, nullptr, isOverscanVisible);
  isOverscanVisible = false;
  registry.emplace_back(showOverscanArea, true, GET_UI_MESSAGE(textResources,ScreenStretchingMessages::showOverscanArea_tooltip));
  currentLineY += Control::pageLineHeight();

  centerX = CheckBox(*context, GET_UI_MESSAGE(textResources,ScreenStretchingMessages::centerX), controlX,
                     currentLineY, Control::pageLabelWidth(), 0, nullptr, isCenteredX);
  isCenteredX = false;
  registry.emplace_back(centerX, true, GET_UI_MESSAGE(textResources,ScreenStretchingMessages::centerX_tooltip));
  currentLineY += Control::pageLineHeight();

  centerY = CheckBox(*context, GET_UI_MESSAGE(textResources,ScreenStretchingMessages::centerY), controlX,
                     currentLineY, Control::pageLabelWidth(), 0, nullptr, isCenteredY);
  isCenteredY = false;
  registry.emplace_back(centerY, true, GET_UI_MESSAGE(textResources,ScreenStretchingMessages::centerY_tooltip));
  currentLineY += Control::pageLineHeight();// + Control::fieldsetContentBottomMargin();

  // --- control registry ---
  if (currentLineY > y + (int32_t)contentHeight())
    Page::moveScrollbarThumb(currentLineY);
  registerControls(std::move(registry));
}

ScreenStretching::~ScreenStretching() noexcept {
  title.release();

  aspectRatioGroup.release();
  ratioPreviewScreen.release();
  ratioPreviewImage.release();
  aspectRatioPreset.release();
  stretching.release();
  cropping.release();
  pixelRatio.release();
  mirror.release();

  displayAdjustGroup.release();
  showOverscanArea.release();
  centerX.release();
  centerY.release();
  blackBordersX.release();
  blackBordersY.release();
  screenCurvature.release();
}


// -- window events -- ---------------------------------------------------------

void ScreenStretching::move(int32_t x, int32_t y, uint32_t width, uint32_t height) {
  Page::moveBase(x, y, width, height);
  const uint32_t fieldsetPaddingX = Control::fieldsetMarginX(width);
  const int32_t controlX = x + (int32_t)fieldsetPaddingX + (int32_t)Control::fieldsetContentMarginX(width);
  uint32_t fieldsetWidth = width - (fieldsetPaddingX << 1);
  if (fieldsetWidth > Control::fieldsetMaxWidth())
    fieldsetWidth = Control::fieldsetMaxWidth();

  title.move(context->renderer(), context->pixelSizeX(), context->pixelSizeY(), x + (int32_t)fieldsetPaddingX, y + 24);

  // aspect ratio group
  int32_t currentLineY = title.y() + (int32_t)title.height() + Control::pageLineHeight();
  aspectRatioGroup.move(*context, x + (int32_t)fieldsetPaddingX, currentLineY, fieldsetWidth,
                        Control::fieldsetContentHeight(5) + (Control::pageLineHeight() >> 1)
                      + Control::pageLineHeight() - (Control::pageLineHeight() >> 3));
  currentLineY += Control::pageLineHeight() + (Control::pageLineHeight() >> 1) - (Control::pageLineHeight() >> 3);

  // ratio preview
  ratioPreviewImage.move(context->renderer(), context->pixelSizeX(), context->pixelSizeY(),
                         controlX + ratioPreviewImage.x() - ratioPreviewScreen.x(),
                         currentLineY + 1 + RATIO_PREVIEW_OFFSET_Y + ratioPreviewImage.y() - ratioPreviewScreen.y());
  ratioPreviewScreen.move(context->renderer(), context->pixelSizeX(), context->pixelSizeY(), controlX, currentLineY + 1 + RATIO_PREVIEW_OFFSET_Y);
  currentLineY += Control::fieldsetContentPaddingY();

  const int32_t ratioControlX = controlX + RATIO_PARAMS_OFFSET;
  aspectRatioPreset.move(*context, ratioControlX, currentLineY);
  currentLineY += Control::pageLineHeight() + (Control::pageLineHeight() >> 2);
  stretching.move(*context, ratioControlX + RATIO_RULER_OFFSET, currentLineY, TextAlignment::right);
  currentLineY += Control::pageLineHeight();
  cropping.move(*context, ratioControlX + RATIO_RULER_OFFSET, currentLineY, TextAlignment::right);
  currentLineY += Control::pageLineHeight() + (Control::pageLineHeight() >> 2);
  pixelRatio.move(*context, ratioControlX, currentLineY);
  currentLineY += Control::pageLineHeight();
  mirror.move(*context, ratioControlX, currentLineY);
  currentLineY += Control::pageLineHeight() + Control::fieldsetContentBottomMargin() + (Control::pageLineHeight() >> 1);

  // display adjustements group
  displayAdjustGroup.move(*context, x + (int32_t)fieldsetPaddingX, currentLineY, fieldsetWidth, Control::fieldsetContentHeight(5) + 1u);
  currentLineY += Control::pageLineHeight() + Control::fieldsetContentPaddingY() + 1;

  blackBordersX.move(*context, controlX, currentLineY);
  blackBordersY.move(*context, blackBordersX.rightX() + 2, currentLineY);
  currentLineY += Control::pageLineHeight();
  screenCurvature.move(*context, controlX, currentLineY, TextAlignment::left);
  currentLineY += Control::pageLineHeight();
  showOverscanArea.move(*context, controlX, currentLineY);
  currentLineY += Control::pageLineHeight();
  centerX.move(*context, controlX, currentLineY);
  currentLineY += Control::pageLineHeight();
  centerY.move(*context, controlX, currentLineY);
  currentLineY += Control::pageLineHeight();// + Control::fieldsetContentBottomMargin();

  Page::moveScrollbarThumb(currentLineY); // required after a move
}

void ScreenStretching::onChange(uint32_t id, uint32_t value) {
  bool isRatioPreviewUpdated = false;
  switch (id) {
    case ASPECT_RATIO_PRESET_ID: {
      isRatioPreviewUpdated = true;
      switch ((AspectRatioPreset)value) {
        case AspectRatioPreset::keepAspectRatio:
          stretching.setSelectedIndex(*context, 0, false);
          cropping.setSelectedIndex(*context, 0, false);
          break;
        case AspectRatioPreset::fillStretched:
          stretching.setSelectedIndex(*context, MAX_RULER_VALUE, false);
          cropping.setSelectedIndex(*context, 0, false);
          break;
        case AspectRatioPreset::fillCropped:
          stretching.setSelectedIndex(*context, 0, false);
          cropping.setSelectedIndex(*context, MAX_RULER_VALUE, false);
          break;
        case AspectRatioPreset::stretchedCropped:
          stretching.setSelectedIndex(*context, MAX_RULER_VALUE, false);
          cropping.setSelectedIndex(*context, MAX_RULER_VALUE, false);
          break;
        case AspectRatioPreset::halfStretchedCropped:
          stretching.setSelectedIndex(*context, MAX_RULER_VALUE >> 1, false);
          cropping.setSelectedIndex(*context, MAX_RULER_VALUE >> 1, false);
          break;
        case AspectRatioPreset::custom:
        default: break;
      }
      break;
    }
    case STRETCHING_ID:
    case CROPPING_ID: {
      isRatioPreviewUpdated = true;
      if (stretchingValue == 0 && croppingValue == 0)
        aspectRatioPreset.setSelectedIndex(*context, (uint32_t)AspectRatioPreset::keepAspectRatio, false);
      else if (stretchingValue == MAX_RULER_VALUE && croppingValue == 0)
        aspectRatioPreset.setSelectedIndex(*context, (uint32_t)AspectRatioPreset::fillStretched, false);
      else if (stretchingValue == 0 && croppingValue == MAX_RULER_VALUE)
        aspectRatioPreset.setSelectedIndex(*context, (uint32_t)AspectRatioPreset::fillCropped, false);
      else if (stretchingValue == MAX_RULER_VALUE && croppingValue == MAX_RULER_VALUE)
        aspectRatioPreset.setSelectedIndex(*context, (uint32_t)AspectRatioPreset::stretchedCropped, false);
      else if (stretchingValue == (MAX_RULER_VALUE >> 1) && croppingValue == (MAX_RULER_VALUE >> 1))
        aspectRatioPreset.setSelectedIndex(*context, (uint32_t)AspectRatioPreset::halfStretchedCropped, false);
      else if (aspectRatioPreset.getSelectedIndex() != (uint32_t)AspectRatioPreset::custom)
        aspectRatioPreset.setSelectedIndex(*context, (uint32_t)AspectRatioPreset::custom, false);
      break;
    }
    case MIRROR_ID: {
      ratioPreviewImage.invertX(context->renderer(), context->pixelSizeX(), context->pixelSizeY());
      break;
    }
    default: assert(false); break;
  }

  if (isRatioPreviewUpdated)
    updatePreview();
}


// -- rendering -- -------------------------------------------------------------

void ScreenStretching::drawIcons() {
  // scrollable geometry
  buffers->bindScrollLocationBuffer(context->renderer(), ScissorRectangle(x(), y(), width(), contentHeight()));

  auto* hoverControl = getActiveControl();
  showOverscanArea.drawIcon(*context, *buffers, (hoverControl == &showOverscanArea));
  centerX.drawIcon(*context, *buffers, (hoverControl == &centerX));
  centerY.drawIcon(*context, *buffers, (hoverControl == &centerY));

  buffers->bindScrollLocationBuffer(context->renderer(), ScissorRectangle(ratioPreviewScreen.x()+1, ratioPreviewScreen.y()+1 - scrollLevel(),
                                                                          ratioPreviewScreen.width()-2, ratioPreviewScreen.height()-2 - scrollLevel()));
  buffers->bindIconBuffer(context->renderer(), ControlBufferType::regular);
  ratioPreviewImage.draw(context->renderer());
}

bool ScreenStretching::drawPageBackgrounds(int32_t mouseX, int32_t) {
  // scrollable geometry
  if (buffers->isFixedLocationBuffer())
    buffers->bindScrollLocationBuffer(context->renderer(), ScissorRectangle(x(), y(), width(), contentHeight()));

  auto* hoverControl = getActiveControl();
  aspectRatioGroup.drawBackground(*context, *buffers);
  displayAdjustGroup.drawBackground(*context, *buffers);

  buffers->bindControlBuffer(context->renderer(), ControlBufferType::regular);
  ratioPreviewScreen.draw(context->renderer());

  aspectRatioPreset.drawBackground(*context, *buffers, (hoverControl == &aspectRatioPreset));
  stretching.drawBackground(*context, *buffers);
  cropping.drawBackground(*context, *buffers);
  screenCurvature.drawBackground(*context, *buffers);
  pixelRatio.drawBackground(*context, mouseX, *buffers, (hoverControl == &pixelRatio));
  mirror.drawBackground(*context, mouseX, *buffers, (hoverControl == &mirror));

  blackBordersX.drawBackground(*context, *buffers);
  blackBordersY.drawBackground(*context, *buffers);
  return false;
}

void ScreenStretching::drawPageLabels() {
  // scrollable geometry
  auto& renderer = context->renderer();
  if (buffers->isFixedLocationBuffer())
    buffers->bindScrollLocationBuffer(renderer, ScissorRectangle(x(), y(), width(), contentHeight()));

  buffers->bindLabelBuffer(renderer, LabelBufferType::title);
  title.draw(renderer);

  auto* hoverControl = getActiveControl();
  aspectRatioGroup.drawLabel(*context, *buffers);
  displayAdjustGroup.drawLabel(*context, *buffers);

  aspectRatioPreset.drawLabels(*context, *buffers, (hoverControl == &aspectRatioPreset));
  stretching.drawLabels(*context, *buffers, (hoverControl == &stretching));
  cropping.drawLabels(*context, *buffers, (hoverControl == &cropping));
  screenCurvature.drawLabels(*context, *buffers, (hoverControl == &screenCurvature));
  pixelRatio.drawLabels(*context, *buffers, (hoverControl == &pixelRatio));
  mirror.drawLabels(*context, *buffers, (hoverControl == &mirror));

  showOverscanArea.drawLabel(*context, *buffers, (hoverControl == &showOverscanArea));
  centerX.drawLabel(*context, *buffers, (hoverControl == &centerX));
  centerY.drawLabel(*context, *buffers, (hoverControl == &centerY));

  blackBordersX.drawLabels(*context, *buffers, (hoverControl == &blackBordersX));
  blackBordersY.drawLabels(*context, *buffers, (hoverControl == &blackBordersY));
}

void ScreenStretching::drawForegrounds() {
  auto& renderer = context->renderer();

  ScissorRectangle fullWindowArea(0, 0, context->clientWidth(), context->clientHeight());
  buffers->bindScrollLocationBuffer(renderer, fullWindowArea); // visible outside of scroll area -> full window

  aspectRatioPreset.drawDropdown(*context, *buffers);
}

void ScreenStretching::drawForegroundLabels() {
  ScissorRectangle fullWindowArea(0, 0, context->clientWidth(), context->clientHeight());
  buffers->bindScrollLocationBuffer(context->renderer(), fullWindowArea); // visible outside of scroll area -> full window

  aspectRatioPreset.drawOptions(*context, *buffers);
}