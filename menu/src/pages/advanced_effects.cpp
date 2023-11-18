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
#include <cstring>
#include <cmath>
#include "menu/controls/geometry_generator.h"
#include "menu/pages/page_content_builder.h"
#include "menu/pages/advanced_effects.h"

using namespace video_api;
using namespace display;
using namespace display::controls;
using namespace menu::controls;
using namespace menu::pages;
using namespace menu;


// -- helpers -- ---------------------------------------------------------------

#define GAMMA_PREVIEW_WIDTH  Control::pageControlWidth()
#define GAMMA_PREVIEW_HEIGHT (Control::pageLineHeight() + (Control::pageLineHeight() >> 2))
#define GAMMA_SAMPLE_COLOR_COMP 12.0/255.0

void AdvancedEffects::generateGammaPreview(const float* fieldsetControlRgba, int32_t x, int32_t y, double gamma) {
  std::vector<ControlVertex> vertices(static_cast<size_t>(20u));
  ControlVertex* vertexIt = vertices.data();

  // background + border
  const float borderColor[4]{ fieldsetControlRgba[0]*0.5f, fieldsetControlRgba[1]*0.5f,
                              fieldsetControlRgba[2]*0.5f, fieldsetControlRgba[3] };
  GeometryGenerator::fillRectangleVertices(vertexIt, borderColor, 0.f, (float)GAMMA_PREVIEW_WIDTH,
                                           0.f, -(float)GAMMA_PREVIEW_HEIGHT);
  vertexIt += 4;
  const float backColor[4]{ 0.f, 0.f, 0.f, 1.f };
  GeometryGenerator::fillRectangleVertices(vertexIt, backColor, 1.f, (float)(GAMMA_PREVIEW_WIDTH-1),
                                           -1.f, -(float)(GAMMA_PREVIEW_HEIGHT-1));
  vertexIt += 4;

  // diamond shapes
  const float linearColorComp = (float)pow(GAMMA_SAMPLE_COLOR_COMP, gamma); // gamma corrected to linear
  float shapeColor[4]{ linearColorComp, linearColorComp, linearColorComp, 1.f };

  GeometryGenerator::fillDiamondVertices(vertexIt, shapeColor,
                        (float)((GAMMA_PREVIEW_WIDTH >> 1) - (GAMMA_PREVIEW_HEIGHT >> 3) - (GAMMA_PREVIEW_WIDTH >> 2)),
                        (float)((GAMMA_PREVIEW_WIDTH >> 1) + (GAMMA_PREVIEW_HEIGHT >> 3) - (GAMMA_PREVIEW_WIDTH >> 2)),
                        -(float)((GAMMA_PREVIEW_HEIGHT >> 1) - (GAMMA_PREVIEW_HEIGHT >> 3)),
                        -(float)((GAMMA_PREVIEW_HEIGHT >> 1) + (GAMMA_PREVIEW_HEIGHT >> 3)));
  vertexIt += 4;
  GeometryGenerator::fillDiamondVertices(vertexIt, shapeColor,
                        (float)((GAMMA_PREVIEW_WIDTH >> 1) - (GAMMA_PREVIEW_HEIGHT >> 2)),
                        (float)((GAMMA_PREVIEW_WIDTH >> 1) + (GAMMA_PREVIEW_HEIGHT >> 2)),
                        -(float)((GAMMA_PREVIEW_HEIGHT >> 1) - (GAMMA_PREVIEW_HEIGHT >> 2)),
                        -(float)((GAMMA_PREVIEW_HEIGHT >> 1) + (GAMMA_PREVIEW_HEIGHT >> 2)));
  vertexIt += 4;
  GeometryGenerator::fillDiamondVertices(vertexIt, shapeColor,
                        (float)((GAMMA_PREVIEW_WIDTH >> 1) - (GAMMA_PREVIEW_HEIGHT >> 3) + (GAMMA_PREVIEW_WIDTH >> 2)),
                        (float)((GAMMA_PREVIEW_WIDTH >> 1) + (GAMMA_PREVIEW_HEIGHT >> 3) + (GAMMA_PREVIEW_WIDTH >> 2)),
                        -(float)((GAMMA_PREVIEW_HEIGHT >> 1) - (GAMMA_PREVIEW_HEIGHT >> 3)),
                        -(float)((GAMMA_PREVIEW_HEIGHT >> 1) + (GAMMA_PREVIEW_HEIGHT >> 3)));

  std::vector<uint32_t> indices{ 0,1,2,2,1,3,  4,5,6,6,5,7,  8,9,10,10,9,11,  12,13,14,14,13,15,  16,17,18,18,17,19 };
  gammaPreview = ControlMesh(context->renderer(), std::move(vertices), indices, context->pixelSizeX(), context->pixelSizeY(),
                             x, y, GAMMA_PREVIEW_WIDTH, Control::pageLineHeight());
}

