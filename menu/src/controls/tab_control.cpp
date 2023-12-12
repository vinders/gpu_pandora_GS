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
static constexpr const uint32_t gradientWidth = (maxTabWidth >> 1);
static constexpr const float cornerColorFactor = 1.25f;

void TabControl::init(RendererContext& context, int32_t x, int32_t y, uint32_t barWidth, const TabControlColors& colors,
                      const char16_t** tabLabels, size_t tabCount) {
  if (tabCount == 0)
    return;
  auto& font = context.getFont(FontType::labels);
  const uint32_t tabHeight = font.XHeight() + (paddingY << 1);
  uint32_t tabWidth = (barWidth - tabHeight) / (uint32_t)tabCount;
  if (tabWidth >= maxTabWidth)
    tabWidth = (tabWidth > maxTabWidth + 8u) ? maxTabWidth : maxTabWidth - 8u;

  const float* idleTabColor = colors.colors[0];
  const float* activeTabColor = colors.colors[1];
  std::vector<ControlVertex> vertices;
  std::vector<uint32_t> indices{ 0,1,2,2,1,3 };

  // create tabs
  const int32_t tabLabelY = y + (int32_t)paddingY + 2;
  const uint32_t totalTabsWidth = tabWidth*(uint32_t)tabCount + tabHeight;
  int32_t tabX = (barWidth > totalTabsWidth + Control::scrollbarWidth())
               ? x + (int32_t)((barWidth - Control::scrollbarWidth() - totalTabsWidth) >> 1)
               : x + (int32_t)((barWidth - totalTabsWidth) >> 1);
  tabMeshes.reserve(tabCount);
  for (uint32_t index = 0; index < (uint32_t)tabCount; ++index, ++tabLabels, tabX += tabWidth) {
    vertices = std::vector<ControlVertex>(static_cast<size_t>(4u));
    GeometryGenerator::fillRectangleVertices(vertices.data(), (index != selectedIndex) ? idleTabColor : activeTabColor,
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

  // create underline bar (with gradients)
  vertices = std::vector<ControlVertex>(static_cast<size_t>(8));
  const float transparentColor[4]{ activeTabColor[0], activeTabColor[1], activeTabColor[2], 0.f };
  GeometryGenerator::fillHorizontalRectangleVertices(vertices.data(), transparentColor, activeTabColor,
                                                     (float)tabHeight, (float)(tabHeight + gradientWidth),
                                                     -(float)(tabHeight - 1u), -(float)tabHeight);
  GeometryGenerator::fillHorizontalRectangleVertices(vertices.data() + 4, activeTabColor, transparentColor,
                                                     (float)(totalTabsWidth - tabHeight - gradientWidth), (float)(totalTabsWidth - tabHeight),
                                                     -(float)(tabHeight - 1u), -(float)tabHeight);
  indices = std::vector<uint32_t>{ 0,1,2,2,1,3, 1,4,3,3,4,6, 4,5,6,6,5,7 };

  barMesh = ControlMesh(context.renderer(), std::move(vertices), indices, context.pixelSizeX(), context.pixelSizeY(),
                        tabMeshes[0].backgroundMesh.x(), y, totalTabsWidth, tabHeight);
}

// ---

void TabControl::move(RendererContext& context, int32_t x, int32_t y, uint32_t barWidth) {
  if (!tabMeshes.empty()) {
    const uint32_t tabHeight = barMesh.height();
    uint32_t tabWidth = (barWidth - tabHeight) / (uint32_t)tabMeshes.size();
    if (tabWidth >= maxTabWidth)
      tabWidth = (tabWidth > maxTabWidth + 8u) ? maxTabWidth : maxTabWidth - 8u;

    std::vector<ControlVertex> vertices;

    // tabs
    const int32_t tabLabelY = y + (int32_t)paddingY + 2;
    const uint32_t totalTabsWidth = tabWidth*(uint32_t)tabMeshes.size() + tabHeight;
    int32_t tabX = (barWidth > totalTabsWidth + Control::scrollbarWidth())
                 ? x + (int32_t)((barWidth - Control::scrollbarWidth() - totalTabsWidth) >> 1)
                 : x + (int32_t)((barWidth - totalTabsWidth) >> 1);
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

    // underline bar (with gradients)
    vertices = barMesh.relativeVertices();
    GeometryGenerator::moveRectangleVerticesX(vertices.data(), (float)tabHeight, (float)(tabHeight + gradientWidth));
    GeometryGenerator::moveRectangleVerticesX(vertices.data() + 4, (float)(totalTabsWidth - tabHeight - gradientWidth), (float)(totalTabsWidth - tabHeight));
    barMesh.update(context.renderer(), std::move(vertices), context.pixelSizeX(), context.pixelSizeY(),
                   tabMeshes[0].backgroundMesh.x(), y, totalTabsWidth, tabHeight);
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
  auto& renderer = context.renderer();
  buffers.bindControlBuffer(renderer, ControlBufferType::regular);
  mouseX -= (mouseY - barMesh.y()); // slanted hover detection

  if (mouseY >= barMesh.y() && mouseY < barMesh.y() + (int32_t)barMesh.height()) { // tab hover
    int32_t hoverIndex = -1, index = 0;
    for (auto& mesh : tabMeshes) {
      if (index != (int32_t)selectedIndex
      && mouseX >= mesh.backgroundMesh.x() && mouseX < mesh.backgroundMesh.x() + (int32_t)mesh.backgroundMesh.width())
        hoverIndex = index;
      else
        mesh.backgroundMesh.draw(renderer);
      ++index;
    }

    if (hoverIndex > -1) {
      buffers.bindControlBuffer(renderer, ControlBufferType::active);
      tabMeshes[hoverIndex].backgroundMesh.draw(renderer);
      buffers.bindControlBuffer(renderer, ControlBufferType::regular);
    }
  }
  else { // no hover
    for (auto& mesh : tabMeshes)
      mesh.backgroundMesh.draw(renderer);
  }
  barMesh.draw(renderer);
}

void TabControl::drawLabels(RendererContext& context, int32_t mouseX, int32_t mouseY, RendererStateBuffers& buffers) {
  auto& renderer = context.renderer();
  buffers.bindLabelBuffer(renderer, LabelBufferType::tab);

  if (mouseY >= barMesh.y() && mouseY < barMesh.y() + (int32_t)barMesh.height()) { // tab hover
    mouseX -= (mouseY - barMesh.y()); // slanted hover detection

    int32_t hoverIndex = -1;
    uint32_t index = 0;
    for (auto& mesh : tabMeshes) {
      if (index != selectedIndex) {
        if (mouseX >= mesh.backgroundMesh.x() && mouseX < mesh.backgroundMesh.x() + (int32_t)mesh.backgroundMesh.width())
          hoverIndex = (int32_t)index;
        else
          mesh.nameMesh.draw(renderer);
      }
      ++index;
    }
    buffers.bindLabelBuffer(renderer, LabelBufferType::tabActive);
    tabMeshes[selectedIndex].nameMesh.draw(renderer);
    if (hoverIndex > -1)
      tabMeshes[hoverIndex].nameMesh.draw(renderer);
  }
  else { // no hover
    uint32_t index = 0;
    for (auto& mesh : tabMeshes) {
      if (index != selectedIndex)
        mesh.nameMesh.draw(renderer);
      ++index;
    }
    buffers.bindLabelBuffer(renderer, LabelBufferType::tabActive);
    tabMeshes[selectedIndex].nameMesh.draw(renderer);
  }
}


      