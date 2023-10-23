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
#include "menu/controls/scroll_bar.h"

using namespace video_api;
using namespace display;
using namespace display::controls;
using namespace menu::controls;


// -- init/resize geometry -- --------------------------------------------------

void ScrollBar::init(RendererContext& context, const float barColor[4], const float thumbColor[4],
                     int32_t x, int32_t y, uint32_t width, uint32_t height) {
  // create background
  maxTopPosition = (visibleScrollArea < totalScrollArea) ? (totalScrollArea - visibleScrollArea) : 0;
  thumbAreaY = y + (int32_t)width; // width also used as up/down box height
  thumbAreaHeight = height - (width << 1);

  std::vector<ControlVertex> vertices(static_cast<size_t>(4u));
  GeometryGenerator::fillRectangleVertices(vertices.data(), barColor, 0.f, (float)width, 0.f, -(float)height);
  std::vector<uint32_t> indices{ 0,1,2, 2,1,3 };
  backMesh = ControlMesh(context.renderer(), std::move(vertices), indices,
                         context.pixelSizeX(), context.pixelSizeY(), x, y, width, height);

  // create moving thumb
  const uint32_t thumbHeight = (visibleScrollArea < totalScrollArea)
                             ? (visibleScrollArea * thumbAreaHeight / totalScrollArea)
                             : thumbAreaHeight;
  vertices = std::vector<ControlVertex>(static_cast<size_t>(4u));
  GeometryGenerator::fillRectangleVertices(vertices.data(), thumbColor, 0.f, (float)width, 0.f, -(float)thumbHeight);
  thumbMesh = ControlMesh(context.renderer(), std::move(vertices), indices,
                          context.pixelSizeX(), context.pixelSizeY(), x, thumbAreaY, width, thumbHeight);

  // create up/down arrows
  const float arrowColorMultiplier = (thumbColor[0]+thumbColor[1]+thumbColor[2] >= 1.5f) ? 0.65f : 1.5f;
  const float arrowColor[4]{ thumbColor[0]*arrowColorMultiplier, thumbColor[1]*arrowColorMultiplier,
                             thumbColor[2]*arrowColorMultiplier, thumbColor[3] };
  const uint32_t arrowPaddingX = (width >> 2);
  const uint32_t arrowPaddingY = arrowPaddingX + 1;
  indices.resize(3);

  vertices = std::vector<ControlVertex>(static_cast<size_t>(5u)); // arrow UP
  GeometryGenerator::fillTriangleVertices(vertices.data(), arrowColor, (float)arrowPaddingX, -(float)arrowPaddingY,
                                          (float)(width - (arrowPaddingX << 1)), (float)(width - (arrowPaddingY << 1) - 1u));
  upMesh = ControlMesh(context.renderer(), std::move(vertices), indices,
                       context.pixelSizeX(), context.pixelSizeY(), x, y, width, width);

  vertices = std::vector<ControlVertex>(static_cast<size_t>(5u)); // arrow DOWN
  GeometryGenerator::fillInvertedTriangleVertices(vertices.data(), arrowColor, (float)arrowPaddingX, -(float)(arrowPaddingY + 1u),
                                                  (float)(width - (arrowPaddingX << 1)), (float)(width - (arrowPaddingY << 1) - 1u));
  downMesh = ControlMesh(context.renderer(), std::move(vertices), indices, context.pixelSizeX(), context.pixelSizeY(),
                         x, thumbAreaY + (int32_t)thumbAreaHeight, width, width);
}

// ---

void ScrollBar::move(RendererContext& context, int32_t x, int32_t y, uint32_t height,
                     uint32_t screenHeightPx, uint32_t totalScrollAreaPx) {
  visibleScrollArea = screenHeightPx;
  totalScrollArea = totalScrollAreaPx;
  maxTopPosition = (visibleScrollArea < totalScrollArea) ? (totalScrollArea - visibleScrollArea) : 0;
  if (topPosition > maxTopPosition)
    topPosition = maxTopPosition;

  thumbAreaY = y + (int32_t)backMesh.width(); // width also used as up/down box height
  thumbAreaHeight = height - (backMesh.width() << 1);
  dragThumbOffsetY = noDrag();

  backMesh.move(context.renderer(), context.pixelSizeX(), context.pixelSizeY(), x, y);

  const int32_t thumbTop = thumbAreaY + (int32_t)(topPosition * thumbAreaHeight / totalScrollArea);
  const uint32_t thumbHeight = (visibleScrollArea < totalScrollArea)
                             ? (visibleScrollArea * thumbAreaHeight / totalScrollArea)
                             : thumbAreaHeight;
  const float* thumbColor = thumbMesh.relativeVertices()[0].color;
  std::vector<ControlVertex> vertices(static_cast<size_t>(4u));
  GeometryGenerator::fillRectangleVertices(vertices.data(), thumbColor, 0.f, (float)backMesh.width(), 0.f, -(float)thumbHeight);
  thumbMesh.update(context.renderer(), std::move(vertices), context.pixelSizeX(), context.pixelSizeY(),
                   x, thumbAreaY + thumbTop, backMesh.width(), thumbHeight);

  upMesh.move(context.renderer(), context.pixelSizeX(), context.pixelSizeY(), x, y);
  downMesh.move(context.renderer(), context.pixelSizeX(), context.pixelSizeY(), x, thumbAreaY + (int32_t)thumbAreaHeight);

  onChange(topPosition);
}

