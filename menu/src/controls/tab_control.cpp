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
#include "menu/controls/tab_control.h"

using namespace video_api;
using namespace display;
using namespace display::controls;
using namespace menu::controls;
using namespace menu;


// -- init/resize geometry -- --------------------------------------------------

static std::vector<ControlVertex> generateTabBarGeometry(uint32_t barWidth, uint32_t tabHeight, uint32_t gradientWidth,
                                                         const float* tabsColor, const float barColor[4],
                                                         std::vector<uint32_t>* outIndices) {
  std::vector<ControlVertex> vertices;
  ControlVertex* vertexIt;
  
  if (tabsColor) { // tabs background
    vertices.resize(28);
    vertexIt = vertices.data();
    GeometryGenerator::fillRectangleVertices(vertexIt, tabsColor, 0.f, (float)barWidth, 0.f, -(float)(tabHeight + 1u));
    vertexIt += 4;
  }
  else { // active tab bar
    vertices.resize(27);
    vertexIt = vertices.data();
  }

  const float borderColor[4]{ barColor[0]*0.75f, barColor[1]*0.75f, barColor[2]*0.75f, barColor[3] };
  const float borderColorTransparent[4]{ borderColor[0], borderColor[1], borderColor[2], 0.f };
  GeometryGenerator::fillRectangleVertices(vertexIt, borderColor, (float)gradientWidth, (float)(barWidth - gradientWidth),
                                           -(float)tabHeight, -(float)(tabHeight + 3u));  // tab bar border
  vertexIt += 4;
  GeometryGenerator::fillHorizontalRectangleVertices(vertexIt, borderColorTransparent, borderColor, // left gradient border
                                                     0.f, (float)gradientWidth, -(float)tabHeight, -(float)(tabHeight + 3u));
  vertexIt += 4;
  GeometryGenerator::fillHorizontalRectangleVertices(vertexIt, borderColor, borderColorTransparent, // right gradient border
                                                     (float)(barWidth - gradientWidth), (float)barWidth,
                                                     -(float)tabHeight, -(float)(tabHeight + 3u));
  vertexIt += 4;

  const float barColorTransparent[4]{ barColor[0], barColor[1], barColor[2], 0.f };
  GeometryGenerator::fillRectangleVertices(vertexIt, barColor, (float)gradientWidth, (float)(barWidth - gradientWidth),
                                           -(float)(tabHeight + 1u), -(float)(tabHeight + 2u));  // tab bar center
  vertexIt += 4;
  GeometryGenerator::fillHorizontalRectangleVertices(vertexIt, barColorTransparent, barColor, // left gradient center
                                                     0.f, (float)gradientWidth, -(float)(tabHeight + 1u), -(float)(tabHeight + 2u));
  vertexIt += 4;
  GeometryGenerator::fillHorizontalRectangleVertices(vertexIt, barColor, barColorTransparent, // right gradient center
                                                     (float)(barWidth - gradientWidth), (float)barWidth,
                                                     -(float)(tabHeight + 1u), -(float)(tabHeight + 2u));

  if (!tabsColor) { // active tab bar -> add arrow
    barWidth >>= 1; // compute bar center
    vertexIt += 4;
    GeometryGenerator::fillControlVertex(*vertexIt,     barColor, (float)barWidth,        -(float)(tabHeight - 12u));
    GeometryGenerator::fillControlVertex(*(++vertexIt), barColor, (float)(barWidth + 6u), -(float)(tabHeight + 1u));
    GeometryGenerator::fillControlVertex(*(++vertexIt), barColor, (float)(barWidth - 6u), -(float)(tabHeight + 1u));

    if (outIndices)
      *outIndices = std::vector<uint32_t>{ 0,1,2, 2,1,3,  4,5,6, 6,5,7,  8,9,10, 10,9,11,  12,13,14, 14,13,15,
                                          16,17,18, 18,17,19,  20,21,22, 22,21,23,  24,25,26 };
  }
  else if (outIndices)
    *outIndices = std::vector<uint32_t>{ 0,1,2, 2,1,3,  4,5,6, 6,5,7,  8,9,10, 10,9,11,  12,13,14, 14,13,15,
                                        16,17,18, 18,17,19,  20,21,22, 22,21,23,  24,25,26, 26,25,27 };
  return vertices;
}

