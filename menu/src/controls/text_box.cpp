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
#define _CRT_SECURE_NO_WARNINGS
#include <cassert>
#include <cstdio>
#include <cstring>
#include "menu/controls/geometry_generator.h"
#include "menu/controls/text_box.h"
#ifdef _WINDOWS
# define itoa(...) _itoa(__VA_ARGS__)
#endif

using namespace display;
using namespace display::controls;
using namespace menu::controls;
using namespace menu;

ControlType TextBox::Type() const noexcept { return ControlType::textBox; }


// -- init/resize geometry -- --------------------------------------------------

static inline void fillCaretColor(const float backgroundColor[4], float outColor[4]) {
  float gray = (backgroundColor[0] + backgroundColor[1] + backgroundColor[2]) * 0.333f;
  if (gray >= 0.6f) {
    gray *= 0.5f;
    *outColor = gray;     // r
    *(++outColor) = gray; // g
    *(++outColor) = gray; // b
  }
  else {
    *outColor = 0.1f + backgroundColor[0]*1.5f*0.9f;     // r
    if (*outColor > 1.f)
      *outColor = 1.f;
    *(++outColor) = 0.1f + backgroundColor[1]*1.5f*0.9f; // g
    if (*outColor > 1.f)
      *outColor = 1.f;
    *(++outColor) = 0.1f + backgroundColor[2]*1.5f*0.9f; // b
    if (*outColor > 1.f)
      *outColor = 1.f;
  }
  *(++outColor) = 0.75f; // a
}

// ---

void TextBox::init(RendererContext& context, const char32_t* label, const char32_t* suffix,
                   int32_t x, int32_t labelY, uint32_t fixedWidth, const float color[4], const char32_t* initValue) {
  auto& inputFont = context.getFont(FontType::inputText);
  auto& labelFont = context.getFont(FontType::labels);
  const uint32_t height = inputFont.XHeight() + (paddingY << 1);

  // create label
  labelMesh = TextMesh(context.renderer(), labelFont, label, context.pixelSizeX(), context.pixelSizeY(), x, labelY);
  uint32_t labelWidthWithMargin = (minLabelWidth >= labelMesh.width()) ? minLabelWidth : labelMesh.width();
  if (labelWidthWithMargin)
    labelWidthWithMargin += labelMargin();

  // create background + border
  const float colorDarker[4]{ color[0]*0.9f, color[1]*0.9f, color[2]*0.9f, color[3] };
  std::vector<ControlVertex> vertices(static_cast<size_t>(22));
  ControlVertex* vertexIt = vertices.data();
  GeometryGenerator::fillControlVertex(*vertexIt,     colorDarker, 0.f,               0.f); // background (gradient from top)
  GeometryGenerator::fillControlVertex(*(++vertexIt), colorDarker, (float)fixedWidth, 0.f);
  GeometryGenerator::fillRectangleVertices(++vertexIt, color, 0.f, (float)fixedWidth, -(float)((height<<1)/3), -(float)height);
  vertexIt += 4;

  const float borderFactor = (color[0] + color[1] + color[2] >= 1.5f) ? 0.7f : 1.3f;
  const float colorBorder[4]{ color[0]*borderFactor, color[1]*borderFactor, color[2]*borderFactor, color[3] };
  GeometryGenerator::fillRectangleVertices(vertexIt, colorBorder, 0.f, (float)fixedWidth, 0.f, -1.f); // border top
  vertexIt += 4;
  GeometryGenerator::fillRectangleVertices(vertexIt, colorBorder, 0.f, (float)fixedWidth, // border bottom
                                           -(float)(height-1), -(float)height);
  vertexIt += 4;
  GeometryGenerator::fillRectangleVertices(vertexIt, colorBorder, 0.f, 1.f, -1.f, -(float)(height-1)); // border left
  vertexIt += 4;
  GeometryGenerator::fillRectangleVertices(vertexIt, colorBorder, (float)(fixedWidth-1), (float)fixedWidth, // border right
                                           -1.f, -(float)(height-1));
  std::vector<uint32_t> indices{ 0,1,2, 2,1,3,  2,3,4, 4,3,5,  6,7,8, 8,7,9,
                                 10,11,12, 12,11,13,  14,15,16, 16,15,17, 18,19,20, 20,19,21 };

  const int32_t boxX = x + (int32_t)labelWidthWithMargin;
  const int32_t boxY = labelY - ((int32_t)height - (int32_t)labelFont.XHeight() + 1)/2;
  controlMesh = ControlMesh(context.renderer(), std::move(vertices), indices, context.pixelSizeX(), context.pixelSizeY(),
                            boxX, boxY, fixedWidth, height);

  // create input value
  const int32_t inputY = boxY + (int32_t)paddingY + (int32_t)(inputFont.XHeight() >> 2);
  inputMesh = TextMesh(context.renderer(), inputFont, initValue, context.pixelSizeX(), context.pixelSizeY(),
                       boxX + (int32_t)paddingX, inputY);
  if (suffix != nullptr && *suffix) {
    suffixMesh = TextMesh(context.renderer(), inputFont, suffix, context.pixelSizeX(), context.pixelSizeY(),
                          boxX + (int32_t)fixedWidth + (int32_t)labelMargin(), inputY);
  }

  // create caret
  float caretColor[4];
  fillCaretColor(color, caretColor);
  const int32_t caretY = inputMesh.y() - inputFont.XHeight() + (inputFont.XHeight() >> 2) + 1;
  const uint32_t caretHeight = (inputFont.XHeight() << 1);

  vertices = std::vector<ControlVertex>(static_cast<size_t>(4));
  GeometryGenerator::fillRectangleVertices(vertices.data(), caretColor, 0.f, 1.f, 0.f, -(float)caretHeight);
  indices = { 0,1,2, 2,1,3 };
  caretMesh = ControlMesh(context.renderer(), std::move(vertices), indices, context.pixelSizeX(), context.pixelSizeY(),
                          boxX + paddingX, caretY, fixedWidth, height);
  
  // input value storage
  inputValue.reserve((size_t)maxValueLength + 1u); // max possible length + ending zero
  if (initValue != nullptr) {
    for (const char32_t* it = initValue; *it; ++it)
      inputValue.emplace_back(*it);
  }
  inputValue.emplace_back((char32_t)0); // ending zero (to allow value() accessor to treat vector as a C-string)
}

