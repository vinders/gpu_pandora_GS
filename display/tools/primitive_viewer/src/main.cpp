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
--------------------------------------------------------------------------------
Description : PS1 primitive viewer tool
*******************************************************************************/
#include <cstdint>
#include <thread>
#include <chrono>
#include <memory>
#include <stdexcept>
#include <video/message_box.h>
#include <video/window_keycodes.h>
#include <video/window.h>

#if defined(_WINDOWS)
# include <system/api/windows_api.h>
# include <system/api/windows_app.h>
# include "_generated/resources.h"
# define _SYSTEM_STR(str) L"" str
#else
# define _SYSTEM_STR(str) str
#endif

using namespace pandora::video;

// create main window
std::unique_ptr<Window> createWindow() { // throws on failure
  Window::Builder builder;
  return builder.setDisplayMode(WindowType::window, WindowBehavior::globalContext, ResizeMode::resizable|ResizeMode::homothety)
         .setSize(800,600)
         .setPosition(Window::Builder::centeredPosition(), Window::Builder::centeredPosition())
         .setIcon(WindowResource::buildIcon(pandora::video::SystemIcon::info))
#        ifdef _WINDOWS
           .setCursor(WindowResource::buildCursorFromPackage(MAKEINTRESOURCE(IDC_MENU_CURSOR)))
#        else
           .setCursor(WindowResource::buildCursorFromPackage("menu_cursor.png"))
#        endif
         .setBackgroundColor(WindowResource::buildColorBrush(WindowResource::rgbColor(0,0,0)))
         .create(_SYSTEM_STR("PSX_PRIMITIVE_VIEWER"), _SYSTEM_STR("PlayStation Primitive Viewer"));
}


// -- handlers -- --------------------------------------------------------------

// window/hardware event handler
bool onWindowEvent(Window*, WindowEvent, uint32_t, int32_t, int32_t, void*) {
  return false;
}

// size/position event handler
bool onPositionEvent(Window*, PositionEvent, int32_t, int32_t, uint32_t, uint32_t) {
  return false;
}

// keyboard event handler
bool onKeyboardEvent(Window* sender, KeyboardEvent event, uint32_t keyCode, uint32_t) {
  switch (event) {
    case KeyboardEvent::keyDown: {
      if (keyCode == _P_VK_ESC)
        Window::sendCloseEvent(sender->handle());
      break;
    }
    default: break;
  }
  return false;
}

// mouse event handler
bool onMouseEvent(Window*, MouseEvent, int32_t, int32_t, int32_t, uint8_t) {
  return false;
}


// -- main loop -- -------------------------------------------------------------

inline void mainAppLoop() {
  try {
    auto window = createWindow();
    window->setMinClientAreaSize(400, 300);
    
    window->setWindowHandler(&onWindowEvent);
    window->setPositionHandler(&onPositionEvent);
    window->setKeyboardHandler(&onKeyboardEvent);
    window->setMouseHandler(&onMouseEvent, Window::CursorMode::visible);
    window->show();
    
    while (Window::pollEvents()) {
      std::this_thread::sleep_for(std::chrono::microseconds(16666LL)); // 60Hz
    }
  }
  catch (const std::exception& exc) {
    MessageBox::flushEvents();
    MessageBox::show("Fatal error", exc.what(), MessageBox::ActionType::ok, MessageBox::IconType::error, true);
    exit(-1);
  }
}


// -- entry point -- -----------------------------------------------------------

#if defined(_WINDOWS)
  int APIENTRY WinMain(_In_ HINSTANCE appInstance, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {
    pandora::system::WindowsApp::instance().init((pandora::system::AppInstanceHandle)appInstance);
    mainAppLoop();
    return 0;
  }
#else
  int main() {
    mainAppLoop();
    return 0;
  }
#endif