// ---

void TabControl::init(RendererContext& context, int32_t x, int32_t y, uint32_t barWidth, const float tabsColor[4],
                      const float barColor[4], const float activeBarColor[4], const char32_t** tabLabels, size_t tabCount) {
  auto& font = context.getFont(FontType::labels);
  uint32_t tabHeight = font.XHeight() + (paddingY << 1);

  // tab labels
  const int32_t tabY = y + paddingY;
  int32_t tabX = x + paddingX;
  tabLabelMeshes.reserve(tabCount);
  while (tabCount) {
#   if !defined(_CPP_REVISION) || _CPP_REVISION != 14
    const auto& mesh = tabLabelMeshes.emplace_back(context.renderer(), font, *tabLabels,
                                                   context.pixelSizeX(), context.pixelSizeY(), tabX, tabY);
#   else
    tabLabelMeshes.emplace_back(*context.renderer, font, *tabLabels, context.pixelSizeX, context.pixelSizeY, tabX, tabY);
    const auto& mesh = tabLabelMeshes.back();
#   endif
    tabX += (mesh.width() >= minTabWidth) ? mesh.width() : minTabWidth;
    tabX += (paddingX << 1);

    ++tabLabels;
    --tabCount;
  }
  
  // center labels
  const int32_t offsetTabsX = ((int32_t)barWidth - (tabX - (int32_t)paddingX)) >> 1;
  for (auto& mesh : tabLabelMeshes) {
    uint32_t tabLabelX = mesh.x() + offsetTabsX;
    if (mesh.width() < minTabWidth)
      tabLabelX += (int32_t)((minTabWidth - mesh.width()) >> 1);
    mesh.move(context.renderer(), context.pixelSizeX(), context.pixelSizeY(), tabLabelX, mesh.y());
  }

  // tab bar
  std::vector<uint32_t> indices;
  auto vertices = generateTabBarGeometry(barWidth, tabHeight, (barWidth >= 800u) ? 200u : (barWidth >> 2),
                                         tabsColor, barColor, &indices);
  barMesh = ControlMesh(context.renderer(), std::move(vertices), indices, context.pixelSizeX(), context.pixelSizeY(),
                        x, y, barWidth, tabHeight + 3u);

  vertices = generateTabBarGeometry(minTabWidth + (paddingX << 1), tabHeight, paddingX, nullptr, activeBarColor, &indices);
  const auto& activeTabLabel = tabLabelMeshes[selectedIndex];
  activeBarMesh = ControlMesh(context.renderer(), std::move(vertices), indices, context.pixelSizeX(), context.pixelSizeY(),
                              activeTabLabel.x() + (int32_t)(activeTabLabel.width() >> 1), y, barWidth, tabHeight + 3u);
}

// ---

void TabControl::move(RendererContext& context, int32_t x, int32_t y, uint32_t barWidth) {
  if (!tabLabelMeshes.empty()) {
    const auto& lastLabel = tabLabelMeshes.back();
    const int32_t tabY = y + paddingY;
    int32_t tabX = ((barWidth - (lastLabel.x() + (int32_t)lastLabel.width() - tabLabelMeshes[0].x())) >> 1) + paddingX;
    for (auto& mesh : tabLabelMeshes) {
      if (mesh.width() >= minTabWidth) {
        mesh.move(context.renderer(), context.pixelSizeX(), context.pixelSizeY(), tabX, tabY);
        tabX += mesh.width();
      }
      else {
        tabX += (minTabWidth >> 1);
        mesh.move(context.renderer(), context.pixelSizeX(), context.pixelSizeY(), tabX, tabY);
        tabX += (minTabWidth >> 1);
      }
      tabX += (paddingX << 1);
    }
  }

  if (!barMesh.relativeVertices().empty()) {
    const uint32_t tabHeight = tabLabelMeshes[0].height() + (paddingY << 1);
    const float* tabsColor = barMesh.relativeVertices()[0].color;
    const float* barColor = barMesh.relativeVertices()[barMesh.relativeVertices().size() - 2u].color;
    auto vertices = generateTabBarGeometry(barWidth, tabHeight, (barWidth >= 800u) ? 200u : (barWidth >> 2),
                                           tabsColor, barColor, nullptr);
    barMesh.update(context.renderer(), std::move(vertices), context.pixelSizeX(), context.pixelSizeY(),
                   x, y, barWidth, barMesh.height());

    const auto& activeTabLabel = tabLabelMeshes[selectedIndex];
    activeBarMesh.move(context.renderer(), context.pixelSizeX(), context.pixelSizeY(),
                       activeTabLabel.x() + (int32_t)(activeTabLabel.width() >> 1), y);
  }
}


