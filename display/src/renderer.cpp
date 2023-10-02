/*******************************************************************************
Pandora GS - PSEmu-compatible GPU driver
Copyright (C) 2021  Romain Vinders

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation, version 2 of the License.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details (LICENSE file).
*******************************************************************************/
#include <cstring>
#if defined(_WINDOWS) && defined(_VIDEO_D3D11_SUPPORT)
# include <video/d3d11/renderer_state_factory.h>
#else
# include <video/vulkan/renderer_state_factory.h>
#endif
#include "display/renderer.h"

using namespace display;
using video_api::BlendFactor;
using video_api::BlendOp;
using video_api::BlendParams;
using video_api::DataFormat;
using video_api::FilterParams;
using video_api::RasterizerParams;
using video_api::SwapChain;
using video_api::TextureFilter;
using video_api::TextureWrap;


// -- helpers -- ---------------------------------------------------------------

// auto-detect internal resolution factor, based on output resolution
template <uint32_t _BaseSize>
static inline uint32_t __computeInternalResFactor(uint32_t clientSize, uint32_t internalResParam) noexcept {
  if (internalResParam == config::autodetectInternalResFactor()) {
    internalResParam = (clientSize + (_BaseSize >> 1)) / _BaseSize; // round(output/source) => (output + source/2)/source
    if (internalResParam > config::maxInternalResFactor())
      internalResParam = config::maxInternalResFactor();
  }
  return internalResParam;
}


// -- set display pipeline components -- ---------------------------------------

Renderer::Renderer(const pandora::hardware::DisplayMonitor& monitor, config::RendererProfile&& settings) 
  : _renderer(std::make_shared<video_api::Renderer>(monitor)),
    _settings(std::move(settings)) {
  video_api::RendererStateFactory factory(*_renderer);

  auto fillMode = (settings.fillMode != config::FillMode::wireframe) ? video_api::FillMode::fill : video_api::FillMode::lines;
  this->_rasterizerState = factory.createRasterizerState(RasterizerParams(video_api::CullMode::none, fillMode, true, false, false));

  TextureWrap texWrap[3] = { TextureWrap::repeatMirror, TextureWrap::repeatMirror, TextureWrap::repeatMirror };
  FilterParams nearestFilter(TextureFilter::nearest, TextureFilter::nearest, TextureFilter::nearest, texWrap);
  FilterParams bilinearFilter(TextureFilter::linear, TextureFilter::linear, TextureFilter::nearest, texWrap);
  this->_filterStates.clear();
  this->_filterStates.append(factory.createFilterState(settings.useTextureBilinear ? bilinearFilter : nearestFilter)); // textures
  this->_filterStates.append(factory.createFilterState(settings.useSpriteBilinear ? bilinearFilter : nearestFilter));  // sprites
  this->_filterStates.append(factory.createFilterState(nearestFilter)); // screen
  
  this->_blendStates.append(factory.createBlendState(BlendParams(BlendFactor::sourceAlpha, BlendFactor::sourceInvAlpha, BlendOp::add,
                                                                 BlendFactor::one, BlendFactor::zero, BlendOp::add))); // no blending / mean (with alpha*0.5)
  this->_blendStates.append(factory.createBlendState(BlendParams(BlendFactor::one, BlendFactor::one,  BlendOp::add,
                                                                 BlendFactor::one, BlendFactor::zero, BlendOp::add))); // add
  this->_blendStates.append(factory.createBlendState(BlendParams(BlendFactor::one, BlendFactor::one,  BlendOp::revSubtract,
                                                                 BlendFactor::one, BlendFactor::zero, BlendOp::add))); // subtract
  this->_blendStates.append(factory.createBlendState(BlendParams(BlendFactor::sourceAlpha, BlendFactor::one, BlendOp::add,
                                                                 BlendFactor::one, BlendFactor::zero, BlendOp::add))); // add quarter (with alpha*0.25)
}

void Renderer::release() noexcept {
  if (this->_renderer != nullptr) {
    this->_renderer.reset();
  }
}

// ---