// ---

void TextBox::move(RendererContext& context, int32_t x, int32_t labelY) {
  const int32_t boxY = labelY - ((int32_t)controlMesh.height() - (int32_t)labelMesh.height() + 1)/2;

  labelMesh.move(context.renderer(), context.pixelSizeX(), context.pixelSizeY(), x, labelY);
  uint32_t labelWidthWithMargin = (minLabelWidth >= labelMesh.width()) ? minLabelWidth : labelMesh.width();
  if (labelWidthWithMargin)
    labelWidthWithMargin += labelMargin();

  const int32_t boxX = x + (int32_t)labelWidthWithMargin;
  controlMesh.move(context.renderer(), context.pixelSizeX(), context.pixelSizeY(), boxX, boxY);

  const int32_t inputY = boxY + (int32_t)paddingY + (int32_t)(inputMesh.height() >> 2);
  inputMesh.move(context.renderer(), context.pixelSizeX(), context.pixelSizeY(), boxX + (int32_t)paddingX, inputY);
  if (suffixMesh.width()) {
    suffixMesh.move(context.renderer(), context.pixelSizeX(), context.pixelSizeY(),
                    boxX + (int32_t)controlMesh.width() + (int32_t)labelMargin(), inputY);
  }

  const int32_t caretY = inputMesh.y() - inputMesh.height() + (inputMesh.height() >> 2) + 1;
  caretMesh.move(context.renderer(), context.pixelSizeX(), context.pixelSizeY(), boxX + paddingX, caretY);
}

// ---

void TextBox::updateCaretLocation(RendererContext& context) {
  uint32_t currentLocation = 0;
  int32_t currentX = inputMesh.x();
  caretDrawCount = 0; // force draw caret

  for (const auto& glyph : inputMesh.meshGlyphs()) {
    if (currentLocation == caretLocation) {
      caretMesh.move(context.renderer(), context.pixelSizeX(), context.pixelSizeY(), currentX, caretMesh.y());
      return; // end here
    }
    currentX += (int32_t)(glyph->advance >> 6);
    ++currentLocation;
  }
  // caret at the end
  caretMesh.move(context.renderer(), context.pixelSizeX(), context.pixelSizeY(), currentX, caretMesh.y());
}


// -- accessors -- -------------------------------------------------------------

ControlStatus TextBox::getStatus(int32_t mouseX, int32_t mouseY) const noexcept {
  return isEnabled() ? (isHover(mouseX, mouseY) ? ControlStatus::hover : ControlStatus::regular) : ControlStatus::disabled;
}


// -- operations -- ------------------------------------------------------------

