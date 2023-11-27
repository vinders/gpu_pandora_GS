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
#include "menu/controls/geometry_generator.h"
#include "menu/controls/popup.h"

using namespace display;
using namespace display::controls;
using namespace menu::controls;
using namespace menu;


// -- init/resize geometry -- --------------------------------------------------

#define BUTTON_WIDTH        80
#define BUTTON_OUTER_MARGIN 12

static uint32_t computePopupWidth(uint32_t buttonCount, uint32_t fontHeight, const char16_t* message, uint32_t maxWidth) {
  uint32_t messageLength = (uint32_t)TextMesh::getStringLength(message);
  const uint32_t textWidth = ((messageLength*fontHeight*3u) >> 2) + 32u;
  const uint32_t buttonBarWidth = buttonCount*(uint32_t)BUTTON_WIDTH + (BUTTON_OUTER_MARGIN << 1);
  
  uint32_t width = (textWidth >= buttonBarWidth) ? textWidth : buttonBarWidth;
  if (width < Popup::minWidth())
    width = Popup::minWidth();
  else if (width > maxWidth)
    width = maxWidth;
  return width;
}

// ---

Popup::Popup(RendererContext& context, const ColorTheme& theme, const char16_t* title, const char16_t* message,
             std::function<void(uint32_t)> onAction, const char16_t** buttonLabels, size_t buttonCount)
  : onAction(std::move(onAction)) {
  // create background hider
  const float hiderColor[4]{ theme.backgroundColor()[0], theme.backgroundColor()[1], theme.backgroundColor()[2], 0.85f };
  std::vector<ControlVertex> vertices(static_cast<size_t>(4));
  GeometryGenerator::fillRectangleVertices(vertices.data(), hiderColor,
                                           0.f, (float)context.clientWidth(), 0.f, -(float)context.clientHeight());
  std::vector<uint32_t> indices{ 0,1,2,2,1,3 };
  backMesh = ControlMesh(context.renderer(), std::move(vertices), indices, context.pixelSizeX(), context.pixelSizeY(),
                         0, 0, context.clientWidth(), context.clientHeight());
  
  // define popup size
  auto& labelFont = context.getFont(FontType::labels);
  auto& messageFont = context.getFont(FontType::inputText);
  const uint32_t width = computePopupWidth((uint32_t)buttonCount, messageFont.XHeight(), message, context.clientWidth() - 20u);
  const int32_t x = (int32_t)(context.clientWidth() >> 1) - (int32_t)(width >> 1);
  const int32_t y = (int32_t)(context.clientHeight() >> 1) - (int32_t)((height()*3u) >> 2);
  const uint32_t titleBarHeight = labelFont.XHeight() + (Control::fieldsetTitlePaddingY() << 1);
  constexpr const uint32_t buttonBarHeight = 31u;
  
  // create popup background
  vertices.resize((size_t)16);
  ControlVertex* vertexIt = vertices.data();

  const float* titleBarColor = theme.popupTitleBarColor();
  GeometryGenerator::fillRectangleVertices(vertexIt, titleBarColor, // title bar background
                                           0.f, (float)width, -1.f, -(float)(titleBarHeight-1));
  vertexIt[1].position[0] -= (float)(titleBarHeight-1u);
  vertexIt += 4;
  const float* backColor = theme.popupControlColor();
  const float backDarkColor[4]{ backColor[0]*0.9f, backColor[1]*0.9f, backColor[2]*0.9f, backColor[3] };
  GeometryGenerator::fillVerticalRectangleVertices(vertexIt, backColor, backDarkColor, // content background
                                                   0.f, (float)width, -(float)titleBarHeight, -(float)(height() - buttonBarHeight));
  vertexIt += 4;
  const float* titleBorderColor = theme.popupBorderColor();
  GeometryGenerator::fillRectangleVertices(vertexIt, titleBorderColor,  // title underline
                                           0.f, (float)width, -(float)(titleBarHeight-1), -(float)titleBarHeight);
  vertexIt += 4;
  GeometryGenerator::fillRectangleVertices(vertexIt, titleBorderColor,  // button bar background
                                           0.f, (float)width, -(float)(height() - buttonBarHeight), -(float)height());
  
  indices = { 0,1,2,2,1,3,  4,5,6,6,5,7,  8,9,10,10,9,11,  12,13,14,14,13,15 };
  controlMesh = ControlMesh(context.renderer(), std::move(vertices), indices, context.pixelSizeX(),
                            context.pixelSizeY(), x, y, width, height());
  
  // create popup title
  titleMesh = TextMesh(context.renderer(), labelFont, title, context.pixelSizeX(), context.pixelSizeY(),
                       x + (int32_t)Control::fieldsetTitleShortPaddingX(), y + (int32_t)Control::fieldsetTitlePaddingY() + 1);
  
  // create popup message
  messageMesh = TextMesh(context.renderer(), messageFont, message, context.pixelSizeX(), context.pixelSizeY(),
                         x + (int32_t)(width >> 1), y + (int32_t)(height() >> 1) - messageFont.XHeight(), TextAlignment::center);
  
  // create buttons
  assert(buttonLabels != nullptr && buttonCount >= (size_t)1);
  const int32_t buttonLabelY = y + (int32_t)height() - (int32_t)((buttonBarHeight + messageFont.XHeight() + 1u) >> 1);
  int32_t buttonX = x + (int32_t)width - (int32_t)BUTTON_OUTER_MARGIN + (int32_t)Control::controlSideMargin()
                  - (int32_t)buttonCount*(int32_t)(BUTTON_WIDTH + Control::controlSideMargin());
  buttons.reserve(buttonCount);
    
  const float buttonColor[4]{ titleBorderColor[0]*1.6f, titleBorderColor[1]*1.6f, titleBorderColor[2]*1.6f, 1.f };
  ButtonStyleProperties buttonStyle(ButtonStyle::fromTopLeft, FontType::inputText, ControlIconType::none,
                                    buttonColor, buttonColor, 0, BUTTON_WIDTH,
                                    Control::buttonPaddingX(), Control::comboBoxPaddingY());
  for (size_t i = 0; i < buttonCount; ++i) {
    buttons.emplace_back(context, buttonLabels[i], buttonX, buttonLabelY, buttonStyle, 0, nullptr);
    buttonX += BUTTON_WIDTH + Control::controlSideMargin();
  }
}

