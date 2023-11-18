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
#include <cstring>
#include "menu/controls/button.h"
#include "menu/controls/check_box.h"
#include "menu/controls/combo_box.h"
#include "menu/controls/fieldset.h"
#include "menu/controls/key_binding.h"
#include "menu/controls/ruler.h"
#include "menu/controls/slider.h"
#include "menu/controls/text_box.h"
#include "menu/controls/geometry_generator.h"
#include "menu/pages/page_content_builder.h"

using namespace display;
using namespace display::controls;
using namespace menu;
using namespace menu::controls;
using namespace menu::pages;

PageContentBuilder::PageContentBuilder(RendererContext& context, const ColorTheme& theme,
                                       int32_t pageX, int32_t pageY, uint32_t pageWidth,
                                       size_t reservedRegisteredControlCount,
                                       std::function<void(uint32_t)> keyChangeHandler,
                                       std::function<void(uint32_t,uint32_t)> keyValueChangeHandler)
  : context(&context),
    theme(&theme),
    currentLineY(pageY),
    keyChangeHandler(std::move(keyChangeHandler)),
    keyValueChangeHandler(std::move(keyValueChangeHandler)) {
  const uint32_t fieldsetPaddingX = Control::fieldsetMarginX(pageWidth);
  controlX = pageX + (int32_t)fieldsetPaddingX + (int32_t)Control::fieldsetContentMarginX(pageWidth);
  fieldsetX = pageX + (int32_t)fieldsetPaddingX;
  
  fieldsetWidth = pageWidth - (fieldsetPaddingX << 1);
  if (fieldsetWidth > Control::fieldsetMaxWidth())
    fieldsetWidth = Control::fieldsetMaxWidth();
  
  registry.reserve(reservedRegisteredControlCount);
}

PageContentMover::PageContentMover(RendererContext& context, int32_t pageX, int32_t pageY, uint32_t pageWidth)
  : context(&context),
    currentLineY(pageY) {
  const uint32_t fieldsetPaddingX = Control::fieldsetMarginX(pageWidth);
  controlX = pageX + (int32_t)fieldsetPaddingX + (int32_t)Control::fieldsetContentMarginX(pageWidth);
  fieldsetX = pageX + (int32_t)fieldsetPaddingX;
  
  fieldsetWidth = pageWidth - (fieldsetPaddingX << 1);
  if (fieldsetWidth > Control::fieldsetMaxWidth())
    fieldsetWidth = Control::fieldsetMaxWidth();
}


// -- page decorations -- ------------------------------------------------------

void PageContentBuilder::addTitle(const char16_t* label, TextMesh& outTitle) {
  outTitle = TextMesh(context->renderer(),
                      context->getFont(FontType::titles), label,
                      context->pixelSizeX(), context->pixelSizeY(),
                      fieldsetX, currentLineY + Control::titleMarginTop(),
                      TextAlignment::left);
  currentLineY = outTitle.y() + (int32_t)outTitle.height() + (int32_t)Control::pageLineHeight()
               - Control::fieldsetContentMarginBottom(); // cancel margin for first fieldset
}

void PageContentMover::moveTitle(TextMesh& title) {
  title.move(context->renderer(),
             context->pixelSizeX(), context->pixelSizeY(),
             fieldsetX, currentLineY + Control::titleMarginTop());
  currentLineY = title.y() + (int32_t)title.height() + (int32_t)Control::pageLineHeight()
               - Control::fieldsetContentMarginBottom(); // cancel margin for first fieldset
}

// ---

void PageContentBuilder::addFieldset(const char16_t* label, uint32_t controlLineCount, uint32_t additionalPaddingY,
                                     Fieldset& outFieldset) {
  currentLineY += Control::fieldsetContentMarginBottom();
  outFieldset = Fieldset(*context, label,
                         theme->fieldsetStyle(), theme->fieldsetControlColor(),
                         fieldsetX, currentLineY,
                         fieldsetWidth, Control::fieldsetContentHeight(controlLineCount) + additionalPaddingY);
  currentLineY += Control::pageLineHeight() + Control::fieldsetContentPaddingTop();
}

