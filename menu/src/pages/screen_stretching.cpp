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
#include "menu/pages/page_content_builder.h"
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
  custom,
  COUNT
};

void ScreenStretching::init(const ColorTheme& theme, const MessageResources& localizedText, int32_t x, int32_t y, uint32_t width) {
  PageContentBuilder builder(*context, theme, x, y, width, 11, nullptr,
                             std::bind(&ScreenStretching::onValueChange,this,std::placeholders::_1,std::placeholders::_2));

  builder.addTitle(localizedText.getMessage(ScreenStretchingMessages::title), title);

  // aspect ratio group
  builder.addFieldset(localizedText.getMessage(ScreenStretchingMessages::aspectRatioGroup), 5,
                      Control::pageLineHeight() + (Control::pageLineHeight() >> 3) + Control::fieldsetContentPaddingTop(),
                      aspectRatioGroup);
  builder.addLineOffset((int32_t)(Control::pageLineHeight() >> 3));

  generatePreview(theme.fieldsetControlColor(), builder.linePositionX(), builder.linePositionY() + 2);
  builder.addLineOffset((int32_t)Control::fieldsetContentPaddingTop());

  // -> aspect ratio presets
  ComboBoxOption ratioPresetOptions[(size_t)AspectRatioPreset::COUNT];
  for (uint32_t i = 0; i < (uint32_t)AspectRatioPreset::COUNT; ++i) {
    auto message = static_cast<ScreenStretchingMessages>((uint32_t)ScreenStretchingMessages::aspectRatio_keep + i);
    ratioPresetOptions[i] = ComboBoxOption(localizedText.getMessage(message), (ComboValue)i);
  }
  const int32_t ratioControlX = builder.linePositionX() + RATIO_PARAMS_OFFSET;
  builder.addComboBox(ASPECT_RATIO_PRESET_ID, localizedText.getMessage(ScreenStretchingMessages::aspectRatio_tooltip),
                      ratioControlX, (Control::pageControlWidth() >> 1) + (Control::pageControlWidth() >> 2) - 2u,
                      ratioPresetOptions, sizeof(ratioPresetOptions)/sizeof(*ratioPresetOptions), 0, aspectRatioPreset);
  builder.addLineOffset((int32_t)(Control::pageLineHeight() >> 2));

  // -> stretching/cropping
  builder.addRuler(STRETCHING_ID, localizedText.getMessage(ScreenStretchingMessages::keepRatio),
                   localizedText.getMessage(ScreenStretchingMessages::stretch),
                   localizedText.getMessage(ScreenStretchingMessages::stretching_tooltip),
                   ratioControlX + RATIO_RULER_OFFSET, RULER_SIZE, MAX_RULER_VALUE, stretchingValue, stretching);
  builder.addRuler(CROPPING_ID, localizedText.getMessage(ScreenStretchingMessages::entire),
                   localizedText.getMessage(ScreenStretchingMessages::cropped),
                   localizedText.getMessage(ScreenStretchingMessages::cropping_tooltip),
                   ratioControlX + RATIO_RULER_OFFSET, RULER_SIZE, MAX_RULER_VALUE, croppingValue, cropping);
  builder.addLineOffset((int32_t)(Control::pageLineHeight() >> 2));

  // -> pixel ratio / mirroring
  ComboBoxOption pixelRatioOptions[]{ ComboBoxOption(localizedText.getMessage(ScreenStretchingMessages::pixelRatio_square), 0/*TMP*/),
                                      ComboBoxOption(localizedText.getMessage(ScreenStretchingMessages::pixelRatio_crt), 1/*TMP*/) };
  builder.addSlider(0, localizedText.getMessage(ScreenStretchingMessages::pixelRatio_tooltip), ratioControlX, RATIO_PARAMS_MAX_WIDTH,
                    pixelRatioOptions, sizeof(pixelRatioOptions)/sizeof(*pixelRatioOptions), 0, pixelRatio);

  ComboBoxOption mirrorOptions[]{ ComboBoxOption(localizedText.getMessage(ScreenStretchingMessages::mirror_none), 0/*TMP*/),
                                  ComboBoxOption(localizedText.getMessage(ScreenStretchingMessages::mirror_mirrorX), 1/*TMP*/) };
  builder.addSlider(MIRROR_ID, localizedText.getMessage(ScreenStretchingMessages::mirror_tooltip), ratioControlX, RATIO_PARAMS_MAX_WIDTH,
                    mirrorOptions, sizeof(mirrorOptions)/sizeof(*mirrorOptions), 0, mirror);
  builder.addLineOffset((int32_t)Control::pageLineHeight() - (int32_t)((Control::pageLineHeight() >> 2) << 1));

  // display adjustments group
  builder.addFieldset(localizedText.getMessage(ScreenStretchingMessages::displayAdjustmentsGroup), 5, 1, displayAdjustGroup);
  screenCurvatureValue = 0;
  isOverscanVisible = isCenteredX = isCenteredY = false;

  builder.addDoubleTextBox(0, 0, localizedText.getMessage(ScreenStretchingMessages::blackBorders), u"px",
                           localizedText.getMessage(ScreenStretchingMessages::blackBorders_tooltip),
                           0, 0, 3u, blackBordersX, blackBordersY);
  builder.addRuler(0, localizedText.getMessage(ScreenStretchingMessages::screenCurvature), nullptr,
                   localizedText.getMessage(ScreenStretchingMessages::screenCurvature_tooltip),
                   RULER_SIZE, MAX_RULER_VALUE, screenCurvatureValue, screenCurvature);
  builder.addCheckBox(0, localizedText.getMessage(ScreenStretchingMessages::showOverscanArea),
                      localizedText.getMessage(ScreenStretchingMessages::showOverscanArea_tooltip),
                      isOverscanVisible, showOverscanArea);
  builder.addCheckBox(0, localizedText.getMessage(ScreenStretchingMessages::centerX),
                      localizedText.getMessage(ScreenStretchingMessages::centerX_tooltip),
                      isCenteredX, centerX);
  builder.addCheckBox(0, localizedText.getMessage(ScreenStretchingMessages::centerY),
                      localizedText.getMessage(ScreenStretchingMessages::centerY_tooltip),
                      isCenteredY, centerY);

  // --- control registry ---
  Page::moveScrollbarThumb(builder.linePositionY());
  registerControls(std::move(builder.controlRegistry()));
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
  blackBordersX.release();
  blackBordersY.release();
  screenCurvature.release();
  showOverscanArea.release();
  centerX.release();
  centerY.release();
}