// -- operations -- ------------------------------------------------------------

void TabControl::click(RendererContext& context, int32_t mouseX) {
  int32_t currentIndex = -1;
  for (const auto& mesh : tabLabelMeshes) {
    if (mouseX < mesh.x() - (int32_t)paddingX)
      break;
    ++currentIndex;
  }

  if (currentIndex == -1)
    return;
  if (currentIndex + 1 >= (int32_t)tabLabelMeshes.size()) {
    const auto& lastTabLabel = tabLabelMeshes.back();
    if (mouseX >= lastTabLabel.x() + (int32_t)lastTabLabel.width() + (int32_t)paddingX)
      return;
  }
  selectedIndex = (uint32_t)currentIndex;

  const auto& activeTabLabel = tabLabelMeshes[selectedIndex];
  activeBarMesh.move(context.renderer(), context.pixelSizeX(), context.pixelSizeY(),
                     activeTabLabel.x() + (int32_t)(activeTabLabel.width() >> 1), activeBarMesh.y());
}

// ---

void TabControl::selectPrevious(RendererContext& context) {
  if (selectedIndex != 0)
    --selectedIndex;
  else
    selectedIndex = (uint32_t)tabLabelMeshes.size() - 1u;

  const auto& activeTabLabel = tabLabelMeshes[selectedIndex];
  activeBarMesh.move(context.renderer(), context.pixelSizeX(), context.pixelSizeY(),
                     activeTabLabel.x() + (int32_t)(activeTabLabel.width() >> 1), activeBarMesh.y());
}

void TabControl::selectNext(RendererContext& context) {
  if (selectedIndex + 1u < (uint32_t)tabLabelMeshes.size())
    ++selectedIndex;
  else
    selectedIndex = 0;

  const auto& activeTabLabel = tabLabelMeshes[selectedIndex];
  activeBarMesh.move(context.renderer(), context.pixelSizeX(), context.pixelSizeY(),
                     activeTabLabel.x() + (int32_t)(activeTabLabel.width() >> 1), activeBarMesh.y());
}

void TabControl::selectIndex(RendererContext& context, uint32_t index) {
  if (index < (uint32_t)tabLabelMeshes.size()) {
    this->selectedIndex = index;

    const auto& activeTabLabel = tabLabelMeshes[selectedIndex];
    activeBarMesh.move(context.renderer(), context.pixelSizeX(), context.pixelSizeY(),
                       activeTabLabel.x() + (int32_t)(activeTabLabel.width() >> 1), activeBarMesh.y());
  }
}


// -- rendering -- -------------------------------------------------------------

void TabControl::drawLabels(RendererContext& context, int32_t mouseX, int32_t mouseY, RendererStateBuffers& buffers) {
  uint32_t currentIndex = 0;
  int32_t hoverIndex = -1;
  buffers.bindLabelBuffer(context.renderer(), LabelBufferType::tab);
  if (mouseY >= barMesh.y() && mouseY < barMesh.y() + (int32_t)barMesh.width()) {
    for (auto& mesh : tabLabelMeshes) {
      if (mouseX >= mesh.x() - (int32_t)paddingX && mouseX < mesh.x() + (int32_t)mesh.width() + (int32_t)paddingX) {
        hoverIndex = (int32_t)currentIndex;
      }
      else if (currentIndex != selectedIndex)
        mesh.draw(context.renderer());
      ++currentIndex;
    }
  }
  else {
    for (auto& mesh : tabLabelMeshes) {
      if (currentIndex != selectedIndex)
        mesh.draw(context.renderer());
      ++currentIndex;
    }
  }

  buffers.bindLabelBuffer(context.renderer(), LabelBufferType::tabActive);
  tabLabelMeshes[selectedIndex].draw(context.renderer());
  if (hoverIndex >= 0 && hoverIndex != (int32_t)selectedIndex)
    tabLabelMeshes[hoverIndex].draw(context.renderer());
}


      