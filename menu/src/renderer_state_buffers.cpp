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
#include <cmath>
#include "menu/renderer_state_buffers.h"

using namespace video_api;
using namespace menu;


// -- helpers -- ---------------------------------------------------------------

// Menu control/icon/text mesh color buffer
// Usages: - control color modifier for ControlMesh (vertex uniform buffer)
//         - texture color modifier for IconMesh (fragment uniform buffer)
//         - glyph background color for TextMesh (fragment uniform buffer)
__align_type(16, // force 16-byte memory alignment
struct MeshColorUniform final {
  float color[4]; // r,g,b,a
});
// Menu scroll position buffer
__align_type(16, // force 16-byte memory alignment
struct ScrollUniform final {
  float offset[4]; // x,y,z,w
});

// ---

// apply the same difference as regular->special text to any color
static void toSpecialLabelColor(float target[4], const float background[4], const float regular[4], const float special[4]) {
  const float ratio = fabs(target[0] + target[1] + target[2] - background[0] - background[1] - background[2]) * 0.3f;
  float* targetIt = target;
  *targetIt *= ratio + (1.f-ratio)*(special[0]+1.f/255.f)/(regular[0]+1.f/255.f);
  if (*targetIt > 1.f)
    *targetIt = 1.f;
  *(++targetIt) *= ratio + (1.f-ratio)*(special[1]+1.f/255.f)/(regular[1]+1.f/255.f);
  if (*targetIt > 1.f)
    *targetIt = 1.f;
  *(++targetIt) *= ratio + (1.f-ratio)*(special[2]+1.f/255.f)/(regular[2]+1.f/255.f);
  if (*targetIt > 1.f)
    *targetIt = 1.f;
  *(++targetIt) *= 0.5f + 0.5f*(special[3]+1.f/255.f)/(regular[3]+1.f/255.f);
  if (*targetIt > 1.f)
    *targetIt = 1.f;
}


// -- buffer creation/destruction -- -------------------------------------------

RendererStateBuffers::RendererStateBuffers(Renderer& renderer, const ColorTheme& theme) {
  // vertex slot 1 - scroll position
  ScrollUniform topLocation{ {0.f,0.f,0.f,0.f} };
  fixedPosition = Buffer<ResourceUsage::staticGpu>(renderer, BufferType::uniform, sizeof(fixedPosition), &topLocation);
  scrollPosition = Buffer<ResourceUsage::staticGpu>(renderer, BufferType::uniform, sizeof(fixedPosition), &topLocation);
  scrollPositionStaging = Buffer<ResourceUsage::staging>(renderer, BufferType::uniform, sizeof(fixedPosition), &topLocation);
  
  updateColorBuffers(renderer, theme);
}

void RendererStateBuffers::updateScrollBuffer(float pixelSizeY, uint32_t scrollLevelY) {
  // vertex slot 1 - scroll position
  ScrollUniform scrollLocation{ { 0.f, pixelSizeY*(float)scrollLevelY, 0.f, 0.f } };
  scrollPositionStaging.write(&scrollLocation);
  scrollPosition.copy(scrollPositionStaging);
}

