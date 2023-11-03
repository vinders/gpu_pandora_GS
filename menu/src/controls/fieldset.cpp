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
#include "menu/controls/geometry_generator.h"
#include "menu/controls/fieldset.h"

using namespace display;
using namespace display::controls;
using namespace menu::controls;
using namespace menu;


// -- init/resize geometry -- --------------------------------------------------

Fieldset::Fieldset(RendererContext& context, const char32_t* label, FieldsetStyle style_,
                   const float color[4], int32_t x, int32_t labelY, uint32_t width, uint32_t contentHeight)
  : style(style_) {
  // create fieldset decoration
  auto& labelFont = context.getFont(FontType::labels);
  const int32_t y = labelY - (int32_t)Control::fieldsetTitlePaddingY();
  const uint32_t barHeight = labelFont.XHeight() + (Control::fieldsetTitlePaddingY() << 1);
  const uint32_t totalHeight = barHeight + contentHeight;
  ++labelY;

  uint32_t paddingX;
  std::vector<ControlVertex> vertices;
  std::vector<uint32_t> indices;
  float backColor[4]{ color[0], color[1], color[2], 0.3f*color[3] };

  paddingX = Control::fieldsetTitleShortPaddingX();
  float transparentColor[4]{ color[0], color[1], color[2], 0.f };

  ControlVertex* vertexIt;
  if (style == FieldsetStyle::classic) {
    indices = { 0,1,2,2,1,3,  4,5,6,6,5,7,  8,9,10,10,9,11,  12,13,14,14,13,15,  16,17,18,18,17,19 };
    vertices.resize(20);
    vertexIt = vertices.data();

    GeometryGenerator::fillHorizontalRectangleVertices(vertexIt, backColor, transparentColor, // title bar background
                                                        0.f, (float)width, -1.f, -(float)(barHeight-1));
  }
  else { // FieldsetStyle::gradientBox -> add content background
    transparentColor[3] = 0.1f*color[3];
    indices = { 0,1,2,2,1,3,  4,5,6,6,5,7,  8,9,10,10,9,11,  12,13,14,14,13,15,  16,17,18,18,17,19,  20,21,22,22,21,23 };
    vertices.resize(24);
    vertexIt = vertices.data();

    GeometryGenerator::fillHorizontalRectangleVertices(vertexIt, backColor, transparentColor, // title bar background
                                                        0.f, (float)width, -1.f, -(float)(barHeight-1));
    vertexIt[1].position[0] -= (float)(barHeight-1u);
    vertexIt += 4;
    backColor[3] *= 0.25f;
    GeometryGenerator::fillRectangleVertices(vertexIt, backColor,  // content background
                                              1.f, (float)width, -(float)barHeight, -(float)totalHeight);
  }
  vertexIt += 4;
  const uint32_t solidWidth = (width << 1) / 3u;
  GeometryGenerator::fillRectangleVertices(vertexIt, color,  // title underline
                                            0.f, (float)solidWidth, -(float)(barHeight-1), -(float)barHeight);
  vertexIt += 4;
  GeometryGenerator::fillHorizontalRectangleVertices(vertexIt, color, transparentColor,  // title underline gradient
                                                      (float)solidWidth, (float)width, -(float)(barHeight-1), -(float)barHeight);
  vertexIt += 4;
  GeometryGenerator::fillVerticalRectangleVertices(vertexIt, transparentColor, color, // vertical line top
                                                    0.f, 1.f, -1.f, -(float)barHeight);
  vertexIt += 4;
  GeometryGenerator::fillVerticalRectangleVertices(vertexIt, color, transparentColor, // vertical line gradient
                                                    0.f, 1.f, -(float)barHeight, -(float)totalHeight);
  controlMesh = ControlMesh(context.renderer(), std::move(vertices), indices, context.pixelSizeX(),
                            context.pixelSizeY(), x, y, width, totalHeight);
  
  // create fieldset title
  labelMesh = TextMesh(context.renderer(), labelFont, label, context.pixelSizeX(), context.pixelSizeY(),
                       x + (int32_t)paddingX, labelY);
}

// ---

void Fieldset::move(RendererContext& context, int32_t x, int32_t labelY, uint32_t width, uint32_t contentHeight) {
  if (controlMesh.width() == 0)
    return;
  const int32_t paddingX = labelMesh.x() - controlMesh.x();
  uint32_t totalHeight = labelMesh.height() + ((uint32_t)Control::fieldsetTitlePaddingY() << 1) + contentHeight;
  ++labelY;
  
  std::vector<ControlVertex> vertices = controlMesh.relativeVertices();
  ControlVertex* vertexIt = vertices.data();

  if (style == FieldsetStyle::gradientBox) {
    GeometryGenerator::resizeRectangleVerticesX(vertexIt, (float)width);        // title bar background
    (vertexIt + 1)->position[0] = (float)width + (vertexIt + 2)->position[1];
    (vertexIt + 3)->position[0] = (float)width;
    vertexIt += 4;
  }
  GeometryGenerator::resizeRectangleVerticesX(vertexIt, (float)width);        // title bar background (gradient) -or- content background (gradientBox)
  vertexIt += 4;
  const uint32_t solidWidth = (width << 1) / 3u;
  GeometryGenerator::resizeRectangleVerticesX(vertexIt, (float)solidWidth);   // title underline
  vertexIt += 4;
  GeometryGenerator::moveRectangleVerticesX(vertexIt, (float)solidWidth, (float)width); // title underline gradient
  vertexIt += 8;
  GeometryGenerator::resizeRectangleVerticesY(vertexIt, -(float)totalHeight); // vertical line

  controlMesh.update(context.renderer(), std::move(vertices), context.pixelSizeX(), context.pixelSizeY(),
                     x, labelY - Control::fieldsetTitlePaddingY() - 1, width, totalHeight);
  
  labelMesh.move(context.renderer(), context.pixelSizeX(), context.pixelSizeY(), x + paddingX, labelY);
}