void PageContentMover::moveFieldset(uint32_t controlLineCount, uint32_t additionalPaddingY, Fieldset& fieldset) {
  currentLineY += Control::fieldsetContentMarginBottom();
  fieldset.move(*context, fieldsetX, currentLineY,
                fieldsetWidth, Control::fieldsetContentHeight(controlLineCount) + additionalPaddingY);
  currentLineY += Control::pageLineHeight() + Control::fieldsetContentPaddingTop();
}


// -- page controls -- ---------------------------------------------------------

void PageContentBuilder::addCheckBox(uint32_t controlId, const char16_t* label, const char16_t* tooltip,
                                     bool& boundValue, CheckBox& outCheckBox) {
  outCheckBox = CheckBox(*context, label,
                         controlX, currentLineY,
                         Control::pageLabelWidth(),
                         controlId, controlId ? keyValueChangeHandler : nullptr,
                         boundValue, enabler);
  registry.emplace_back(outCheckBox, true, tooltip);
  currentLineY += Control::pageLineHeight();
}

void PageContentMover::moveCheckBox(CheckBox& checkBox) {
  checkBox.move(*context, controlX, currentLineY);
  currentLineY += Control::pageLineHeight();
}

// ---

void PageContentBuilder::addComboBox(uint32_t controlId, const char16_t* label, const char16_t* tooltip,
                                     uint32_t minControlWidth, ComboBoxOption* options, size_t optionCount,
                                     int32_t selectedIndex, ComboBox& outComboBox) {
  outComboBox = ComboBox(*context, label,
                         controlX, currentLineY,
                         Control::pageLabelWidth(), minControlWidth,
                         ComboBoxStyle::cutCorner, theme->comboBoxColorParams(),
                         controlId, controlId ? keyValueChangeHandler : nullptr,
                         options, optionCount, selectedIndex, enabler);
  registry.emplace_back(outComboBox, true, tooltip);
  currentLineY += Control::pageLineHeight();
}

void PageContentMover::moveComboBox(ComboBox& comboBox) {
  comboBox.move(*context, controlX, currentLineY);
  currentLineY += Control::pageLineHeight();
}

void PageContentBuilder::addComboBox(uint32_t controlId, const char16_t* tooltip, int32_t x, uint32_t minControlWidth,
                                     ComboBoxOption* options, size_t optionCount,
                                     int32_t selectedIndex, ComboBox& outComboBox) {
  outComboBox = ComboBox(*context, nullptr,
                         x, currentLineY,
                         0, minControlWidth,
                         ComboBoxStyle::cutCorner, theme->comboBoxColorParams(),
                         controlId, controlId ? keyValueChangeHandler : nullptr,
                         options, optionCount, selectedIndex, enabler);
  registry.emplace_back(outComboBox, true, tooltip);
  currentLineY += Control::pageLineHeight();
}

void PageContentMover::moveComboBox(int32_t x, ComboBox& comboBox) {
  comboBox.move(*context, x, currentLineY);
  currentLineY += Control::pageLineHeight();
}

void PageContentBuilder::addDoubleComboBox(uint32_t leftControlId, const char16_t* label, const char16_t* tooltip,
                                           ComboBoxOption* leftOptions, size_t leftOptionCount, int32_t selectedLeftIndex,
                                           ComboBox& outLeftComboBox,
                                           uint32_t rightControlId, uint32_t rightControlWidth,
                                           ComboBoxOption* rightOptions, size_t rightOptionCount, int32_t selectedRightIndex,
                                           ComboBox& outRightComboBox) {
  outLeftComboBox = ComboBox(*context, label,
                             controlX, currentLineY,
                             Control::pageLabelWidth(),
                             Control::pageControlWidth() - Control::controlSideMargin() - rightControlWidth,
                             ComboBoxStyle::classic, theme->comboBoxColorParams(),
                             leftControlId, leftControlId ? keyValueChangeHandler : nullptr,
                             leftOptions, leftOptionCount, selectedLeftIndex, enabler);
  outRightComboBox = ComboBox(*context, nullptr,
                              outLeftComboBox.x() + (int32_t)outLeftComboBox.width() + (int32_t)Control::controlSideMargin(),
                              currentLineY, 0, rightControlWidth,
                              ComboBoxStyle::cutCorner, theme->comboBoxColorParams(),
                              rightControlId, rightControlId ? keyValueChangeHandler : nullptr,
                              rightOptions, rightOptionCount, selectedRightIndex, enabler);
  registry.emplace_back(outLeftComboBox, true, tooltip);
  registry.emplace_back(outRightComboBox, true, tooltip, Control::controlSideMargin());
  currentLineY += Control::pageLineHeight();
}

