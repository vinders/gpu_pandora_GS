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
#include "menu/controls/vertical_tab_control.h"

using namespace video_api;
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

ControlType VerticalTabControl::Type() const noexcept { return ControlType::verticalTabControl; }

void VerticalTabControl::init(RendererContext& context, int32_t x, int32_t y, uint32_t tabWidth, uint32_t barHeight,
                              uint32_t paddingY, uint32_t paddingTop, const float barColor[4], const float borderColor[4],
                              const TabOption* tabs, size_t tabCount) {
  // vertical bar
  std::vector<ControlVertex> vertices;
  vertices.resize(paddingTop ? 12 : 8);
  ControlVertex* vertexIt = vertices.data();

  setControlVertex(*vertexIt,     barColor, 0.f,             0.f);
  setControlVertex(*(++vertexIt), barColor, (float)tabWidth, 0.f);
  setControlVertex(*(++vertexIt), barColor, 0.f,             -(float)barHeight);
  setControlVertex(*(++vertexIt), barColor, (float)tabWidth, -(float)barHeight);

  setControlVertex(*(++vertexIt), borderColor, (float)tabWidth,     0.f);
  setControlVertex(*(++vertexIt), borderColor, (float)(tabWidth+1), 0.f);
  setControlVertex(*(++vertexIt), borderColor, (float)tabWidth,     -(float)barHeight);
  setControlVertex(*(++vertexIt), borderColor, (float)(tabWidth+1), -(float)barHeight);

  std::vector<uint32_t> indices;
  if (paddingTop) {
    setControlVertex(*(++vertexIt), borderColor, 0.f,             -(float)paddingTop);
    setControlVertex(*(++vertexIt), borderColor, (float)tabWidth, -(float)paddingTop);
    setControlVertex(*(++vertexIt), borderColor, 0.f,             -(float)(paddingTop+1u));
    setControlVertex(*(++vertexIt), borderColor, (float)tabWidth, -(float)(paddingTop+1u));
    indices = { 0,1,2, 2,1,3,  4,5,6, 6,5,7,  8,9,10, 10,9,11 };
  }
  else {
    vertices.resize(8);
    vertexIt = vertices.data();
    indices = { 0,1,2, 2,1,3,  4,5,6, 6,5,7 };
  }
  barMesh = ControlMesh(context.renderer(), std::move(vertices), indices, context.pixelSizeX(), context.pixelSizeY(),
                        x, y, tabWidth+1u, barHeight);

  // tab icons/labels
  auto& font = context.getFont(FontType::labels);
  int32_t tabY = y + (int32_t)paddingTop + (int32_t)(paddingY >> 1);
  int32_t tabCenterX = x + (int32_t)(tabWidth >> 1);

  for (const auto* tabIt = tabs; tabCount; ++tabIt, --tabCount) {
    IconMesh icon;
    TextMesh label;
    uint32_t height = paddingY;
    if (tabIt->icon != ControlIconType::none) {
      auto iconData = context.imageLoader().getIcon(tabIt->icon);
      if (iconData.texture() != nullptr) {
        const int32_t iconX = x + ((int32_t)tabWidth - (int32_t)iconData.width())/2;
        icon = IconMesh(context.renderer(), std::move(iconData.texture()), context.pixelSizeX(), context.pixelSizeY(),
                        iconX, tabY + (int32_t)paddingY, iconData.offsetX(), iconData.offsetY(), iconData.width(), iconData.height());
        height += iconData.height();
      }
    }
    if (tabIt->name && *(tabIt->name.get()) != (char32_t)0) {
      int32_t labelY = y + (int32_t)height;
      if (icon.width())
        labelY += (int32_t)iconLabelMargin();
      label = TextMesh(context.renderer(), font, tabIt->name.get(), context.pixelSizeX(), context.pixelSizeY(), tabCenterX, labelY, TextAlignment::center);
      height += font.XHeight();
    }
    height += paddingY;

    tabMeshes.emplace_back(tabY, height, std::move(icon), std::move(label));
    tabY += (int32_t)height;
  }

  // active tab arrow
  vertices.resize(3);
  indices.resize(3);
  vertexIt = vertices.data();
  setControlVertex(*vertexIt,     borderColor, 0.f, 0.f);
  setControlVertex(*(++vertexIt), borderColor, 6.f, -6.f);
  setControlVertex(*(++vertexIt), borderColor, 0.f, -12.f);
  const auto& selectedTab = tabMeshes[selectedIndex];
  activeTabMesh = ControlMesh(context.renderer(), std::move(vertices), indices, context.pixelSizeX(), context.pixelSizeY(),
                              x + (int32_t)tabWidth - 6, selectedTab.y + (int32_t)(selectedTab.height >> 1) - 6, 6u, 12u);
}

void VerticalTabControl::move(RendererContext& context, int32_t x, int32_t y, uint32_t barHeight) {
  const int32_t offsetX = x - barMesh.x();
  const int32_t offsetY = y - barMesh.y();
  auto vertices = barMesh.relativeVertices();
  vertices[2].position[1] = -(float)barHeight;
  vertices[3].position[1] = -(float)barHeight;
  vertices[6].position[1] = -(float)barHeight;
  vertices[7].position[1] = -(float)barHeight;
  barMesh.update(context.renderer(), std::move(vertices), context.pixelSizeX(), context.pixelSizeY(),
                 x, y, barMesh.width(), barHeight);

  for (auto& tab : tabMeshes) {
    tab.y += offsetY;
    if (tab.iconMesh.width()) {
      tab.iconMesh.move(context.renderer(), context.pixelSizeX(), context.pixelSizeY(),
                        tab.iconMesh.x() + offsetX, tab.iconMesh.y() + offsetY);
    }
    if (tab.nameMesh.width()) {
      tab.nameMesh.move(context.renderer(), context.pixelSizeX(), context.pixelSizeY(),
                        tab.nameMesh.x() + offsetX, tab.nameMesh.y() + offsetY);
    }
  }

  const auto& selectedTab = tabMeshes[selectedIndex];
  activeTabMesh.move(context.renderer(), context.pixelSizeX(), context.pixelSizeY(),
                     activeTabMesh.x() + offsetX, selectedTab.y + (int32_t)(selectedTab.height >> 1) - 6);
}

