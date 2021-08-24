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
#pragma once

# include "config/config.h"
# include "display/viewport.h"
#if defined(_WINDOWS) && defined(_VIDEO_D3D11_SUPPORT)
# include <video/d3d11/renderer.h>
# include <video/d3d11/depth_stencil_buffer.h>
# include <video/d3d11/texture.h>
  namespace renderer_api = pandora::video::d3d11;
#else
# include <video/vulkan/renderer.h>
# include <video/vulkan/depth_stencil_buffer.h>
# include <video/vulkan/texture.h>
  namespace renderer_api = pandora::video::vulkan;
#endif

namespace display {
  class Renderer final {
  public:
    Renderer(pandora::video::WindowHandle handle, const pandora::hardware::DisplayMode& displayMode,
              const Viewport& viewport, const config::RendererProfile& config);
    ~Renderer() noexcept;

    Renderer() = default;
    Renderer(const Renderer&) = delete;
    Renderer(Renderer&&) noexcept = default;
    Renderer& operator=(const Renderer&) = delete;
    Renderer& operator=(Renderer&&) noexcept = default;

    void changeConfig(const config::RendererProfile& config);
    void resize(const pandora::hardware::DisplayMode& displayMode, const Viewport& viewport);

    void swapBuffers(bool useVsync);

    const config::RendererProfile& configProfile() const noexcept { return this->_config; }
      
  private:
    std::shared_ptr<pandora::video::d3d11::Renderer> _renderer = nullptr;
    renderer_api::SwapChain _swapChain;
    renderer_api::DepthStencilBuffer _depthBuffer;
    renderer_api::RasterizerState _rasterizerState;
    renderer_api::FilterStateArray _filterStates;
    renderer_api::BlendStateArray<4> _blendStates;
    renderer_api::Viewport _viewport;
    config::RendererProfile _config;
  };
}