void PageContentMover::moveDoubleComboBox(ComboBox& leftComboBox, ComboBox& rightComboBox) {
  leftComboBox.move(*context, controlX, currentLineY);
  rightComboBox.move(*context, leftComboBox.x() + (int32_t)leftComboBox.width() + (int32_t)Control::controlSideMargin(),
                     currentLineY);
  currentLineY += Control::pageLineHeight();
}

void PageContentBuilder::addComboBoxWithButton(uint32_t controlId, const char16_t* label, const char16_t* tooltip,
                                               ComboBoxOption* options, size_t optionCount, ComboBox& outComboBox,
                                               uint32_t buttonId, const char16_t* buttonLabel, Button& outButton) {
  outComboBox = ComboBox(*context, label,
                         controlX, currentLineY,
                         Control::pageLabelWidth(),
                         (Control::pageControlWidth() >> 1),
                         ComboBoxStyle::classic, theme->comboBoxColorParams(),
                         controlId, controlId ? keyValueChangeHandler : nullptr,
                         options, optionCount, 0, enabler);

  ButtonStyleProperties buttonStyle(ButtonStyle::fromTopLeft, FontType::inputText, ControlIconType::none,
                                    theme->buttonControlColor(), theme->buttonBorderColor(), 1, 0,
                                    Control::buttonPaddingX(), Control::comboBoxPaddingY());
  outButton = Button(*context, buttonLabel,
                     outComboBox.x() + (int32_t)outComboBox.width() + (int32_t)Control::controlSideMargin(),
                     currentLineY, buttonStyle,
                     buttonId, buttonId ? keyChangeHandler : nullptr, enabler);

  registry.emplace_back(outComboBox, true, tooltip);
  registry.emplace_back(outButton, true, tooltip, Control::controlSideMargin());
  currentLineY += Control::pageLineHeight();
}

void PageContentMover::moveComboBoxWithButton(ComboBox& comboBox, Button& button) {
  comboBox.move(*context, controlX, currentLineY);
  button.move(*context, comboBox.x() + (int32_t)comboBox.width() + (int32_t)Control::controlSideMargin(), currentLineY);
  currentLineY += Control::pageLineHeight();
}

// ---

void PageContentBuilder::addRuler(uint32_t controlId, const char16_t* label, const char16_t* suffix, const char16_t* tooltip,
                                  uint32_t controlWidth, uint32_t maxValue, uint32_t& boundValue, Ruler& outRuler) {
  outRuler = Ruler(*context, label, suffix, FontType::labels, TextAlignment::left,
                   controlX, currentLineY,
                   Control::pageLabelWidth(), controlWidth,
                   theme->rulerColorParams(),
                   controlId, controlId ? keyValueChangeHandler : nullptr,
                   0, maxValue, 1, boundValue, enabler);
  registry.emplace_back(outRuler, true, tooltip);
  currentLineY += Control::pageLineHeight();
}

void PageContentMover::moveRuler(Ruler& ruler) {
  ruler.move(*context, controlX, currentLineY, TextAlignment::left);
  currentLineY += Control::pageLineHeight();
}