void TextBox::replaceValueText(RendererContext& context, const char32_t* textValue) {
  assert(valueType == TextBoxType::text || (textValue != nullptr && *textValue >= U'0' && *textValue <= U'9'));
  isEditing = false;
  const int32_t inputY = controlMesh.y() + (int32_t)paddingY + (int32_t)(inputMesh.height() >> 2);
  inputMesh = TextMesh(context.renderer(), context.getFont(FontType::inputText), textValue,
                       context.pixelSizeX(), context.pixelSizeY(), controlMesh.x() + (int32_t)paddingX, inputY);

  inputValue.clear();
  inputValue.reserve((size_t)maxValueLength + 1u);
  if (textValue != nullptr) {
    for (const char32_t* it = textValue; *it; ++it)
      inputValue.emplace_back(*it);
  }
  inputValue.emplace_back((char32_t)0); // ending zero
}

void TextBox::replaceValueInteger(RendererContext& context, uint32_t integerValue) {
  assert(valueType == TextBoxType::integer || valueType == TextBoxType::number);
  char32_t buffer[MAX_INTEGER_LENGTH+1];
  replaceValueText(context, fromInteger(integerValue, buffer));
}

void TextBox::replaceValueNumber(RendererContext& context, double numberValue) {
  assert(valueType == TextBoxType::number);
  auto buffer = fromNumber(numberValue, maxValueLength);
  replaceValueText(context, buffer.get());
}

// ---

bool TextBox::click(RendererContext& context, int32_t mouseX) {
  if (isEnabled()) {
    isEditing = true;
    caretDrawCount = 0;

    // detect caret location + move caret
    bool isCaretMoved = false;
    caretLocation = 0;
    int32_t currentX = inputMesh.x();
    for (const auto& glyph : inputMesh.meshGlyphs()) {
      uint32_t charWidth = (glyph->advance >> 6);
      if (mouseX <= currentX + (int32_t)(charWidth >> 1)) {
        caretMesh.move(context.renderer(), context.pixelSizeX(), context.pixelSizeY(), currentX, caretMesh.y());
        isCaretMoved = true;
        break;
      }
      currentX += (int32_t)charWidth;
      ++caretLocation;
    }
    if (!isCaretMoved) // caret at the end
      caretMesh.move(context.renderer(), context.pixelSizeX(), context.pixelSizeY(), currentX, caretMesh.y());
  }
  else { // close
    isEditing = false;
    if (onChange != nullptr)
      onChange(operationId);
  }
  return isEditing;
}

// ---

void TextBox::addChar(RendererContext& context, char32_t code) {
  if (inputValue.size() >= (size_t)maxValueLength + 1u) // max possible length + ending zero
    return;

  // verify character constraints
  if (valueType != TextBoxType::text) {
    if (code < U'0' || code > U'9') {
      if (valueType == TextBoxType::integer || (code != U'.' && code != U','))
        return;
      for (const auto it : inputValue) {
        if (it == U'.')
          return; // separator already set
      }
      code = U'.'; // force '.' separator
    }
  }

  auto& font = context.getFont(FontType::inputText);
  if (caretLocation >= (uint32_t)inputValue.size() - 1u/*ignore ending zero*/) {
    if (!inputMesh.push(context.renderer(), font, context.pixelSizeX(), context.pixelSizeY(), code))
      return;
    inputValue.back() = code;
    inputValue.emplace_back((char32_t)0); // new ending zero
  }
  else {
    if (!inputMesh.insertBefore(context.renderer(), font, context.pixelSizeX(), context.pixelSizeY(), code, caretLocation))
      return;
    inputValue.insert(inputValue.begin()+caretLocation, code);
  }
  ++caretLocation;
  updateCaretLocation(context);
}

void TextBox::removeChar(RendererContext& context) {
  if (caretLocation == 0 || inputValue.size() <= (size_t)1/*only ending zero*/)
    return;

  if (caretLocation >= (uint32_t)inputValue.size() - 1u/*ignore ending zero*/) {
    inputMesh.pop(context.renderer());
    inputValue.pop_back();
    inputValue.back() = (char32_t)0; // new ending zero
    --caretLocation;
  }
  else {
    --caretLocation;
    inputMesh.removeAt(context.renderer(), context.pixelSizeX(), caretLocation);
    inputValue.erase(inputValue.begin()+caretLocation);
  }
  updateCaretLocation(context);
}

void TextBox::close() {
  isEditing = false;
  if (onChange != nullptr)
    onChange(operationId);
}


// -- rendering -- -------------------------------------------------------------