// ---

void Popup::move(RendererContext& context) {
  if (backMesh.width() == 0)
    return;
  std::vector<ControlVertex> vertices = backMesh.relativeVertices();
  GeometryGenerator::resizeRectangleVertices(vertices.data(), (float)context.clientWidth(), -(float)context.clientHeight());
  backMesh.update(context.renderer(), std::move(vertices), context.pixelSizeX(), context.pixelSizeY(),
                  0, 0, context.clientWidth(), context.clientHeight());

  const int32_t x = (int32_t)(context.clientWidth() >> 1) - (int32_t)(controlMesh.width() >> 1);
  const int32_t y = (int32_t)(context.clientHeight() >> 1) - (int32_t)((height()*3u) >> 2);
  const int32_t messageY = y + messageMesh.y() - controlMesh.y();
  const int32_t buttonLabelY = y + buttons[0].y() - controlMesh.y() + Control::comboBoxPaddingY();
  int32_t buttonX = x + buttons[0].x() - controlMesh.x();
  controlMesh.move(context.renderer(), context.pixelSizeX(), context.pixelSizeY(), x, y);

  titleMesh.move(context.renderer(), context.pixelSizeX(), context.pixelSizeY(),
                 x + (int32_t)Control::fieldsetTitleShortPaddingX(), y + (int32_t)Control::fieldsetTitlePaddingY() + 1);

  messageMesh.move(context.renderer(), context.pixelSizeX(), context.pixelSizeY(),
                   x + ((int32_t)controlMesh.width() - (int32_t)messageMesh.width())/2, messageY);

  for (auto& button : buttons) {
    button.move(context, buttonX, buttonLabelY);
    buttonX += BUTTON_WIDTH + Control::controlSideMargin();
  }
}


// -- accessors/operations -- --------------------------------------------------

bool Popup::click(RendererContext&, int32_t mouseX, int32_t mouseY) {
  if (buttons.empty())
    return false;

  if (mouseX >= controlMesh.x() && mouseX < controlMesh.x() + (int32_t)controlMesh.width()
  &&  mouseY >= controlMesh.y() && mouseY < controlMesh.y() + (int32_t)controlMesh.height()) { // inside popup box
    if (mouseY >= buttons[0].y() && mouseY < buttons[0].y() + (int32_t)buttons[0].height()) { // inside button bar
      uint32_t index = 0;
      for (const auto& button : buttons) {
        if (mouseX < button.x() + (int32_t)button.width() && mouseX >= button.x()) {
          actionIndex_ = (int32_t)index;
          if (onAction)
            onAction(index);
          return false;
        }
        ++index;
      }
    }
  }
  else { // click on back hider -> close
    actionIndex_ = (int32_t)(buttons.size() - 1u);
    return false;
  }
  return true;
}

bool Popup::clickSelectedIndex() noexcept {
  if (selectedIndex_ > -1) {
    actionIndex_ = selectedIndex_;
    if (onAction)
      onAction((uint32_t)selectedIndex_);
    return false;
  }
  return !buttons.empty();
}

void Popup::mouseMove(int32_t mouseX, int32_t mouseY) noexcept {
  hoverIndex = -1;
  if (mouseY >= buttons[0].y() && mouseY < buttons[0].y() + (int32_t)buttons[0].height()) {
    uint32_t index = 0;
    for (auto& button : buttons) {
      if (mouseX < button.x() + (int32_t)button.width() && mouseX >= button.x()) {
        hoverIndex = index;
        break;
      }
      ++index;
    }
  }
}


// -- rendering -- -------------------------------------------------------------

void Popup::drawBackground(RendererContext& context, RendererStateBuffers& buffers) {
  auto& renderer = context.renderer();

  buffers.bindControlBuffer(context.renderer(), ControlBufferType::regular);
  backMesh.draw(renderer);
  controlMesh.draw(renderer);

  int32_t index = 0;
  for (auto& button : buttons) {
    button.drawPopupBackground(context, buffers, (hoverIndex == index), (selectedIndex_ == index));
    ++index;
  }
}

void Popup::drawLabels(RendererContext& context, RendererStateBuffers& buffers) {
  auto& renderer = context.renderer();

  buffers.bindLabelBuffer(context.renderer(), LabelBufferType::fieldset);
  titleMesh.draw(renderer);

  buffers.bindLabelBuffer(context.renderer(), LabelBufferType::regular);
  messageMesh.draw(renderer);

  int32_t index = 0;
  for (auto& button : buttons) {
    button.drawLabel(context, buffers, (hoverIndex == index));
    ++index;
  }
}