void PageContentBuilder::addRuler(uint32_t controlId, const char16_t* prefix, const char16_t* suffix, const char16_t* tooltip,
                                  int32_t x, uint32_t controlWidth, uint32_t maxValue, uint32_t& boundValue, Ruler& outRuler) {
  int32_t rulerLabelOffsetY = (context->getFont(FontType::labels).XHeight() - context->getFont(FontType::inputText).XHeight())/2;
  outRuler = Ruler(*context, prefix, suffix, FontType::inputText, TextAlignment::right,
                   x, currentLineY + rulerLabelOffsetY,
                   0, controlWidth,
                   theme->rulerColorParams(),
                   controlId, controlId ? keyValueChangeHandler : nullptr,
                   0, maxValue, 1, boundValue, enabler);
  registry.emplace_back(outRuler, true, tooltip);
  currentLineY += Control::pageLineHeight();
}

void PageContentMover::moveRuler(int32_t x, Ruler& ruler) {
  ruler.move(*context, x, currentLineY, TextAlignment::right);
  currentLineY += Control::pageLineHeight();
}

// ---

void PageContentBuilder::addSlider(uint32_t controlId, const char16_t* label, const char16_t* tooltip,
                                   uint32_t controlWidth, ComboBoxOption* options, size_t optionCount,
                                   int32_t selectedIndex, Slider& outSlider) {
  outSlider = Slider(*context, label,
                     controlX, currentLineY,
                     Control::pageLabelWidth(), controlWidth,
                     theme->sliderArrowColor(),
                     controlId, controlId ? keyValueChangeHandler : nullptr,
                     options, optionCount, selectedIndex, enabler);
  registry.emplace_back(outSlider, true, tooltip);
  currentLineY += Control::pageLineHeight();
}

void PageContentMover::moveSlider(Slider& slider) {
  slider.move(*context, controlX, currentLineY);
  currentLineY += Control::pageLineHeight();
}

void PageContentBuilder::addSlider(uint32_t controlId, const char16_t* tooltip, int32_t x, uint32_t controlWidth,
                                   ComboBoxOption* options, size_t optionCount,
                                   int32_t selectedIndex, Slider& outSlider) {
  outSlider = Slider(*context, nullptr,
                     x, currentLineY,
                     0, controlWidth,
                     theme->sliderArrowColor(),
                     controlId, controlId ? keyValueChangeHandler : nullptr,
                     options, optionCount, selectedIndex, enabler);
  registry.emplace_back(outSlider, true, tooltip);
  currentLineY += Control::pageLineHeight();
}

void PageContentMover::moveSlider(int32_t x, Slider& slider) {
  slider.move(*context, x, currentLineY);
  currentLineY += Control::pageLineHeight();
}

// ---

void PageContentBuilder::addStringTextBox(uint32_t controlId, const char16_t* label, const char16_t* tooltip,
                                          const char16_t* value, uint32_t maxValueLength, TextBox& outTextBox) {
  outTextBox = TextBox(*context, label, nullptr,
                       controlX, currentLineY,
                       Control::pageLabelWidth(), Control::pageControlWidth(),
                       theme->textBoxControlColor(),
                       controlId, controlId ? keyChangeHandler : nullptr,
                       value, maxValueLength);
  registry.emplace_back(outTextBox, true, tooltip);
  currentLineY += Control::pageLineHeight();
}

void PageContentBuilder::addResolutionTextBox(uint32_t controlId, const char16_t* label, const char16_t* tooltip,
                                              uint32_t controlWidth, uint32_t controlOffsetX,
                                              uint32_t integerValue, TextBox& outTextBox) {
  outTextBox = TextBox(*context, label, nullptr,
                       controlX, currentLineY,
                       Control::pageLabelWidth() + controlOffsetX, controlWidth,
                       theme->textBoxControlColor(),
                       controlId, controlId ? keyChangeHandler : nullptr,
                       integerValue, 4u, false, enabler);
  registry.emplace_back(outTextBox, true, tooltip);
  currentLineY += Control::pageLineHeight();
}