void TextBox::drawBackground(RendererContext& context, RendererStateBuffers& buffers) {
  if (isEnabled()) {
    buffers.bindControlBuffer(context.renderer(), ControlBufferType::regular);
    controlMesh.draw(context.renderer());

    if (isEditing) {
      if (++caretDrawCount < 30u) // display during 30 frames
        caretMesh.draw(context.renderer());
      else if (caretDrawCount >= 60u) // hide during 30 frames, then reset counter
        caretDrawCount = 0;
    }
  }
  else {
    buffers.bindControlBuffer(context.renderer(), ControlBufferType::disabled);
    controlMesh.draw(context.renderer());
  }
}

void TextBox::drawLabels(RendererContext& context, RendererStateBuffers& buffers, bool isActive) {
  LabelBufferType labelBuffer, inputBuffer;
  if (isEnabled()) {
    labelBuffer = isActive ? LabelBufferType::active : LabelBufferType::regular;
    inputBuffer = LabelBufferType::textInput;
  }
  else {
    labelBuffer = LabelBufferType::disabled;
    inputBuffer = LabelBufferType::textInputDisabled;
  }

  buffers.bindLabelBuffer(context.renderer(), labelBuffer);
  if (labelMesh.width())
    labelMesh.draw(context.renderer());
  if (suffixMesh.width())
    suffixMesh.draw(context.renderer());

  buffers.bindLabelBuffer(context.renderer(), inputBuffer);
  inputMesh.draw(context.renderer());
}


// -- value parsers/accessors (int/double <-> string) -- -----------------------

uint32_t TextBox::valueInteger() const noexcept {
  assert(valueType == TextBoxType::integer);

  uint32_t value = 0;
  for (const char32_t* it = valueText(); *it; ++it) {
    value *= 10u;
    value += (uint32_t)(*it - U'0');
  }
  return value;
}

double TextBox::valueNumber() const noexcept {
  assert(valueType == TextBoxType::integer || valueType == TextBoxType::number);

  bool isSeparatorFound = false;
  uint32_t divider = 1;
  uint64_t value = 0;
  for (const char32_t* it = valueText(); *it; ++it) {
    if (*it != '.') {
      value *= 10uLL;
      value += (uint64_t)(*it - U'0');
      if (isSeparatorFound)
        divider *= 10u;
    }
    else isSeparatorFound = true;
  }
  return (double)value / (double)divider;
}

// ---

const char32_t* TextBox::fromInteger(uint32_t integerValue, char32_t buffer[MAX_INTEGER_LENGTH + 1]) noexcept {
  char32_t* it = &buffer[MAX_INTEGER_LENGTH];
  *it = (char32_t)0;

  if (integerValue) {
    while (integerValue) {
      *(--it) = (char32_t)(integerValue % 10u) + U'0';
      integerValue /= 10u;
    }
  }
  else *(--it) = U'0';
  return it;
}

std::unique_ptr<char32_t[]> TextBox::fromNumber(double numberValue, size_t bufferLength) {
  char formatter[MAX_INTEGER_LENGTH + 4] = "%."; // create "%.<bufferLength>f" formatter
  itoa((int)bufferLength, &formatter[2], 10);

  char* endFormatter = &formatter[3];
  while (*endFormatter)
    ++endFormatter;
  *endFormatter = 'f';
  *(++endFormatter) = '\0';

  size_t bufferSize = MAX_INTEGER_LENGTH + bufferLength + 2u; // format number
  std::unique_ptr<char[]> buffer(new char[bufferSize]);
  int length = snprintf(buffer.get(), bufferSize, formatter, numberValue);
  if (length > 0) {
    if (length >= (int)bufferLength)
      length = (int)bufferLength;

    char* revIt = &(buffer.get()[length - 1]); // remove trailing zeroes
    while (*revIt == '0' && revIt > buffer.get()) {
      --revIt;
      --length;
    }
    if (*revIt == '.') // no decimals -> remove '.' too
      --length;
  }

  // copy formatted string into char32 buffer
  std::unique_ptr<char32_t[]> output;
  if (length > 0) {
    buffer.get()[length] = '\0';
    output.reset(new char32_t[length + 1]);

    char32_t* destIt = output.get();
    for (const char* srcIt = buffer.get(); *srcIt; ++srcIt, ++destIt)
      *destIt = (char32_t)*srcIt;
    output.get()[length] = (char32_t)0;
  }
  else {
    output.reset(new char32_t[2]);
    *output.get() = U'0';
    output.get()[1] = (char32_t)0;
  }
  return output;
}
