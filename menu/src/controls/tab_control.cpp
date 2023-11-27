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

static constexpr const uint32_t maxTabWidth = 212u;
static constexpr const float cornerColorFactor = 1.25f;

void TabControl::init(RendererContext& context, int32_t x, int32_t y, uint32_t barWidth, const TabControlColors& colors,
                      const char16_t** tabLabels, size_t tabCount) {
  auto& font = context.getFont(FontType::labels);
  const uint32_t tabHeight = font.XHeight() + (paddingY << 1);
  uint32_t tabWidth = (barWidth - tabHeight) / (uint32_t)tabCount;
  if (tabWidth >= maxTabWidth)
    tabWidth = (tabWidth > maxTabWidth + 8u) ? maxTabWidth : maxTabWidth - 8u;
  
  // create background bar
  std::vector<ControlVertex> vertices(static_cast<size_t>(4 + 4));
  const float* backgroundColor = colors.colors[0];
  const float transparentColor[4]{ backgroundColor[0], backgroundColor[1], backgroundColor[2], 0.1f*backgroundColor[3] };
  const float gradientColor[4]{ backgroundColor[0], backgroundColor[1], backgroundColor[2], 0.5f*backgroundColor[3] }; 
  const uint32_t barMeshWidth = barWidth - tabHeight; // slanted tabs
  GeometryGenerator::fillVerticalRectangleVertices(vertices.data(), transparentColor, gradientColor,
                                                   0.f, (float)barMeshWidth, 0.f, -(float)(tabHeight - 1u));
  GeometryGenerator::fillRectangleVertices(vertices.data() + 4, backgroundColor, 0.f, (float)barMeshWidth,
                                           -(float)(tabHeight - 1u), -(float)tabHeight);
  std::vector<uint32_t> indices{ 0,1,2,2,1,3, 4,5,6,6,5,7 };

  barMesh = ControlMesh(context.renderer(), std::move(vertices), indices, context.pixelSizeX(), context.pixelSizeY(),
                        x + (int32_t)tabHeight, y, barMeshWidth, tabHeight);

  // create tabs
  const int32_t tabLabelY = y + (int32_t)paddingY + 2;
  int32_t tabX = x;
  tabMeshes.reserve(tabCount);
  indices = { 0,1,2,2,1,3 };
  for (uint32_t index = 0; index < (uint32_t)tabCount; ++index, ++tabLabels, tabX += tabWidth) {
    vertices = std::vector<ControlVertex>(static_cast<size_t>(4u));
    GeometryGenerator::fillRectangleVertices(vertices.data(), (index != selectedIndex) ? backgroundColor : colors.colors[1],
                                             0.f, (float)tabWidth, 0.f, -(float)tabHeight);
    vertices[2].position[0] += (float)tabHeight; // slanted
    vertices[3].position[0] += (float)tabHeight;
    vertices[3].color[0] *= cornerColorFactor;
    vertices[3].color[1] *= cornerColorFactor;
    vertices[3].color[2] *= cornerColorFactor;
    ControlMesh tabBackground(context.renderer(), std::move(vertices), indices, context.pixelSizeX(),
                              context.pixelSizeY(), tabX, y, tabWidth, tabHeight);

    TextMesh tabLabel(context.renderer(), font, *tabLabels, context.pixelSizeX(), context.pixelSizeY(),
                      tabX + (int32_t)((tabWidth + tabHeight) >> 1), tabLabelY, TextAlignment::center);

    tabMeshes.emplace_back(y, tabHeight, std::move(tabBackground), std::move(tabLabel));
  }
}

// ---

