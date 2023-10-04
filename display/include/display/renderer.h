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

#include <array>
#include <video/window.h>
#include "config/config.h"
#include "display/types.h"
#include "display/viewport.h"
#if defined(_WINDOWS) && defined(_VIDEO_D3D11_SUPPORT)
# include <video/d3d11/renderer.h>
# include <video/d3d11/graphics_pipeline.h>
# include <video/d3d11/depth_stencil_buffer.h>
# include <video/d3d11/sampler.h>
# include <video/d3d11/texture.h>
  namespace video_api = pandora::video::d3d11;
#else
# include <video/vulkan/renderer.h>
# include <video/vulkan/depth_stencil_buffer.h>
# include <video/vulkan/sampler.h>
# include <video/vulkan/texture.h>
  namespace video_api = pandora::video::vulkan;
#endif

namespace display {
  class Renderer final {
  public:
    /// @brief Create display pipeline (initialize size-independant components)
    /// @param monitor  Display monitor on which the output window is located.
    /// @warning 'openWindow' should be called with window/size params before drawing anything.
    /// @throws invalid_argument/runtime_error/bad_alloc on failure.
    Renderer(const pandora::hardware::DisplayMonitor& monitor, config::RendererProfile&& settings);
    void release() noexcept; ///< Destroy display pipeline

    Renderer() = default; ///< Empty instance (verify with 'isRunning()')
    Renderer(const Renderer&) = delete;
    Renderer(Renderer&&) noexcept = default;
    Renderer& operator=(const Renderer&) = delete;
    Renderer& operator=(Renderer&&) noexcept = default;
    ~Renderer() noexcept { release(); }

    // -- accessors --

    /// @brief Verify if current instance is valid (true) or empty (false)
    inline bool isRunning() const noexcept { return (this->_renderer != nullptr); }
    /// @brief Read renderer profile settings
    inline const config::RendererProfile& configProfile() const noexcept { return this->_settings; }

    // -- set display pipeline components --

    /// @brief Create window/size-dependant rendering components
    void openWindow(pandora::video::Window& window, const config::WindowConfig& windowConfig,
                    const pandora::hardware::DisplayMode& outputArea, unsigned long vramHeight,
                    const DisplayState& psxDisplayState, bool useVsync);
    void closeWindow() noexcept;

    void onWindowSizeChange(pandora::video::Window& window, const config::WindowConfig& windowConfig);
    void onProfileChange(config::RendererProfile&& settings);

    // -- operations --

    void swapBuffers() { this->_swapChain.swapBuffers(); }


  private:
    std::shared_ptr<pandora::video::d3d11::Renderer> _renderer = nullptr;
    video_api::SwapChain _swapChain;
    
    video_api::TextureTarget2D _internalFramebuffers;
    uint32_t _internalResX = 1;
    uint32_t _internalResY = 1;

    config::RendererProfile _settings;
    pandora::hardware::DisplayMode _outputMode{ 0,0,32,60 };
    video_api::Viewport _outputViewport;

    video_api::DepthStencilBuffer _depthBuffer;
    video_api::RasterizerState _rasterizerState;
    std::array<video_api::BlendState,4> _blendStates;
    video_api::Sampler _sampler;
  };
}
