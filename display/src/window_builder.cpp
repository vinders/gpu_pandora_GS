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
#include "display/window_builder.h"

using namespace config;
using namespace display;
using namespace pandora::hardware;
using namespace pandora::video;


std::unique_ptr<Window> WindowBuilder::build(pandora::video::WindowHandle parentWindow, void* moduleInstance,
                                             pandora::hardware::DisplayMode& outDisplayMode) {
  // identify target monitor
  auto monitor = std::make_shared<DisplayMonitor>(this->_windowConfig.monitorId, true);
  pandora::hardware::DisplayMode displayMode = monitor->getDisplayMode(); // get desktop resolution + rate

  // configure window
  Window::Builder builder;
  if (this->_windowConfig.windowMode != WindowMode::window) {
    if (this->_windowConfig.windowMode == WindowMode::fullscreen) { // fullscreen
      if (_windowConfig.fullscreen.width != desktopResolution() && _windowConfig.fullscreen.height != desktopResolution()
      &&  _windowConfig.fullscreen.width <= displayMode.width) { // not desktop resolution + supported mode -> set size
        displayMode.width = this->_windowConfig.fullscreen.width;
        displayMode.height = this->_windowConfig.fullscreen.height;
        displayMode.refreshRate = this->_windowConfig.fullscreen.refreshRate;
      }
      builder.setDisplayMode(WindowType::fullscreen, WindowBehavior::globalContext|WindowBehavior::topMost, ResizeMode::fixed);
    }
    else { // fullscreen-window
      builder.setDisplayMode(WindowType::borderless, WindowBehavior::globalContext | WindowBehavior::topMost
                                                     | WindowBehavior::aboveTaskbar, ResizeMode::fixed);
    }
    builder.setPosition(Window::Builder::defaultPosition(), Window::Builder::defaultPosition());
  }
  else { // window mode
    displayMode.height = this->_windowConfig.windowHeight;
    double width = this->_windowConfig.isWideSource ? (double)displayMode.height * 16./9. : (double)displayMode.height * 4./3.;
    displayMode.width = static_cast<uint32_t>(width + 0.500001);

    builder.setDisplayMode(WindowType::window, WindowBehavior::globalContext, ResizeMode::resizable | ResizeMode::homothety)
           .setPosition(Window::Builder::centeredPosition(), Window::Builder::centeredPosition());
  }
  outDisplayMode = displayMode;

  // create window (from emulator handle)
  builder.setParentMonitor(monitor)
         .setSize(displayMode.width, displayMode.height)
         .setRefreshRate(displayMode.refreshRate)
         .setCursor(this->_cursor)
         .setBackgroundColor(WindowResource::buildColorBrush(WindowResource::rgbColor(0,0,0)));
# ifdef _WINDOWS
    builder.setModuleInstance(moduleInstance);
# endif
  return builder.update(parentWindow, true);
}