// -- window events -- ---------------------------------------------------------

void ScreenStretching::move(int32_t x, int32_t y, uint32_t width, uint32_t height) {
  Page::moveBase(x, y, width, height);
  PageContentMover mover(*context, x, y, width);
  const int32_t offsetX = mover.linePositionX() - blackBordersX.x();

  mover.moveTitle(title);

  // aspect ratio group
  mover.moveFieldset(aspectRatioGroup);
  mover.addLineOffset((int32_t)(Control::pageLineHeight() >> 3));

  ratioPreviewImage.move(context->renderer(), context->pixelSizeX(), context->pixelSizeY(),
                         mover.linePositionX() + ratioPreviewImage.x() - ratioPreviewScreen.x(),
                         mover.linePositionY() + 2 + RATIO_PREVIEW_OFFSET_Y + ratioPreviewImage.y() - ratioPreviewScreen.y());
  ratioPreviewScreen.move(context->renderer(), context->pixelSizeX(), context->pixelSizeY(),
                          mover.linePositionX(), mover.linePositionY() + 2 + RATIO_PREVIEW_OFFSET_Y);
  mover.addLineOffset((int32_t)Control::fieldsetContentPaddingTop());

  const int32_t ratioControlX = mover.linePositionX() + RATIO_PARAMS_OFFSET;
  mover.moveComboBox(ratioControlX, aspectRatioPreset);
  mover.addLineOffset((int32_t)(Control::pageLineHeight() >> 2));

  mover.moveRuler(ratioControlX + RATIO_RULER_OFFSET, stretching);
  mover.moveRuler(ratioControlX + RATIO_RULER_OFFSET, cropping);
  mover.addLineOffset((int32_t)(Control::pageLineHeight() >> 2));

  mover.moveSlider(ratioControlX, pixelRatio);
  mover.moveSlider(ratioControlX, mirror);
  mover.addLineOffset((int32_t)Control::pageLineHeight() - (int32_t)((Control::pageLineHeight() >> 2) << 1));

  // display adjustements group
  mover.moveFieldset(displayAdjustGroup);

  mover.moveDoubleTextBox(blackBordersX, blackBordersY);
  mover.moveRuler(screenCurvature);
  mover.moveCheckBox(showOverscanArea);
  mover.moveCheckBox(centerX);
  mover.moveCheckBox(centerY);

  Page::moveScrollbarThumb(mover.linePositionY()); // required after a move
  Page::moveRegisteredControls(offsetX);
}

void ScreenStretching::onValueChange(uint32_t id, uint32_t value) {
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
      ratioPreviewImage.invertX(context->renderer());
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

void ScreenStretching::drawPageBackgrounds(int32_t mouseX, int32_t mouseY) {
  // scrollable geometry
  if (buffers->isFixedLocationBuffer())
    buffers->bindScrollLocationBuffer(context->renderer(), ScissorRectangle(x(), y(), width(), contentHeight()));

  aspectRatioGroup.drawBackground(*context, *buffers);
  displayAdjustGroup.drawBackground(*context, *buffers);

  buffers->bindControlBuffer(context->renderer(), ControlBufferType::regular);
  ratioPreviewScreen.draw(context->renderer());

  auto* hoverControl = getActiveControl();
  aspectRatioPreset.drawBackground(*context, *buffers, (hoverControl == &aspectRatioPreset));
  stretching.drawBackground(*context, *buffers);
  cropping.drawBackground(*context, *buffers);
  screenCurvature.drawBackground(*context, *buffers);
  pixelRatio.drawBackground(*context, mouseX, *buffers, (hoverControl == &pixelRatio));
  mirror.drawBackground(*context, mouseX, *buffers, (hoverControl == &mirror));

  blackBordersX.drawBackground(*context, mouseX, mouseY, *buffers, (hoverControl == &blackBordersX));
  blackBordersY.drawBackground(*context, mouseX, mouseY, *buffers, (hoverControl == &blackBordersY));
}

void ScreenStretching::drawPageLabels() {
  // scrollable geometry
  auto& renderer = context->renderer();
  if (buffers->isFixedLocationBuffer())
    buffers->bindScrollLocationBuffer(renderer, ScissorRectangle(x(), y(), width(), contentHeight()));

  buffers->bindLabelBuffer(renderer, LabelBufferType::title);
  title.draw(renderer);

  aspectRatioGroup.drawLabel(*context, *buffers);
  displayAdjustGroup.drawLabel(*context, *buffers);

  auto* hoverControl = getActiveControl();
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
