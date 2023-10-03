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
// no include limit -> different include may use different usage options

#if defined(_WINDOWS) && defined(_VIDEO_D3D11_SUPPORT)
# include <video/d3d11/renderer.h>
# include <video/d3d11/texture.h>
# include <video/d3d11/buffer.h>
# ifdef USE_SHADERS
# include <video/d3d11/shader.h>
# endif
namespace video_api = pandora::video::d3d11;
#else
# include <video/vulkan/renderer.h>
# include <video/vulkan/texture.h>
# include <video/vulkan/buffer.h>
# ifdef USE_SHADERS
# include <video/vulkan/shader.h>
# endif
namespace video_api = pandora::video::vulkan;
#endif