void TabControl::move(RendererContext& context, int32_t x, int32_t y, uint32_t barWidth) {
  const uint32_t tabHeight = barMesh.height();
  const uint32_t barMeshWidth = barWidth - tabHeight;
  std::vector<ControlVertex> vertices = barMesh.relativeVertices();
  vertices[1].position[0] = (float)barMeshWidth;
  vertices[3].position[0] = (float)barMeshWidth;
  vertices[5].position[0] = (float)barMeshWidth;
  vertices[7].position[0] = (float)barMeshWidth;
  barMesh.update(context.renderer(), std::move(vertices), context.pixelSizeX(), context.pixelSizeY(),
                 x + (int32_t)tabHeight, y, barMeshWidth, barMesh.height());

  if (!tabMeshes.empty()) {
    uint32_t tabWidth = (barWidth - tabHeight) / (uint32_t)tabMeshes.size();
    if (tabWidth >= maxTabWidth)
      tabWidth = (tabWidth > maxTabWidth + 8u) ? maxTabWidth : maxTabWidth - 8u;

    int32_t tabX = x;
    const int32_t tabLabelY = y + (int32_t)paddingY + 2;
    for (uint32_t index = 0; index < (uint32_t)tabMeshes.size(); ++index, tabX += tabWidth) {
      auto& tabMesh = tabMeshes[index];
      vertices = tabMesh.backgroundMesh.relativeVertices();
      vertices[1].position[0] = (float)tabWidth;
      vertices[3].position[0] = (float)(tabWidth + tabHeight);
      tabMesh.backgroundMesh.update(context.renderer(), std::move(vertices), context.pixelSizeX(), context.pixelSizeY(),
                                    tabX, y, tabWidth, tabMesh.backgroundMesh.height());
      tabMesh.nameMesh.move(context.renderer(), context.pixelSizeX(), context.pixelSizeY(),
                            tabX + (int32_t)((tabWidth + tabHeight) >> 1) - (int32_t)(tabMesh.nameMesh.width() >> 1), tabLabelY);
    }
  }
}

void TabControl::updateSelection(RendererContext& context, uint32_t index) {
  if (!tabMeshes.empty()) {
    auto& oldSelectedMesh = tabMeshes[selectedIndex].backgroundMesh;
    auto& newSelectedMesh = tabMeshes[index].backgroundMesh;
    selectedIndex = index;

    float activeColor[4]; float regularColor[4];
    memcpy(activeColor, oldSelectedMesh.relativeVertices()[0].color, sizeof(float)*4u);
    memcpy(regularColor, newSelectedMesh.relativeVertices()[0].color, sizeof(float)*4u);

    std::vector<ControlVertex> vertices = oldSelectedMesh.relativeVertices();
    for (auto& vertex : vertices)
      memcpy(vertex.color, regularColor, sizeof(float)*4u);
    vertices[3].color[0] *= cornerColorFactor;
    vertices[3].color[1] *= cornerColorFactor;
    vertices[3].color[2] *= cornerColorFactor;
    oldSelectedMesh.update(context.renderer(), std::move(vertices), context.pixelSizeX(), context.pixelSizeY(),
                           oldSelectedMesh.x(), oldSelectedMesh.y(), oldSelectedMesh.width(), oldSelectedMesh.height());

    vertices = newSelectedMesh.relativeVertices();
    for (auto& vertex : vertices)
      memcpy(vertex.color, activeColor, sizeof(float)*4u);
    newSelectedMesh.update(context.renderer(), std::move(vertices), context.pixelSizeX(), context.pixelSizeY(),
                           newSelectedMesh.x(), newSelectedMesh.y(), newSelectedMesh.width(), newSelectedMesh.height());

    if (onChange)
      onChange(selectedIndex);
  }
}


// -- operations -- ------------------------------------------------------------

void TabControl::click(RendererContext& context, int32_t mouseX, int32_t mouseY) {
  if (mouseY >= barMesh.y() && mouseY < barMesh.y() + (int32_t)barMesh.height()) { // tab hover
    mouseX -= (mouseY - barMesh.y()); // slanted hover detection

    int32_t hoverIndex = -1, index = 0;
    for (auto& mesh : tabMeshes) {
      if (mouseX >= mesh.backgroundMesh.x() && mouseX < mesh.backgroundMesh.x() + (int32_t)mesh.backgroundMesh.width()) {
        hoverIndex = index;
        break;
      }
      ++index;
    }
    if (hoverIndex > -1)
      updateSelection(context, (uint32_t)hoverIndex);
  }
}

