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
using namespace menu;


// -- init/resize geometry -- --------------------------------------------------

#define ARROW_SIZE 7

void VerticalTabControl::init(RendererContext& context, int32_t x, int32_t y, uint32_t tabWidth, uint32_t barHeight,
                              uint32_t paddingY, uint32_t paddingTop, const controls::VerticalTabControlColors& colors,
                              const VerticalTabOption* tabs, size_t tabCount) {
  // vertical bar
  std::vector<ControlVertex> vertices(static_cast<size_t>(14));
  ControlVertex* vertexIt = vertices.data();
  
  GeometryGenerator::fillControlVertex(*vertexIt, colors.colors[0], 0.f, 0.f);
  GeometryGenerator::fillControlVertex(*(++vertexIt), colors.colors[0], (float)tabWidth, 0.f);
  GeometryGenerator::fillVerticalRectangleVertices(++vertexIt, colors.colors[0], colors.colors[1],
                                                   0.f, (float)tabWidth, -(float)(barHeight >> 1), -(float)barHeight); // background
  vertexIt += 4;
  const float* borderColor = colors.colors[2];
  GeometryGenerator::fillRectangleVertices(vertexIt, borderColor, (float)tabWidth, (float)(tabWidth+1), // right border
                                           0.f, -(float)barHeight);
  vertexIt += 4;
  float topBorderColor[4]{ borderColor[0], borderColor[1], borderColor[2], 0.9f*borderColor[3] };
  if (borderColor[0] > borderColor[1] && borderColor[0] > 1.5f * borderColor[2]) { // red/yellow bar -> darken below top padding
    topBorderColor[0] *= 0.45f;
    topBorderColor[1] *= 0.45f;
    topBorderColor[2] *= 0.45f;
    topBorderColor[3] = 0.75f * borderColor[3];
  }
  GeometryGenerator::fillRectangleVertices(vertexIt, topBorderColor, 0.f, (float)tabWidth, // top border
                                           -(float)paddingTop, -(float)(paddingTop+1u));
  std::vector<uint32_t> indices{ 0,1,2,2,1,3, 2,3,4,4,3,5, 6,7,8,8,7,9, 10,11,12,12,11,13 };
  barMesh = ControlMesh(context.renderer(), std::move(vertices), indices, context.pixelSizeX(), context.pixelSizeY(),
                        x, y, tabWidth+1u, barHeight);

  // tab icons/labels
  auto& font = context.getFont(FontType::inputText);
  int32_t tabY = y + (int32_t)paddingTop + (int32_t)paddingY + 2;
  int32_t tabCenterX = x + (int32_t)(tabWidth >> 1);

  for (const auto* tabIt = tabs; tabCount; ++tabIt, --tabCount) {
    IconMesh icon;
    TextMesh label;
    uint32_t height = paddingY;
    if (tabIt->icon() != TabIconType::none) {
      auto iconData = context.imageLoader().getTabIcon(tabIt->icon(), context.scaling());
      if (iconData.texture() != nullptr) {
        const int32_t iconX = x + ((int32_t)tabWidth - (int32_t)iconData.contentWidth())/2;
        icon = IconMesh(context.renderer(), std::move(iconData.texture()), context.pixelSizeX(), context.pixelSizeY(),
                        iconX, tabY + (int32_t)paddingY, iconData.offsetX(), iconData.offsetY(), iconData.textureWidth(), iconData.textureHeight(), iconData.scaling());
        height += iconData.contentHeight();
      }
    }
    if (tabWidth >= Control::sectionWideTabWidth() && tabIt->name() && *(tabIt->name()) != (char16_t)0) {
      int32_t labelY = tabY + (int32_t)height;
      if (icon.width()) {
        labelY += (int32_t)iconLabelMargin();
        height += iconLabelMargin();
      }
      height += font.XHeight();

      label = TextMesh(context.renderer(), font, tabIt->name(), context.pixelSizeX(), context.pixelSizeY(), tabCenterX, labelY, TextAlignment::center);
    }
    height += paddingY;

    tabMeshes.emplace_back(tabY, height, std::move(icon), std::move(label));
    tabY += (int32_t)height;
  }

  // active tab arrow
  vertices = std::vector<ControlVertex>(static_cast<size_t>(3));
  vertexIt = vertices.data();
  float arrowColor[4]{ borderColor[0]*1.1f, borderColor[1]*1.1f, borderColor[2]*1.1f, borderColor[3]*2.f };
  if (arrowColor[3] > 1.f)
    arrowColor[3] = 1.f;
  GeometryGenerator::fillControlVertex(*vertexIt,     arrowColor, 0.f, -(float)ARROW_SIZE);
  GeometryGenerator::fillControlVertex(*(++vertexIt), borderColor, (float)ARROW_SIZE, 0.f);
  GeometryGenerator::fillControlVertex(*(++vertexIt), borderColor, (float)ARROW_SIZE, -(float)(ARROW_SIZE << 1));
  indices.resize(3);

  const auto& selectedTab = tabMeshes[selectedIndex];
  const int32_t arrowY = (selectedTab.iconMesh.width())
                       ? (selectedTab.iconMesh.y() + (int32_t)((selectedTab.iconMesh.height() + 1u) >> 1) - ARROW_SIZE)
                       : (selectedTab.y + (int32_t)(selectedTab.height >> 1) - ARROW_SIZE);
  activeTabMesh = ControlMesh(context.renderer(), std::move(vertices), indices, context.pixelSizeX(), context.pixelSizeY(),
                              x + (int32_t)tabWidth - 7, arrowY, 7u, 14u);
}

