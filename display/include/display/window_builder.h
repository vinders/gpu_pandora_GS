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

#include <memory>
#include <video/window.h>
#include <video/window_resource.h>
#include "config/config.h"

#ifdef _WINDOWS
# define __PACKAGE_RES_ID wchar_t*
#else
# define __PACKAGE_RES_ID char*
#endif

namespace display {
  /// @brief Window builder: window config container, window resource container, window creator
  class WindowBuilder final {
  public:
    /// @brief Create window builder (window config + resource container)
    /// @param cursorResource  Custom cursor resource ID/path (or NULL to keep default cursor)
    WindowBuilder(const __PACKAGE_RES_ID cursorResource = nullptr)
      : _cursor((cursorResource != nullptr) ? pandora::video::WindowResource::buildIconFromPackage(cursorResource) : nullptr) {}

    WindowBuilder(const WindowBuilder&) = default;
    WindowBuilder(WindowBuilder&&) = default;
    WindowBuilder& operator=(const WindowBuilder&) = default;
    WindowBuilder& operator=(WindowBuilder&&) = default;
    
    /// @brief Get reference to window settings: can be edited
    config::WindowConfig& windowConfig() noexcept { return _windowConfig; }
    /// @brief Read window settings
    const config::WindowConfig& windowConfig() const noexcept { return _windowConfig; }
    
    // -- operations --
  
    /// @brief Create output window
    /// @param parentWindow    Handle to emulator window (turned into output window)
    /// @param moduleInstance  DLL/library module instance (required on Windows)
    /// @throws invalid_argument/runtime_error on failure.
    std::unique_ptr<pandora::video::Window> build(pandora::video::WindowHandle parentWindow, void* moduleInstance);
    
  private:
    config::WindowConfig _windowConfig;
    std::shared_ptr<pandora::video::WindowResource> _cursor;
  };
}
