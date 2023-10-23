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


// -- init/resize geometry -- --------------------------------------------------

Fieldset::Fieldset(RendererContext& context, const char32_t* label, FieldsetStyle style_,
                   const float color[4], int32_t x, int32_t labelY, uint32_t paddingX,
                   uint32_t paddingY, uint32_t width, uint32_t contentHeight)
  : style(style_) {
  // create fieldset decoration
  auto& labelFont = context.getFont(FontType::labels);
  const int32_t y = labelY - (int32_t)paddingY;
  const uint32_t barHeight = labelFont.XHeight() + (paddingY << 1);
  const uint32_t totalHeight = barHeight + contentHeight;

  std::vector<ControlVertex> vertices;
  std::vector<uint32_t> indices;
  switch (style) {
    // title -> underline + vertical line
    case FieldsetStyle::title: {
      vertices.resize(8);
      GeometryGenerator::fillRectangleVertices(vertices.data(), color,      // title underline
                                               0.f, (float)width, -(float)(barHeight-1), -(float)barHeight);
      GeometryGenerator::fillRectangleVertices(vertices.data() + 4, color,  // vertical line
                                               0.f, 1.f, -(float)barHeight, -(float)totalHeight);
      indices = { 0,1,2,2,1,3,  4,5,6,6,5,7 };
      paddingX = 0; // remove padding for label
      break;
    }
    // classic -> title bar + contour
    case FieldsetStyle::classic:
    default: {
      vertices.resize(20);
      ControlVertex* vertexIt = vertices.data();

      const float backColor[4]{ color[0], color[1], color[2], 0.3f*color[3] };
      GeometryGenerator::fillRectangleVertices(vertexIt, backColor,  // title bar background
                                               1.f, (float)(width-1), -1.f, -(float)(barHeight-1));
      vertexIt += 4;
      GeometryGenerator::fillRectangleVertices(vertexIt, color, 0.f, (float)width, 0.f, -1.f); // contour top
      vertexIt += 4;
      GeometryGenerator::fillRectangleVertices(vertexIt, color, 0.f, (float)width, -(float)(totalHeight-1), -(float)totalHeight); // bottom
      vertexIt += 4;
      GeometryGenerator::fillRectangleVertices(vertexIt, color, 0.f, 1.f, -1.f, -(float)(totalHeight-1)); // contour left
      vertexIt += 4;
      GeometryGenerator::fillRectangleVertices(vertexIt, color, (float)(width-1), (float)width, -1.f, -(float)(totalHeight-1)); // right
      indices = { 0,1,2, 2,1,3,  4,5,6, 6,5,7,  8,9,10, 10,9,11,  12,13,14, 14,13,15,  16,17,18, 18,17,19 };
      break;
    }
  }
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
  const int32_t paddingY = labelMesh.y() - controlMesh.y();
  const uint32_t totalHeight = labelMesh.height() + ((uint32_t)paddingY << 1) + contentHeight;
  
  std::vector<ControlVertex> vertices = controlMesh.relativeVertices();
  switch (style) {
    case FieldsetStyle::title: {
      GeometryGenerator::resizeRectangleVerticesX(vertices.data(), (float)width);            // title underline
      GeometryGenerator::resizeRectangleVerticesY(vertices.data() + 4, -(float)totalHeight); // vertical line
      break;
    }
    case FieldsetStyle::classic:
    default: {
      ControlVertex* vertexIt = vertices.data();
      GeometryGenerator::resizeRectangleVerticesX(vertexIt, (float)(width-1)); // title bar background
      vertexIt += 4;
      GeometryGenerator::resizeRectangleVerticesX(vertexIt, (float)width);     // contour top
      vertexIt += 4;
      GeometryGenerator::resizeRectangleVerticesX(vertexIt, (float)width);     // contour bottom
      GeometryGenerator::moveRectangleVerticesY(vertexIt, -(float)(totalHeight-1), -(float)totalHeight);
      vertexIt += 4;
      GeometryGenerator::resizeRectangleVerticesY(vertexIt, -(float)(totalHeight-1)); // contour left
      vertexIt += 4;
      GeometryGenerator::moveRectangleVerticesX(vertexIt, (float)(width-1), (float)width); // contour right
      GeometryGenerator::resizeRectangleVerticesY(vertexIt, -(float)(totalHeight-1));
      break;
    }
  }
  controlMesh.update(context.renderer(), std::move(vertices), context.pixelSizeX(), context.pixelSizeY(),
                     x, labelY - paddingY, width, totalHeight);
  
  labelMesh.move(context.renderer(), context.pixelSizeX(), context.pixelSizeY(), x + paddingX, labelY);
}