void AdvancedEffects::updateGammaPreview(double gamma) {
  const float linearColorComp = (float)pow(GAMMA_SAMPLE_COLOR_COMP, gamma); // gamma corrected to linear
  float shapeColor[4]{ linearColorComp, linearColorComp, linearColorComp, 1.f };

  std::vector<ControlVertex> vertices = gammaPreview.relativeVertices();
  const auto* endIt = vertices.data() + (intptr_t)vertices.size();
  for (ControlVertex* it = &vertices[8]; it < endIt; ++it)
    memcpy(it->color, shapeColor, sizeof(float)*4u);

  gammaPreview.update(context->renderer(), std::move(vertices), context->pixelSizeX(), context->pixelSizeY(),
                      gammaPreview.x(), gammaPreview.y(), gammaPreview.width(), gammaPreview.height());
}


// -- page -- ------------------------------------------------------------------

#define GAMMA_CORRECTION_ID 1

void AdvancedEffects::init(const ColorTheme& theme, const MessageResources& localizedText, int32_t x, int32_t y, uint32_t width) {
  PageContentBuilder builder(*context, theme, x, y, width, 3, nullptr,
                             std::bind(&AdvancedEffects::onValueChange,this,std::placeholders::_1,std::placeholders::_2));

  builder.addTitle(localizedText.getMessage(AdvancedEffectsMessages::title), title);

  constexpr const uint32_t controlWidth = Control::pageControlWidth() - (Control::pageControlWidth() >> 2);
  constexpr const uint32_t gammaRulerOffsetX = Control::pageLabelWidth() - (Control::pageControlWidth() >> 2);
  constexpr const uint32_t gammaPreviewLineHeight = Control::pageLineHeight() + (Control::pageLineHeight() >> 1);

  // gamma group
  builder.addFieldset(localizedText.getMessage(AdvancedEffectsMessages::gammaGroup), 1, gammaPreviewLineHeight, gammaGroup);

  generateGammaPreview(theme.fieldsetControlColor(),
                       builder.linePositionX() + (int32_t)gammaRulerOffsetX + (int32_t)Control::labelMargin(),
                       builder.linePositionY() - (int32_t)(Control::pageLineHeight() >> 2),
                       gammaFromRulerValue(gammaCorrectionValue));
  builder.addLineOffset(gammaPreviewLineHeight);

  builder.addRuler(GAMMA_CORRECTION_ID, u"1.8", u"2.6", localizedText.getMessage(AdvancedEffectsMessages::gammaCorrection_tooltip),
                   builder.linePositionX() + (int32_t)gammaRulerOffsetX, Control::pageControlWidth(),
                   8, gammaCorrectionValue, gammaCorrection);

  // color group
  builder.addFieldset(localizedText.getMessage(AdvancedEffectsMessages::colorGroup), 4, 0, colorGroup);

  ComboBoxOption bitDepthOptions[]{ ComboBoxOption(u"16-bit", 0), ComboBoxOption(u"32-bit", 1) };
  builder.addSlider(0, localizedText.getMessage(AdvancedEffectsMessages::colorBitDepth),
                    localizedText.getMessage(AdvancedEffectsMessages::colorBitDepth_tooltip), controlWidth,
                    bitDepthOptions, sizeof(bitDepthOptions)/sizeof(*bitDepthOptions), 1, colorBitDepth);

  ComboBoxOption scanlinesOptions[]{ ComboBoxOption(localizedText.getMessage(CommonMessages::disabled), 0),
                                    ComboBoxOption(u"Dark lines", 1),
                                    ComboBoxOption(u"CRT-hyllian", 2),
                                    ComboBoxOption(u"CRT-royale", 3) };
  builder.addComboBox(0, localizedText.getMessage(AdvancedEffectsMessages::scanlines),
                      localizedText.getMessage(AdvancedEffectsMessages::scanlines_tooltip), controlWidth,
                      scanlinesOptions, sizeof(scanlinesOptions)/sizeof(*scanlinesOptions), 0, scanlines);
  ComboBoxOption colorFilterOptions[]{ ComboBoxOption(localizedText.getMessage(CommonMessages::disabled), 0),
                                    ComboBoxOption(u"Natural-vision (CRT colors)", 1),
                                    ComboBoxOption(u"Grayscale (desaturated)", 2),
                                    ComboBoxOption(u"Sepia (desaturated)", 3),
                                    ComboBoxOption(u"High-intensity colors", 4),
                                    ComboBoxOption(u"Night-blue overlay", 5),
                                    ComboBoxOption(u"Scifi-green overlay", 6),
                                    ComboBoxOption(u"Golden overlay", 7),
                                    ComboBoxOption(u"Color-blind (protanopia)", 8),
                                    ComboBoxOption(u"Color-blind (deuteranopia)", 9),
                                    ComboBoxOption(u"Color-blind (tritanopia)", 10) };
  builder.addComboBox(0, localizedText.getMessage(AdvancedEffectsMessages::colorFilter),
                      localizedText.getMessage(AdvancedEffectsMessages::colorFilter_tooltip), controlWidth,
                      colorFilterOptions, sizeof(colorFilterOptions)/sizeof(*colorFilterOptions), 0, colorFilter);
  ComboBoxOption specialShadingOptions[]{ ComboBoxOption(localizedText.getMessage(CommonMessages::disabled), 0),
                                          ComboBoxOption(u"Pencil (Kirsch)", 1),
                                          ComboBoxOption(u"Cartoon (Cel-shading)", 2),
                                          ComboBoxOption(u"Storybook", 3),
                                          ComboBoxOption(u"Broken glass", 4) };
  builder.addComboBox(0, localizedText.getMessage(AdvancedEffectsMessages::specialShading),
                      localizedText.getMessage(AdvancedEffectsMessages::specialShading_tooltip), controlWidth,
                      specialShadingOptions, sizeof(specialShadingOptions)/sizeof(*specialShadingOptions), 0, specialShading);

  // hdr group
  builder.addFieldset(localizedText.getMessage(AdvancedEffectsMessages::hdrGroup), 2, 0, hdrGroup);

  builder.addRuler(0, localizedText.getMessage(AdvancedEffectsMessages::bloom), nullptr,
                   localizedText.getMessage(AdvancedEffectsMessages::bloom_tooltip),
                   Control::pageControlWidth() >> 2, 3, bloomValue, bloom);
  builder.addRuler(0, localizedText.getMessage(AdvancedEffectsMessages::lumaSharpen), nullptr,
                   localizedText.getMessage(AdvancedEffectsMessages::lumaSharpen_tooltip),
                   Control::pageControlWidth() >> 2, 3, lumaSharpenValue, lumaSharpen);

  // special game fixes group
  builder.addFieldset(localizedText.getMessage(AdvancedEffectsMessages::specialGameFixesGroup), 3, 0, specialGameFixesGroup);
  lowCompatibilityFrameReadValue = ignoreSmallFramebufferMovesValue = fakeGpuBusyStatesValue = false;

  builder.addCheckBox(0, localizedText.getMessage(AdvancedEffectsMessages::lowCompatibilityFrameRead),
                      localizedText.getMessage(AdvancedEffectsMessages::lowCompatibilityFrameRead_tooltip),
                      lowCompatibilityFrameReadValue, lowCompatibilityFrameRead);
  builder.addCheckBox(0, localizedText.getMessage(AdvancedEffectsMessages::ignoreSmallFramebufferMoves),
                      localizedText.getMessage(AdvancedEffectsMessages::ignoreSmallFramebufferMoves_tooltip),
                      ignoreSmallFramebufferMovesValue, ignoreSmallFramebufferMoves);
  builder.addCheckBox(0, localizedText.getMessage(AdvancedEffectsMessages::fakeGpuBusyStates),
                      localizedText.getMessage(AdvancedEffectsMessages::fakeGpuBusyStates_tooltip),
                      fakeGpuBusyStatesValue, fakeGpuBusyStates);

  // control registry
  Page::moveScrollbarThumb(builder.linePositionY());
  registerControls(std::move(builder.controlRegistry()));
}

