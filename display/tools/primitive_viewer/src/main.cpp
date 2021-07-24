/*******************************************************************************
Description : Example - window creation + main loop
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
# include "../../../_img/resources_win32/resources.h"
# define _SYSTEM_STR(str) L"" str
#else
# define _SYSTEM_STR(str) str
#endif

using namespace pandora::video;

// create main window
std::unique_ptr<Window> createWindow() { // throws on failure
# ifdef _WINDOWS
    auto mainIcon = WindowResource::buildIconFromPackage(MAKEINTRESOURCE(IDI_LOGO_ICON));
# else
    auto mainIcon = WindowResource::buildIconFromPackage("pad.png");
# endif
  
  Window::Builder builder;
  return builder.setDisplayMode(WindowType::window, WindowBehavior::globalContext, ResizeMode::resizable|ResizeMode::homothety)
         .setSize(800,600)
         .setPosition(Window::Builder::centeredPosition(), Window::Builder::centeredPosition())
         .setIcon(mainIcon)
         .setBackgroundColor(WindowResource::buildColorBrush(WindowResource::rgbColor(0,0,0)))
         .create(_SYSTEM_STR("PSX_PRIMITIVE_VIEWER"), _SYSTEM_STR("PlayStation Primitive Viewer"));
}


// -- handlers -- --------------------------------------------------------------

// window/hardware event handler
bool onWindowEvent(Window* sender, WindowEvent event, uint32_t status, int32_t posX, int32_t posY, void* data) {
  return false;
}

// size/position event handler
bool onPositionEvent(Window* sender, PositionEvent event, int32_t posX, int32_t posY, uint32_t sizeX, uint32_t sizeY) {
  return false;
}

// keyboard event handler
bool onKeyboardEvent(Window* sender, KeyboardEvent event, uint32_t keyCode, uint32_t change) {
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
bool onMouseEvent(Window* sender, MouseEvent event, int32_t x, int32_t y, int32_t index, uint8_t activeKeys) {
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
  int APIENTRY WinMain(_In_ HINSTANCE appInstance, _In_opt_ HINSTANCE, _In_ LPSTR cmdLine, _In_ int cmdShow) {
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