void RendererStateBuffers::updateColorBuffers(Renderer& renderer, const ColorTheme& theme) {
  // vertex slot 0 - control color modifier / fragment slot 0 - icon color modifier
  MeshColorUniform colorData{ {1.f,1.f,1.f,1.f} };
  controlBuffers[(size_t)ControlBufferType::regular] = Buffer<ResourceUsage::staticGpu>(renderer, BufferType::uniform, sizeof(colorData), &colorData);
  colorData.color[2] = colorData.color[1] = colorData.color[0] = 1.5f;
  controlBuffers[(size_t)ControlBufferType::selectedTile] = Buffer<ResourceUsage::staticGpu>(renderer, BufferType::uniform, sizeof(colorData), &colorData);
  memcpy(colorData.color, theme.disabledControlModifier(), sizeof(float)*4);
  controlBuffers[(size_t)ControlBufferType::disabled] = Buffer<ResourceUsage::staticGpu>(renderer, BufferType::uniform, sizeof(colorData), &colorData);
  memcpy(colorData.color, theme.activeControlModifier(), sizeof(float)*4);
  controlBuffers[(size_t)ControlBufferType::active] = Buffer<ResourceUsage::staticGpu>(renderer, BufferType::uniform, sizeof(colorData), &colorData);
  memcpy(colorData.color, theme.activeScrollControlModifier(), sizeof(float)*4);
  controlBuffers[(size_t)ControlBufferType::activeScroll] = Buffer<ResourceUsage::staticGpu>(renderer, BufferType::uniform, sizeof(colorData), &colorData);
  memcpy(colorData.color, theme.regularIconModifier(), sizeof(float)*4);
  controlBuffers[(size_t)ControlBufferType::regularIcon] = Buffer<ResourceUsage::staticGpu>(renderer, BufferType::uniform, sizeof(colorData), &colorData);
  memcpy(colorData.color, theme.disabledIconModifier(), sizeof(float)*4);
  controlBuffers[(size_t)ControlBufferType::disabledIcon] = Buffer<ResourceUsage::staticGpu>(renderer, BufferType::uniform, sizeof(colorData), &colorData);
  memcpy(colorData.color, theme.activeIconModifier(), sizeof(float)*4);
  controlBuffers[(size_t)ControlBufferType::activeIcon] = Buffer<ResourceUsage::staticGpu>(renderer, BufferType::uniform, sizeof(colorData), &colorData);
  
  // fragment slot 0 - text background color
  memcpy(colorData.color, theme.regularLabelColor(), sizeof(float)*4);
  labelBuffers[(size_t)LabelBufferType::regular] = Buffer<ResourceUsage::staticGpu>(renderer, BufferType::uniform, sizeof(colorData), &colorData);
  memcpy(colorData.color, theme.disabledLabelColor(), sizeof(float)*4);
  labelBuffers[(size_t)LabelBufferType::disabled] = Buffer<ResourceUsage::staticGpu>(renderer, BufferType::uniform, sizeof(colorData), &colorData);
  memcpy(colorData.color, theme.activeLabelColor(), sizeof(float)*4);
  labelBuffers[(size_t)LabelBufferType::active] = Buffer<ResourceUsage::staticGpu>(renderer, BufferType::uniform, sizeof(colorData), &colorData);
  memcpy(colorData.color, theme.tileLabelColor(), sizeof(float)*4);
  labelBuffers[(size_t)LabelBufferType::tile] = Buffer<ResourceUsage::staticGpu>(renderer, BufferType::uniform, sizeof(colorData), &colorData);
  memcpy(colorData.color, theme.activeTileLabelColor(), sizeof(float)*4);
  labelBuffers[(size_t)LabelBufferType::activeTile] = Buffer<ResourceUsage::staticGpu>(renderer, BufferType::uniform, sizeof(colorData), &colorData);
  memcpy(colorData.color, theme.selectedTileLabelColor(), sizeof(float)*4);
  labelBuffers[(size_t)LabelBufferType::selectedTile] = Buffer<ResourceUsage::staticGpu>(renderer, BufferType::uniform, sizeof(colorData), &colorData);
  
  memcpy(colorData.color, theme.titleLabelColor(), sizeof(float)*4);
  labelBuffers[(size_t)LabelBufferType::title] = Buffer<ResourceUsage::staticGpu>(renderer, BufferType::uniform, sizeof(colorData), &colorData);
  memcpy(colorData.color, theme.fieldsetLabelColor(), sizeof(float)*4);
  labelBuffers[(size_t)LabelBufferType::fieldset] = Buffer<ResourceUsage::staticGpu>(renderer, BufferType::uniform, sizeof(colorData), &colorData);
  memcpy(colorData.color, theme.tabLabelColor(), sizeof(float)*4);
  labelBuffers[(size_t)LabelBufferType::tab] = Buffer<ResourceUsage::staticGpu>(renderer, BufferType::uniform, sizeof(colorData), &colorData);
  memcpy(colorData.color, theme.tabActiveLabelColor(), sizeof(float)*4);
  labelBuffers[(size_t)LabelBufferType::tabActive] = Buffer<ResourceUsage::staticGpu>(renderer, BufferType::uniform, sizeof(colorData), &colorData);
  memcpy(colorData.color, theme.verticalTabLabelColor(), sizeof(float)*4);
  labelBuffers[(size_t)LabelBufferType::verticalTab] = Buffer<ResourceUsage::staticGpu>(renderer, BufferType::uniform, sizeof(colorData), &colorData);
  memcpy(colorData.color, theme.verticalTabActiveLabelColor(), sizeof(float)*4);
  labelBuffers[(size_t)LabelBufferType::verticalTabActive] = Buffer<ResourceUsage::staticGpu>(renderer, BufferType::uniform, sizeof(colorData), &colorData);
  
  memcpy(colorData.color, theme.buttonLabelColor(), sizeof(float)*4);
  labelBuffers[(size_t)LabelBufferType::button] = Buffer<ResourceUsage::staticGpu>(renderer, BufferType::uniform, sizeof(colorData), &colorData);
  toSpecialLabelColor(colorData.color, theme.buttonReferenceColor(), theme.regularLabelColor(), theme.disabledLabelColor());
  labelBuffers[(size_t)LabelBufferType::buttonDisabled] = Buffer<ResourceUsage::staticGpu>(renderer, BufferType::uniform, sizeof(colorData), &colorData);
  memcpy(colorData.color, theme.buttonLabelColor(), sizeof(float)*4);
  toSpecialLabelColor(colorData.color, theme.buttonReferenceColor(), theme.regularLabelColor(), theme.activeLabelColor());
  labelBuffers[(size_t)LabelBufferType::buttonActive] = Buffer<ResourceUsage::staticGpu>(renderer, BufferType::uniform, sizeof(colorData), &colorData);

  memcpy(colorData.color, theme.textBoxLabelColor(), sizeof(float)*4);
  labelBuffers[(size_t)LabelBufferType::textInput] = Buffer<ResourceUsage::staticGpu>(renderer, BufferType::uniform, sizeof(colorData), &colorData);
  toSpecialLabelColor(colorData.color, theme.textBoxControlColor(), theme.regularLabelColor(), theme.disabledLabelColor());
  labelBuffers[(size_t)LabelBufferType::textInputDisabled] = Buffer<ResourceUsage::staticGpu>(renderer, BufferType::uniform, sizeof(colorData), &colorData);
  
  memcpy(colorData.color, theme.comboBoxLabelColor(), sizeof(float)*4);
  labelBuffers[(size_t)LabelBufferType::comboBoxValue] = Buffer<ResourceUsage::staticGpu>(renderer, BufferType::uniform, sizeof(colorData), &colorData);
  toSpecialLabelColor(colorData.color, theme.comboBoxControlColor(), theme.regularLabelColor(), theme.disabledLabelColor());
  labelBuffers[(size_t)LabelBufferType::comboBoxValueDisabled] = Buffer<ResourceUsage::staticGpu>(renderer, BufferType::uniform, sizeof(colorData), &colorData);
  memcpy(colorData.color, theme.comboBoxDropdownLabelColor(), sizeof(float)*4);
  labelBuffers[(size_t)LabelBufferType::dropdownValue] = Buffer<ResourceUsage::staticGpu>(renderer, BufferType::uniform, sizeof(colorData), &colorData);

  colorData.color[2] = colorData.color[1] = colorData.color[0] = 0.1f;
  colorData.color[3] = 1.f;
  labelBuffers[(size_t)LabelBufferType::keyboardKey] = Buffer<ResourceUsage::staticGpu>(renderer, BufferType::uniform, sizeof(colorData), &colorData);
  colorData.color[3] = theme.disabledControlModifier()[3];
  labelBuffers[(size_t)LabelBufferType::keyboardKeyDisabled] = Buffer<ResourceUsage::staticGpu>(renderer, BufferType::uniform, sizeof(colorData), &colorData);
}