AdvancedEffects::~AdvancedEffects() noexcept {
  title.release();

  gammaGroup.release();
  gammaCorrection.release();
  gammaPreview.release();

  colorGroup.release();
  colorBitDepth.release();
  scanlines.release();
  colorFilter.release();
  specialShading.release();

  hdrGroup.release();
  bloom.release();
  lumaSharpen.release();
  
  specialGameFixesGroup.release();
  lowCompatibilityFrameRead.release();
  ignoreSmallFramebufferMoves.release();
  fakeGpuBusyStates.release();
}


// -- window events -- ---------------------------------------------------------

void AdvancedEffects::move(int32_t x, int32_t y, uint32_t width, uint32_t height) {
  Page::moveBase(x, y, width, height);
  PageContentMover mover(*context, x, y, width);

  mover.moveTitle(title);

  constexpr const uint32_t gammaRulerOffsetX = Control::pageLabelWidth() - (Control::pageControlWidth() >> 2);
  constexpr const uint32_t gammaPreviewLineHeight = Control::pageLineHeight() + (Control::pageLineHeight() >> 1);

  // gamma group
  mover.moveFieldset(1, gammaPreviewLineHeight, gammaGroup);

  gammaPreview.move(context->renderer(), context->pixelSizeX(), context->pixelSizeY(),
                    mover.linePositionX() + (int32_t)gammaRulerOffsetX + (int32_t)Control::labelMargin(),
                    mover.linePositionY() - (int32_t)(Control::pageLineHeight() >> 2));
  mover.addLineOffset(gammaPreviewLineHeight);
  mover.moveRuler(mover.linePositionX() + (int32_t)gammaRulerOffsetX, gammaCorrection);

  // color group
  mover.moveFieldset(4, 0, colorGroup);

  mover.moveSlider(colorBitDepth);
  mover.moveComboBox(scanlines);
  mover.moveComboBox(colorFilter);
  mover.moveComboBox(specialShading);

  // hdr group
  mover.moveFieldset(2, 0, hdrGroup);

  mover.moveRuler(bloom);
  mover.moveRuler(lumaSharpen);

  // special game fixes group
  mover.moveFieldset(3, 0, specialGameFixesGroup);

  mover.moveCheckBox(lowCompatibilityFrameRead);
  mover.moveCheckBox(ignoreSmallFramebufferMoves);
  mover.moveCheckBox(fakeGpuBusyStates);

  Page::moveScrollbarThumb(mover.linePositionY()); // required after a move
}