void PageContentBuilder::addNumberTextBox(uint32_t controlId, const char16_t* label, const char16_t* suffix, const char16_t* tooltip,
                                          uint32_t controlWidth, double numberValue, TextBox& outTextBox) {
  outTextBox = TextBox(*context, label, suffix,
                       controlX, currentLineY,
                       Control::pageLabelWidth(), controlWidth,
                       theme->textBoxControlColor(),
                       controlId, controlId ? keyChangeHandler : nullptr,
                       numberValue, 6u, enabler);
  registry.emplace_back(outTextBox, true, tooltip);
  currentLineY += Control::pageLineHeight();
}

void PageContentMover::moveTextBox(TextBox& textBox) {
  textBox.move(*context, controlX, currentLineY);
  currentLineY += Control::pageLineHeight();
}

void PageContentBuilder::addDoubleTextBox(uint32_t leftControlId, uint32_t rightControlId,
                                          const char16_t* label, const char16_t* suffix, const char16_t* tooltip,
                                          uint32_t leftIntegerValue, uint32_t rightIntegerValue, uint32_t maxValueLength,
                                          TextBox& outLeftTextBox, TextBox& outRightTextBox) {
  outLeftTextBox = TextBox(*context, label, nullptr,
                           controlX, currentLineY,
                           Control::pageLabelWidth(), (Control::pageControlWidth() >> 2) - 1u,
                           theme->textBoxControlColor(),
                           leftControlId, leftControlId ? keyChangeHandler : nullptr,
                           leftIntegerValue, maxValueLength, true, enabler);
  outRightTextBox = TextBox(*context, nullptr, suffix,
                            outLeftTextBox.x() + (int32_t)outLeftTextBox.width() + (int32_t)Control::controlSideMargin(),
                            currentLineY, 0, (Control::pageControlWidth() >> 2) - 1u,
                            theme->textBoxControlColor(),
                            rightControlId, rightControlId ? keyChangeHandler : nullptr,
                            rightIntegerValue, maxValueLength, true, enabler);
  registry.emplace_back(outLeftTextBox, true, tooltip);
  registry.emplace_back(outRightTextBox, true, tooltip, Control::controlSideMargin());
  currentLineY += Control::pageLineHeight();
}

void PageContentMover::moveDoubleTextBox(TextBox& leftTextBox, TextBox& rightTextBox) {
  leftTextBox.move(*context, controlX, currentLineY);
  rightTextBox.move(*context, leftTextBox.x() + (int32_t)leftTextBox.width() + (int32_t)Control::controlSideMargin(),
                    currentLineY);
  currentLineY += Control::pageLineHeight();
}

// ---

void PageContentBuilder::addControllerKeyBinding(const char16_t* label, const char16_t* tooltip,
                                                 uint32_t keyCode, KeyBinding& outBinding) {
  outBinding = KeyBinding(*context, label,
                          controlX, currentLineY,
                          Control::pageLabelWidth(), (Control::pageControlWidth() >> 1) + 1,
                          theme->textBoxControlColor(), theme->keyboardKeyColorParams(), KeyBindingType::controller,
                          KeyBinding::emptyKeyValue(), keyCode,
                          false, enabler);
  registry.emplace_back(outBinding, true, tooltip);
  currentLineY += Control::pageLineHeight();
}

void PageContentBuilder::addDualKeyBinding(const char16_t* label, const char16_t* tooltip,
                                           uint32_t keyboardKeyCode, uint32_t controllerKeyCode, KeyBinding& outBinding) {
  outBinding = KeyBinding(*context, label,
                          controlX, currentLineY,
                          Control::pageLabelWidth(), Control::pageControlWidth(),
                          theme->textBoxControlColor(), theme->keyboardKeyColorParams(),
                          KeyBindingType::both, keyboardKeyCode,
                          controllerKeyCode, true, enabler);
  registry.emplace_back(outBinding, true, tooltip);
  currentLineY += Control::pageLineHeight();
}

void PageContentMover::moveKeyBinding(KeyBinding& binding) {
  binding.move(*context, controlX, currentLineY);
  currentLineY += Control::pageLineHeight();
}

// ---