// ---

void VerticalTabControl::move(RendererContext& context, int32_t x, int32_t y, uint32_t barHeight) {
  const int32_t offsetX = x - barMesh.x();
  const int32_t offsetY = y - barMesh.y();
  auto vertices = barMesh.relativeVertices();
  GeometryGenerator::moveRectangleVerticesY(vertices.data() + 2, -(float)(barHeight >> 1), -(float)barHeight);
  GeometryGenerator::resizeRectangleVerticesY(vertices.data() + 6, -(float)barHeight);
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
  moveSelection(context, false);
}

void VerticalTabControl::move(RendererContext& context, uint32_t barHeight) {
  if (barHeight != barMesh.height()) {
    auto vertices = barMesh.relativeVertices();
    GeometryGenerator::moveRectangleVerticesY(vertices.data() + 2, -(float)(barHeight >> 1), -(float)barHeight);
    GeometryGenerator::resizeRectangleVerticesY(vertices.data() + 6, -(float)barHeight);
    barMesh.update(context.renderer(), std::move(vertices), context.pixelSizeX(), context.pixelSizeY(),
                   barMesh.x(), barMesh.y(), barMesh.width(), barHeight);
  }
}

void VerticalTabControl::moveSelection(RendererContext& context, bool notify) {
  const auto& selectedTab = tabMeshes[selectedIndex];
  const int32_t arrowY = (selectedTab.iconMesh.width())
                       ? (selectedTab.iconMesh.y() + (int32_t)((selectedTab.iconMesh.height() + 1u) >> 1) - ARROW_SIZE)
                       : (selectedTab.y + (int32_t)(selectedTab.height >> 1) - ARROW_SIZE);

  activeTabMesh.move(context.renderer(), context.pixelSizeX(), context.pixelSizeY(),
                      activeTabMesh.x(), arrowY);
  if (notify && onChange)
    onChange(selectedIndex);
}


// -- operations -- ------------------------------------------------------------

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
  if (selectedIndex != (uint32_t)currentIndex) {
    selectedIndex = (uint32_t)currentIndex;
    moveSelection(context, true);
  }
}

// ---

void VerticalTabControl::selectPrevious(RendererContext& context) {
  if (selectedIndex != 0)
    --selectedIndex;
  else
    selectedIndex = (uint32_t)tabMeshes.size() - 1u;

  moveSelection(context, true);
}

void VerticalTabControl::selectNext(RendererContext& context) {
  if (selectedIndex + 1u < (uint32_t)tabMeshes.size())
    ++selectedIndex;
  else
    selectedIndex = 0;

  moveSelection(context, true);
}

void VerticalTabControl::selectIndex(RendererContext& context, uint32_t index) {
  if (index < (uint32_t)tabMeshes.size()) {
    this->selectedIndex = index;
    moveSelection(context, false);
  }
}


// -- rendering -- -------------------------------------------------------------

void VerticalTabControl::drawIcons(RendererContext& context, int32_t mouseX, int32_t mouseY,
                                   RendererStateBuffers& buffers) {
  uint32_t currentIndex = 0;
  int32_t hoverIndex = -1;
  buffers.bindIconBuffer(context.renderer(), ControlBufferType::regularTabIcon);
  if (mouseX >= barMesh.x() && mouseX < barMesh.x() + (int32_t)barMesh.width()) {
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

  buffers.bindIconBuffer(context.renderer(), ControlBufferType::activeTabIcon);
  tabMeshes[selectedIndex].iconMesh.draw(context.renderer());
  if (hoverIndex >= 0 && hoverIndex != (int32_t)selectedIndex)
    tabMeshes[hoverIndex].iconMesh.draw(context.renderer());
}

void VerticalTabControl::drawLabels(RendererContext& context, int32_t mouseX, int32_t mouseY,
                                    RendererStateBuffers& buffers) {
  uint32_t currentIndex = 0;
  int32_t hoverIndex = -1;
  buffers.bindLabelBuffer(context.renderer(), LabelBufferType::verticalTab);
  if (mouseX >= barMesh.x() && mouseX < barMesh.x() + (int32_t)barMesh.width()) {
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

  buffers.bindLabelBuffer(context.renderer(), LabelBufferType::verticalTabActive);
  tabMeshes[selectedIndex].nameMesh.draw(context.renderer());
  if (hoverIndex >= 0 && hoverIndex != (int32_t)selectedIndex)
    tabMeshes[hoverIndex].nameMesh.draw(context.renderer());
}


      