// ---

void TabControl::selectPrevious(RendererContext& context) {
  uint32_t newIndex = (selectedIndex != 0) ? (selectedIndex - 1u) : ((uint32_t)tabMeshes.size() - 1u);
  updateSelection(context, (uint32_t)newIndex);
}

void TabControl::selectNext(RendererContext& context) {
  uint32_t newIndex = (selectedIndex + 1u < (uint32_t)tabMeshes.size()) ? (selectedIndex + 1u) : 0;
  updateSelection(context, (uint32_t)newIndex);
}

void TabControl::selectIndex(RendererContext& context, uint32_t index) {
  if (index < (uint32_t)tabMeshes.size()) {
    updateSelection(context, index);
  }
}


// -- rendering -- -------------------------------------------------------------

void TabControl::drawBackground(RendererContext& context, int32_t mouseX, int32_t mouseY, RendererStateBuffers& buffers) {
  buffers.bindControlBuffer(context.renderer(), ControlBufferType::regular);
  barMesh.draw(context.renderer());

  if (mouseY >= barMesh.y() && mouseY < barMesh.y() + (int32_t)barMesh.height()) { // tab hover
    mouseX -= (mouseY - barMesh.y()); // slanted hover detection

    int32_t hoverIndex = -1, index = 0;
    for (auto& mesh : tabMeshes) {
      if (mouseX >= mesh.backgroundMesh.x() && mouseX < mesh.backgroundMesh.x() + (int32_t)mesh.backgroundMesh.width())
        hoverIndex = index;
      else 
        mesh.backgroundMesh.draw(context.renderer());
      ++index;
    }
    if (hoverIndex > -1) {
      buffers.bindControlBuffer(context.renderer(), ControlBufferType::active);
      tabMeshes[hoverIndex].backgroundMesh.draw(context.renderer());
    }
  }
  else { // no hover
    for (auto& mesh : tabMeshes)
      mesh.backgroundMesh.draw(context.renderer());
  }
}

void TabControl::drawLabels(RendererContext& context, int32_t mouseX, int32_t mouseY, RendererStateBuffers& buffers) {
  buffers.bindLabelBuffer(context.renderer(), LabelBufferType::tab);
  if (mouseY >= barMesh.y() && mouseY < barMesh.y() + (int32_t)barMesh.height()) { // tab hover
    mouseX -= (mouseY - barMesh.y()); // slanted hover detection

    int32_t hoverIndex = -1;
    uint32_t index = 0;
    for (auto& mesh : tabMeshes) {
      if (index != selectedIndex) {
        if (mouseX >= mesh.backgroundMesh.x() && mouseX < mesh.backgroundMesh.x() + (int32_t)mesh.backgroundMesh.width())
          hoverIndex = (int32_t)index;
        else
          mesh.nameMesh.draw(context.renderer());
      }
      ++index;
    }
    buffers.bindLabelBuffer(context.renderer(), LabelBufferType::tabActive);
    tabMeshes[selectedIndex].nameMesh.draw(context.renderer());
    if (hoverIndex > -1)
      tabMeshes[hoverIndex].nameMesh.draw(context.renderer());
  }
  else { // no hover
    uint32_t index = 0;
    for (auto& mesh : tabMeshes) {
      if (index != selectedIndex)
        mesh.nameMesh.draw(context.renderer());
      ++index;
    }
    buffers.bindLabelBuffer(context.renderer(), LabelBufferType::tabActive);
    tabMeshes[selectedIndex].nameMesh.draw(context.renderer());
  }
}


      