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
#include "menu/controls/fieldset.h"

using namespace display;
using namespace display::controls;
using namespace menu::controls;

static inline void setControlVertex(ControlVertex& outVertex, const float rgba[4], float x, float y) {
  float* position = outVertex.position;
  *position = x;
  *(++position) = y;
  *(++position) = 0.f; // z
  *(++position) = 1.f; // w
  memcpy(outVertex.color, rgba, 4*sizeof(float));
}

// ---

Fieldset::Fieldset(RendererContext& context, const char32_t* label, FieldsetStyle style_,
                   const float color[4], int32_t x, int32_t labelY, uint32_t paddingX,
                   uint32_t paddingY, uint32_t width, uint32_t contentHeight)
  : style(style_) {
  auto& labelFont = context.getFont(FontType::labels);
  
  // create decoration
  const int32_t y = labelY - (int32_t)paddingY;
  const uint32_t barHeight = labelFont.XHeight() + (paddingY << 1);
  const uint32_t totalHeight = barHeight + contentHeight;
  std::vector<ControlVertex> vertices;
  std::vector<uint32_t> indices;
  
  if (style == FieldsetStyle::title) { // title -> underline + vertical line
    vertices.resize(8);
    ControlVertex* vertexIt = vertices.data();
    setControlVertex(*vertexIt,     color, 0.f,          -(float)(barHeight - 1)); // underline
    setControlVertex(*(++vertexIt), color, (float)width, -(float)(barHeight - 1));
    setControlVertex(*(++vertexIt), color, 0.f,          -(float)barHeight);
    setControlVertex(*(++vertexIt), color, (float)width, -(float)barHeight);
    setControlVertex(*(++vertexIt), color, 0.f, -(float)barHeight); // vertical line
    setControlVertex(*(++vertexIt), color, 1.f, -(float)barHeight);
    setControlVertex(*(++vertexIt), color, 0.f, -(float)totalHeight);
    setControlVertex(*(++vertexIt), color, 1.f, -(float)totalHeight);
    indices = { 0,1,2, 2,1,3,  4,5,6, 6,5,7 };
    paddingX = 0;
  }
  else { // classic -> title bar + contour
    const float backColor[4]{ color[0], color[1], color[2], 0.3f*color[3] };
    vertices.resize(20);
    ControlVertex* vertexIt = vertices.data();
    setControlVertex(*vertexIt,     backColor, 1.f,                -1.f); // bar background
    setControlVertex(*(++vertexIt), backColor, (float)(width - 1), -1.f);
    setControlVertex(*(++vertexIt), backColor, 1.f,                -(float)(barHeight - 1));
    setControlVertex(*(++vertexIt), backColor, (float)(width - 1), -(float)(barHeight - 1));
    setControlVertex(*(++vertexIt), color, 0.f,          0.f); // contour top
    setControlVertex(*(++vertexIt), color, (float)width, 0.f);
    setControlVertex(*(++vertexIt), color, 0.f,          -1.f);
    setControlVertex(*(++vertexIt), color, (float)width, -1.f);
    setControlVertex(*(++vertexIt), color, 0.f,          -(float)(totalHeight - 1)); // contour bottom
    setControlVertex(*(++vertexIt), color, (float)width, -(float)(totalHeight - 1));
    setControlVertex(*(++vertexIt), color, 0.f,          -(float)totalHeight);
    setControlVertex(*(++vertexIt), color, (float)width, -(float)totalHeight);
    setControlVertex(*(++vertexIt), color, 0.f,                -1.f); // contour left
    setControlVertex(*(++vertexIt), color, 1.f,                -1.f);
    setControlVertex(*(++vertexIt), color, 0.f,                -(float)(totalHeight - 1));
    setControlVertex(*(++vertexIt), color, 1.f,                -(float)(totalHeight - 1));
    setControlVertex(*(++vertexIt), color, (float)(width - 1), -1.f); // contour right
    setControlVertex(*(++vertexIt), color, (float)width,       -1.f);
    setControlVertex(*(++vertexIt), color, (float)(width - 1), -(float)(totalHeight - 1));
    setControlVertex(*(++vertexIt), color, (float)width,       -(float)(totalHeight - 1));
    indices = { 0,1,2, 2,1,3,  4,5,6, 6,5,7,  8,9,10, 10,9,11,  12,13,14, 14,13,15,  16,17,18, 18,17,19 };
  }
  controlMesh = ControlMesh(*context.renderer, std::move(vertices), indices, context.pixelSizeX, context.pixelSizeY,
                            x, y, width, totalHeight);
  
  // create label
  labelMesh = TextMesh(*context.renderer, labelFont, label, context.pixelSizeX, context.pixelSizeY,
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
  if (style == FieldsetStyle::title) {
    vertices[1].position[0] = (float)width;
    vertices[3].position[0] = (float)width;
    vertices[6].position[1] = -(float)totalHeight;
    vertices[7].position[1] = -(float)totalHeight;
  }
  else {
    vertices[1].position[0] = (float)(width - 1);
    vertices[3].position[0] = (float)(width - 1);
    vertices[5].position[0] = (float)width;
    vertices[7].position[0] = (float)width;
    vertices[8].position[1] = -(float)(totalHeight - 1);
    vertices[9].position[0] = (float)width;
    vertices[9].position[1] = -(float)(totalHeight - 1);
    vertices[10].position[1] = -(float)totalHeight;
    vertices[11].position[0] = (float)width;
    vertices[11].position[1] = -(float)totalHeight;
    vertices[14].position[1] = -(float)(totalHeight - 1);
    vertices[15].position[1] = -(float)(totalHeight - 1);
    vertices[16].position[0] = (float)(width - 1);
    vertices[17].position[0] = (float)width;
    vertices[18].position[0] = (float)(width - 1);
    vertices[18].position[1] = -(float)(totalHeight - 1);
    vertices[19].position[0] = (float)width;
    vertices[19].position[1] = -(float)(totalHeight - 1);
  }
  controlMesh.update(*context.renderer, std::move(vertices), context.pixelSizeX, context.pixelSizeY,
                     x, labelY - paddingY, width, totalHeight);
  
  labelMesh.move(*context.renderer, context.pixelSizeX, context.pixelSizeY, x + paddingX, labelY);
}