static void fillColorPreviewBorderRgba(const float previewColor[4], float outRgba[4]) {
  *outRgba = *previewColor * 0.6f;
  *(++outRgba) = *(++previewColor) * 0.6f;
  *(++outRgba) = *(++previewColor) * 0.6f;
  *(++outRgba) = *(++previewColor);
}
static void fillColorPreviewTopRgba(const float previewColor[4], float outRgba[4]) {
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

void PageContentBuilder::addColorPicker(uint32_t controlId, const char16_t* label, const float previewColorRgba[4],
                                        ComboBoxOption* options, size_t optionCount, int32_t selectedIndex,
                                        ComboBox& outComboBox, ControlMesh& outColorPreview) {
  outComboBox = ComboBox(*context, label,
                         controlX, currentLineY,
                         Control::pageLabelWidth(), (Control::pageControlWidth() >> 1),
                         ComboBoxStyle::classic, theme->comboBoxColorParams(),
                         controlId, controlId ? keyValueChangeHandler : nullptr,
                         options, optionCount, selectedIndex, enabler);
  registry.emplace_back(outComboBox, true, nullptr);

  const uint32_t previewBoxSize = outComboBox.height();
  float previewBorderRgba[4];
  fillColorPreviewBorderRgba(previewColorRgba, previewBorderRgba);

  std::vector<ControlVertex> vertices(static_cast<size_t>(8)); 
  GeometryGenerator::fillRectangleVertices(vertices.data(), previewBorderRgba,
                                            0.f, (float)previewBoxSize, 0.f, -(float)previewBoxSize);
  GeometryGenerator::fillRectangleVertices(vertices.data() + 4, previewColorRgba,
                                            1.f, (float)previewBoxSize - 1.f, -1.f, -(float)previewBoxSize + 1.f);
  fillColorPreviewTopRgba(previewColorRgba, vertices[4].color);
  memcpy(vertices[5].color, vertices[4].color, sizeof(float)*4u);

  std::vector<uint32_t> indices{ 0,1,2,2,1,3,  4,5,6,6,5,7 };
  outColorPreview = ControlMesh(context->renderer(), std::move(vertices), indices, context->pixelSizeX(), context->pixelSizeY(),
                                outComboBox.x() + (int32_t)outComboBox.width() + 1, outComboBox.y(), previewBoxSize, previewBoxSize);
  currentLineY += Control::pageLineHeight();
}

void PageContentBuilder::changeColorPickerColors(RendererContext& context, const float previewColorRgba[4],
                                                 ControlMesh& colorPreview) {
  float previewBorderRgba[4];
  fillColorPreviewBorderRgba(previewColorRgba, previewBorderRgba);

  std::vector<ControlVertex> vertices = colorPreview.relativeVertices();
  memcpy(vertices[0].color, previewBorderRgba, sizeof(float)*4);
  memcpy(vertices[1].color, previewBorderRgba, sizeof(float)*4);
  memcpy(vertices[2].color, previewBorderRgba, sizeof(float)*4);
  memcpy(vertices[3].color, previewBorderRgba, sizeof(float)*4);
  fillColorPreviewTopRgba(previewColorRgba, vertices[4].color);
  memcpy(vertices[5].color, vertices[4].color, sizeof(float)*4u);
  memcpy(vertices[6].color, previewColorRgba, sizeof(float)*4);
  memcpy(vertices[7].color, previewColorRgba, sizeof(float)*4);

  colorPreview.update(context.renderer(), std::move(vertices), context.pixelSizeX(), context.pixelSizeY(),
                      colorPreview.x(), colorPreview.y(), colorPreview.width(), colorPreview.height());
}

void PageContentMover::moveColorPicker(ComboBox& comboBox, ControlMesh& colorPreview) {
  comboBox.move(*context, controlX, currentLineY);
  colorPreview.move(context->renderer(), context->pixelSizeX(), context->pixelSizeY(),
                    comboBox.x() + (int32_t)comboBox.width() + 1, comboBox.y());
  currentLineY += Control::pageLineHeight();
}