void RendererStateBuffers::release() noexcept {
  fixedPosition.release();
  scrollPosition.release();
  
  auto* controlBuffer = &controlBuffers[0];
  for (uint32_t i = 0; i < (uint32_t)ControlBufferType::COUNT; ++i, ++controlBuffer)
    controlBuffer->release();
  auto* labelBuffer = &labelBuffers[0];
  for (uint32_t i = 0; i < (uint32_t)LabelBufferType::COUNT; ++i, ++labelBuffer)
    labelBuffer->release();
}


// -- buffer binding -- --------------------------------------------------------

void RendererStateBuffers::unbind(Renderer& renderer) noexcept {
  BufferHandle emptyHandles[]{ nullptr, nullptr };
  TextureView emptyTexture = nullptr;
  renderer.bindVertexUniforms(0, emptyHandles, 1);
  renderer.bindFragmentUniforms(0, emptyHandles, 2);
  renderer.bindFragmentTextures(0, &emptyTexture, 1);
}

// ---

void RendererStateBuffers::bindControlBuffer(Renderer& renderer, ControlBufferType type) {
  if (type != boundControlType) { // if not already bound
    // if icon/label fragment uniforms currently bound, unbind first
    if ((uint32_t)boundIconType + (uint32_t)boundLabelType != (uint32_t)ControlBufferType::COUNT + (uint32_t)LabelBufferType::COUNT) {
      BufferHandle emptyHandle = nullptr;
      TextureView emptyTexture = nullptr;
      renderer.bindFragmentUniforms(0, &emptyHandle, 1);
      renderer.bindFragmentTextures(0, &emptyTexture, 1);
      boundIconType = ControlBufferType::COUNT;
      boundLabelType = LabelBufferType::COUNT;
    }
    
    assert((uint32_t)type < (uint32_t)ControlBufferType::COUNT);
    renderer.bindVertexUniforms(0, controlBuffers[(size_t)type].handlePtr(), 1);
    boundControlType = type;
  }
}