void Renderer::openWindow(pandora::video::Window& window, const config::WindowConfig& windowConfig,
                          const pandora::hardware::DisplayMode& outputMode, unsigned long vramHeight,
                          const DisplayState& psxDisplayState) {
  this->_internalResX = __computeInternalResFactor<320>(outputMode.width, this->_settings.internalResFactorX);
  this->_internalResY = __computeInternalResFactor<240>(outputMode.height, this->_settings.internalResFactorY);

  // PS1 framebuffers: width=factor*1024 -- height=factor*512 (psx) -- height=factor*1024 (arcade)
  auto internalVramWidth = this->_internalResX << 10;
  auto internalVramHeight = (vramHeight == psxVramHeight()) ? this->_internalResX << 9 : this->_internalResX << 10;
  video_api::Texture2DParams framebufferParams(internalVramWidth, internalVramHeight, DataFormat::rgba8_sRGB,
                                               1, 1, 0, video_api::ResourceUsage::staticGpu);
  this->_internalFramebuffers = video_api::TextureTarget2D(*_renderer, framebufferParams, nullptr, true);

  // output framebuffer
  this->_outputMode = outputMode;
  this->_swapChain = SwapChain(this->_renderer, window.handle(),
                               SwapChain::Descriptor(internalVramWidth, internalVramHeight, 2, outputMode.refreshRate),
                               DataFormat::rgba8_sRGB);
  _renderer->setCleanActiveRenderTarget(this->_swapChain.getRenderTargetView(), nullptr);
  _renderer->setRasterizerState(this->_rasterizerState);
  _renderer->setBlendState(this->_blendStates.get()[0]);

  // output viewport
  auto viewportWidth = this->_internalResX * psxDisplayState.displayAreaSize.x;
  auto viewportHeight = this->_internalResY * psxDisplayState.displayAreaSize.y;


  _renderer->setViewport(this->_displayArea);//fractional sizes???

  /*
  //---
      auto internalWidth = this->_internalResX * psxDisplayArea.width;
      auto internalHeight = this->_internalResY * psxDisplayArea.height;

      this->_outputMode = outputMode;
      if (windowConfig.windowMode == config::WindowMode::window) {
        auto windowViewport = display::Viewport(outputMode.height, windowConfig.isWideSource);
        window.setMinClientAreaSize(windowViewport.minWindowWidth(), windowViewport.minWindowHeight());
        this->_displayArea = video_api::Viewport(0, 0, internalWidth, internalHeight);
      }
      else { // fullscreen
        auto stretchedViewport = display::Viewport(outputMode, this->_settings.screenStretching, this->_settings.screenCropping,
                                                   windowConfig.isWideSource);
        if (outputMode.width != stretchedViewport.scaledSourceWidth() || outputMode.height != stretchedViewport.scaledSourceHeight()) {
          // internal "window" size = internal framebuffer size * (output window size / output image size)
          int32_t internalWindowX = internalWidth * outputMode.width / stretchedViewport.scaledSourceWidth();
          int32_t internalWindowY = internalHeight * outputMode.height / stretchedViewport.scaledSourceHeight();
          this->_displayArea = video_api::Viewport(internalWindowX - internalWidth, internalWindowY - internalHeight,
                                                   internalWindowX, internalWindowY);
        }
        else
          this->_displayArea = video_api::Viewport(0, 0, internalWidth, internalHeight);
      }
  //---*/
}

void Renderer::closeWindow() noexcept {
  this->_swapChain.release();
}

// ---

void Renderer::onWindowSizeChange(pandora::video::Window& window, const config::WindowConfig& windowConfig) {
  auto windowSize = window.getClientSize();
  if (windowSize.width == this->_outputMode.width && windowSize.height == this->_outputMode.height)
    return;

  auto displayMode = this->_outputMode;
  displayMode.width = windowSize.width;
  displayMode.height = windowSize.height;
  closeWindow();
  openWindow(window, windowConfig, displayMode);
}

void Renderer::onProfileChange(config::RendererProfile&& settings) {
  //TODO: reload shaders/buffers if necessary
  memcpy(&(this->_settings), &settings, sizeof(config::RendererProfile));
}


// -- operations -- ------------------------------------------------------------

void Renderer::swapBuffers(bool useVsync) {
  this->_swapChain.swapBuffersDiscard(useVsync);
}