// ---

void VerticalTabControl::click(RendererContext& context, int32_t mouseY) {
  int32_t currentIndex = -1;
  for (const auto& mesh : tabMeshes) {
    if (mouseY < mesh.y)
      break;
    ++currentIndex;
  }

  if (currentIndex == -1)
    return;
  if (currentIndex + 1 >= (int32_t)tabMeshes.size()) {
    const auto& lastTabLabel = tabMeshes.back();
    if (mouseY >= lastTabLabel.y + (int32_t)lastTabLabel.height)
      return;
  }
  selectedIndex = (uint32_t)currentIndex;

  const auto& selectedTab = tabMeshes[selectedIndex];
  activeTabMesh.move(context.renderer(), context.pixelSizeX(), context.pixelSizeY(),
                     activeTabMesh.x(), selectedTab.y + (int32_t)(selectedTab.height >> 1) - 6);
}

void VerticalTabControl::selectPrevious(RendererContext& context) {
  if (selectedIndex != 0)
    --selectedIndex;
  else
    selectedIndex = (uint32_t)tabMeshes.size() - 1u;

  const auto& selectedTab = tabMeshes[selectedIndex];
  activeTabMesh.move(context.renderer(), context.pixelSizeX(), context.pixelSizeY(),
                     activeTabMesh.x(), selectedTab.y + (int32_t)(selectedTab.height >> 1) - 6);
}

void VerticalTabControl::selectNext(RendererContext& context) {
  if (selectedIndex + 1u < (uint32_t)tabMeshes.size())
    ++selectedIndex;
  else
    selectedIndex = 0;

  const auto& selectedTab = tabMeshes[selectedIndex];
  activeTabMesh.move(context.renderer(), context.pixelSizeX(), context.pixelSizeY(),
                     activeTabMesh.x(), selectedTab.y + (int32_t)(selectedTab.height >> 1) - 6);
}

void VerticalTabControl::selectIndex(RendererContext& context, uint32_t index) {
  if (index < (uint32_t)tabMeshes.size()) {
    this->selectedIndex = index;

    const auto& selectedTab = tabMeshes[selectedIndex];
    activeTabMesh.move(context.renderer(), context.pixelSizeX(), context.pixelSizeY(),
                       activeTabMesh.x(), selectedTab.y + (int32_t)(selectedTab.height >> 1) - 6);
  }
}

// ---

void VerticalTabControl::drawIcons(RendererContext& context, int32_t mouseX, int32_t mouseY,
                                   Buffer<ResourceUsage::staticGpu>& hoverActiveFragmentUniform) {
  uint32_t currentIndex = 0;
  int32_t hoverIndex = -1;
  if (mouseX >= barMesh.x() && mouseX < barMesh.x() + (int32_t)barMesh.height()) {
    for (auto& mesh : tabMeshes) {
      if (mouseY >= mesh.y && mouseY < mesh.y + (int32_t)mesh.height) {
        hoverIndex = (int32_t)currentIndex;
      }
      else if (currentIndex != selectedIndex)
        mesh.iconMesh.draw(context.renderer());
      ++currentIndex;
    }
  }
  else {
    for (auto& mesh : tabMeshes) {
      if (currentIndex != selectedIndex)
        mesh.iconMesh.draw(context.renderer());
      ++currentIndex;
    }
  }

  context.renderer().bindFragmentUniforms(0, hoverActiveFragmentUniform.handlePtr(), 1);
  tabMeshes[selectedIndex].iconMesh.draw(context.renderer());
  if (hoverIndex >= 0 && hoverIndex != (int32_t)selectedIndex)
    tabMeshes[hoverIndex].iconMesh.draw(context.renderer());
}

void VerticalTabControl::drawLabels(RendererContext& context, int32_t mouseX, int32_t mouseY,
                                    Buffer<ResourceUsage::staticGpu>& hoverActiveFragmentUniform) {
  uint32_t currentIndex = 0;
  int32_t hoverIndex = -1;
  if (mouseX >= barMesh.x() && mouseX < barMesh.x() + (int32_t)barMesh.height()) {
    for (auto& mesh : tabMeshes) {
      if (mouseY >= mesh.y && mouseY < mesh.y + (int32_t)mesh.height) {
        hoverIndex = (int32_t)currentIndex;
      }
      else if (currentIndex != selectedIndex)
        mesh.nameMesh.draw(context.renderer());
      ++currentIndex;
    }
  }
  else {
    for (auto& mesh : tabMeshes) {
      if (currentIndex != selectedIndex)
        mesh.nameMesh.draw(context.renderer());
      ++currentIndex;
    }
  }

  context.renderer().bindFragmentUniforms(0, hoverActiveFragmentUniform.handlePtr(), 1);
  tabMeshes[selectedIndex].nameMesh.draw(context.renderer());
  if (hoverIndex >= 0 && hoverIndex != (int32_t)selectedIndex)
    tabMeshes[hoverIndex].nameMesh.draw(context.renderer());
}


      