void AdvancedEffects::onValueChange(uint32_t id, uint32_t value) {
  if (id == GAMMA_CORRECTION_ID) {
    updateGammaPreview(gammaFromRulerValue(value));
  }
}


// -- rendering -- -------------------------------------------------------------

void AdvancedEffects::drawIcons() {
  // scrollable geometry
  buffers->bindScrollLocationBuffer(context->renderer(), ScissorRectangle(x(), y(), width(), contentHeight()));

  auto* hoverControl = getActiveControl();
  lowCompatibilityFrameRead.drawIcon(*context, *buffers, (hoverControl == &lowCompatibilityFrameRead));
  ignoreSmallFramebufferMoves.drawIcon(*context, *buffers, (hoverControl == &ignoreSmallFramebufferMoves));
  fakeGpuBusyStates.drawIcon(*context, *buffers, (hoverControl == &fakeGpuBusyStates));
}

void AdvancedEffects::drawPageBackgrounds(int32_t mouseX, int32_t) {
  // scrollable geometry
  if (buffers->isFixedLocationBuffer())
    buffers->bindScrollLocationBuffer(context->renderer(), ScissorRectangle(x(), y(), width(), contentHeight()));

  buffers->bindControlBuffer(context->renderer(), ControlBufferType::regular);
  gammaPreview.draw(context->renderer());

  gammaGroup.drawBackground(*context, *buffers);
  colorGroup.drawBackground(*context, *buffers);
  hdrGroup.drawBackground(*context, *buffers);
  specialGameFixesGroup.drawBackground(*context, *buffers);

  gammaCorrection.drawBackground(*context, *buffers);
  bloom.drawBackground(*context, *buffers);
  lumaSharpen.drawBackground(*context, *buffers);

  auto* hoverControl = getActiveControl();
  colorBitDepth.drawBackground(*context, mouseX, *buffers, (hoverControl == &colorBitDepth));
  scanlines.drawBackground(*context, *buffers, (hoverControl == &scanlines));
  colorFilter.drawBackground(*context, *buffers, (hoverControl == &colorFilter));
  specialShading.drawBackground(*context, *buffers, (hoverControl == &specialShading));
}