void RendererStateBuffers::bindIconBuffer(Renderer& renderer, ControlBufferType type) {
  if (type != boundIconType) { // if not already bound
    // if control vertex uniform currently bound, unbind first
    if ((uint32_t)boundControlType != (uint32_t)LabelBufferType::COUNT) {
      BufferHandle emptyHandle = nullptr;
      renderer.bindVertexUniforms(0, &emptyHandle, 1);
      boundControlType = ControlBufferType::COUNT;
    }
    
    assert((uint32_t)type < (uint32_t)ControlBufferType::COUNT);
    renderer.bindFragmentUniforms(0, controlBuffers[(size_t)type].handlePtr(), 1);
    boundIconType = type;
    boundLabelType = LabelBufferType::COUNT; // reset label binding (needed to avoid false positives for top IF)
  }
}

void RendererStateBuffers::bindLabelBuffer(Renderer& renderer, LabelBufferType type) {
  if (type != boundLabelType) { // if not already bound
    // if control vertex uniform currently bound, unbind first
    if ((uint32_t)boundControlType != (uint32_t)LabelBufferType::COUNT) {
      BufferHandle emptyHandle = nullptr;
      renderer.bindVertexUniforms(0, &emptyHandle, 1);
      boundControlType = ControlBufferType::COUNT;
    }
    
    assert((uint32_t)type < (uint32_t)LabelBufferType::COUNT);
    renderer.bindFragmentUniforms(0, labelBuffers[(size_t)type].handlePtr(), 1);
    boundLabelType = type;
    boundIconType = ControlBufferType::COUNT; // reset icon binding flag (needed to avoid false positives for top IF)
  }
}
