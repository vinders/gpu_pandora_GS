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
#include "display/renderer.h"

using namespace display;


Renderer::Renderer(pandora::video::WindowHandle, const pandora::hardware::DisplayMode&,
                   const Viewport&, const config::RendererProfile&) {

}
Renderer::~Renderer() noexcept {

}

// ---

void Renderer::changeConfig(const config::RendererProfile&) {

}
void Renderer::resize(const pandora::hardware::DisplayMode&, const Viewport&) {

}

void Renderer::swapBuffers(bool) {

}