void AdvancedEffects::drawPageLabels() {
  // scrollable geometry
  auto& renderer = context->renderer();
  if (buffers->isFixedLocationBuffer())
    buffers->bindScrollLocationBuffer(renderer, ScissorRectangle(x(), y(), width(), contentHeight()));

  buffers->bindLabelBuffer(renderer, LabelBufferType::title);
  title.draw(renderer);

  gammaGroup.drawLabel(*context, *buffers);
  colorGroup.drawLabel(*context, *buffers);
  hdrGroup.drawLabel(*context, *buffers);
  specialGameFixesGroup.drawLabel(*context, *buffers);

  auto* hoverControl = getActiveControl();
  gammaCorrection.drawLabels(*context, *buffers, (hoverControl == &gammaCorrection));
  bloom.drawLabels(*context, *buffers, (hoverControl == &bloom));
  lumaSharpen.drawLabels(*context, *buffers, (hoverControl == &lumaSharpen));

  colorBitDepth.drawLabels(*context, *buffers, (hoverControl == &colorBitDepth));
  scanlines.drawLabels(*context, *buffers, (hoverControl == &scanlines));
  colorFilter.drawLabels(*context, *buffers, (hoverControl == &colorFilter));
  specialShading.drawLabels(*context, *buffers, (hoverControl == &specialShading));

  lowCompatibilityFrameRead.drawLabel(*context, *buffers, (hoverControl == &lowCompatibilityFrameRead));
  ignoreSmallFramebufferMoves.drawLabel(*context, *buffers, (hoverControl == &ignoreSmallFramebufferMoves));
  fakeGpuBusyStates.drawLabel(*context, *buffers, (hoverControl == &fakeGpuBusyStates));
}