void ScrollBar::updateThumbPosition(RendererContext& context, uint32_t top) {
  if (top > maxTopPosition)
    top = maxTopPosition;

  if (topPosition != top) {
    topPosition = top;
    const int32_t thumbTop = thumbAreaY + (int32_t)(top * thumbAreaHeight / totalScrollArea);
    thumbMesh.move(context.renderer(), context.pixelSizeX(), context.pixelSizeY(), thumbMesh.x(), thumbTop);

    onChange(topPosition);
  }
}


// -- operations -- ------------------------------------------------------------

void ScrollBar::mouseMove(RendererContext& context, int32_t mouseY) {
  if (isDragged()) {
    int32_t barOffset = (mouseY - (int32_t)dragThumbOffsetY) - thumbAreaY;
    updateThumbPosition(context, (barOffset > 0) ? ((uint32_t)barOffset * totalScrollArea / thumbAreaHeight) : 0);
  }
}

// ---

void ScrollBar::click(RendererContext& context, int32_t mouseY, bool isMouseDown) {
  if (mouseY < thumbAreaY) { // UP button
    updateThumbPosition(context, (topPosition > scrollStep) ? topPosition - scrollStep : 0);
  }
  else if (mouseY >= downMesh.y()) { // DOWN button
    updateThumbPosition(context, (topPosition + scrollStep < maxTopPosition) ? topPosition + scrollStep : maxTopPosition);
  }
  else if (mouseY >= thumbMesh.y() && mouseY < thumbMesh.y() + (int32_t)thumbMesh.height()) { // thumb bar
    dragThumbOffsetY = isMouseDown ? mouseY - thumbMesh.y() : noDrag();
  }
  else { // background
    const float targetCenter = (float)totalScrollArea * (static_cast<float>(mouseY - thumbAreaY) / (float)thumbAreaHeight);
    int32_t top = (int32_t)targetCenter - (int32_t)(visibleScrollArea >> 1);

    const uint32_t minTop = (topPosition > visibleScrollArea) ? topPosition - visibleScrollArea : 0;
    if (top < (int32_t)minTop) // max offset = current position +- visible area
      top = (int32_t)minTop;
    else if ((uint32_t)top > topPosition + visibleScrollArea)
      top = (int32_t)(topPosition + visibleScrollArea);
    updateThumbPosition(context, (uint32_t)top);
  }
}


// -- rendering -- -------------------------------------------------------------

bool ScrollBar::drawControl(RendererContext& context, int32_t mouseX, int32_t mouseY,
                            Buffer<ResourceUsage::staticGpu>& hoverPressedVertexUniform) {
  if (isEnabled()) {
    backMesh.draw(context.renderer());

    if (isHover(mouseX, mouseY)) {
      if (mouseY < thumbAreaY) { // UP hover/pressed
        thumbMesh.draw(context.renderer());
        downMesh.draw(context.renderer());
        
        context.renderer().bindVertexUniforms(0, hoverPressedVertexUniform.handlePtr(), 1);
        upMesh.draw(context.renderer());
        return true;
      }
      else if (mouseY >= downMesh.y()) { // DOWN hover/pressed
        thumbMesh.draw(context.renderer());
        upMesh.draw(context.renderer());
        
        context.renderer().bindVertexUniforms(0, hoverPressedVertexUniform.handlePtr(), 1);
        downMesh.draw(context.renderer());
        return true;
      }
      else if (mouseY >= thumbMesh.y() && mouseY < thumbMesh.y() + (int32_t)thumbMesh.height()) { // thumb hover/pressed
        upMesh.draw(context.renderer());
        downMesh.draw(context.renderer());

        context.renderer().bindVertexUniforms(0, hoverPressedVertexUniform.handlePtr(), 1);
        thumbMesh.draw(context.renderer());
        return true;
      }
    }
    thumbMesh.draw(context.renderer());
    upMesh.draw(context.renderer());
    downMesh.draw(context.renderer());
  }
  return false;
}
