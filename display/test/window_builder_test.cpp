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
#include <gtest/gtest.h>
#include <display/window_builder.h>

using namespace display;

class WindowBuilderTest : public testing::Test {
public:
protected:
  //static void SetUpTestCase() {}
  //static void TearDownTestCase() {}

  void SetUp() override {}
  void TearDown() override {}
};

#ifdef _WINDOWS
#include <system/api/windows_app.h>
#include <Windows.h>
# define __GET_MODULE GetModuleHandle(NULL)
# define __SYSTEM_STR(str) L"" str
#else
# define __GET_MODULE nullptr
# define __SYSTEM_STR(str) str
#endif


TEST_F(WindowBuilderTest, windowBuild) {
  WindowBuilder builder;
  EXPECT_ANY_THROW(builder.build((pandora::video::WindowHandle)0, nullptr));
  auto moduleInstance = __GET_MODULE;

  // parent window
# ifdef _WINDOWS // can't use toolbox Window as parent with 'callParentEventProc'=true
    pandora::system::WindowsApp::instance().init(moduleInstance);

    const wchar_t* contextName = L"TEST_WINDOW0";
    WNDCLASSEXW windowClass;
    ZeroMemory(&windowClass, sizeof(WNDCLASSEXW));
    windowClass.cbSize = sizeof(WNDCLASSEXW);
    windowClass.lpszClassName = contextName;
    windowClass.hInstance = moduleInstance;
    windowClass.lpfnWndProc = DefWindowProcW;
    RegisterClassExW(&windowClass);
    auto parentHandle = CreateWindowExW(0, contextName, L"Emulator", WS_OVERLAPPEDWINDOW|WS_POPUP, 200, 120, 640, 480, nullptr, nullptr, moduleInstance, nullptr);
    ShowWindow(parentHandle, SW_NORMAL);
# else
    auto parentWindow = pandora::video::Window::Builder{}.create(__SYSTEM_STR("TEST_WINDOW0"), __SYSTEM_STR("Emulator"));
    parentWindow->show();
    auto parentHandle = parentWindow->handle();
# endif

  // fullscreen
  builder.windowConfig().windowMode = config::WindowMode::fullscreen;
  auto outputWindow = builder.build(parentHandle, moduleInstance);
  outputWindow->pollEvents();
  outputWindow.reset();

  // fullscreen-window
  builder.windowConfig().windowMode = config::WindowMode::fullscreenWindow;
  outputWindow = builder.build(parentHandle, moduleInstance);
  outputWindow->pollEvents();
  outputWindow.reset();

  // window
  builder.windowConfig().windowMode = config::WindowMode::window;
  builder.windowConfig().windowHeight = 720;
  outputWindow = builder.build(parentHandle, moduleInstance);
  outputWindow->pollEvents();
  outputWindow.reset();

# ifdef _WINDOWS
    UnregisterClassW(contextName, windowClass.hInstance);
# endif
}
