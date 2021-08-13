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
PSEmu Plugin Developer Kit Header definition - (C)1998 Vision Thing
This file can be used only to develop PSEmu Plugins. Other usage is highly prohibited.
*******************************************************************************/
#include <video/screensaver.h>
#include <video/message_box.h>
#include "_generated/library_info.h"
#include "config/file_path_utils.h"
#include "config/serializer.h"
#include "display/window_builder.h"
#include "psemu/syslog.h"
#include "psemu/timer.h"
#include "psemu/psemu_gpu.h"

using pandora::video::MessageBox;
using psemu::SysLog;
using psemu::Timer;

config::EmulatorInfo g_emulator;
config::UnicodeString g_configDir;
config::VideoConfig   g_videoConfig;
config::ActionsConfig g_inputConfig;

display::WindowBuilder g_windowConfigurator = nullptr;
std::unique_ptr<pandora::video::Window> g_window = nullptr;
Timer g_timer;


// -- entry point -- -----------------------------------------------------------

#ifdef _WINDOWS
# include <system/api/windows_app.h>
# include "_generated/resources.h"

  // Main library entry point (Windows)
  BOOL APIENTRY DllMain(HANDLE module, DWORD reason, LPVOID) {
    pandora::system::WindowsApp::instance().init((reason != DLL_PROCESS_DETACH) ? (HINSTANCE)module : nullptr); // attach / detach
    return TRUE;
  }
#endif


// -- plugin library info -- ---------------------------------------------------

extern "C" char* CALLBACK PSEgetLibName() {
  static char libName[] = LIBRARY_NAME;
  return libName;
}
extern "C" unsigned long CALLBACK PSEgetLibType() {
  return PSE_LT_GPU;
}
extern "C" unsigned long CALLBACK PSEgetLibVersion() {
  return (_PPDK_HEADER_VERSION << 16) | (psemu::libVersionMajor() << 8) | psemu::libVersionMinor();
}

#ifndef _WINDOWS
  extern "C" char* GPUgetLibInfos() {
    static char libName[] = LIBRARY_NAME "\nBy Romain Vinders";
    return libName;
  }
#endif


// -- driver base interface -- -------------------------------------------------

#ifdef _WINDOWS
# define __MENU_CURSOR_ID MAKEINTRESOURCEW(IDC_MENU_CURSOR)
#else
# define __MENU_CURSOR_ID "menu_cursor.png"
#endif

// Show message-box to choose config directory + create it
static config::UnicodeString createConfigDirectory(const config::UnicodeString& pluginDir) {
  bool isUserDir = true;
  if (config::isPathWritable(pluginDir.c_str())) {
    auto choice = MessageBox::show(__UNICODE_STR("First config initialization"),
                      __UNICODE_STR("Please choose where to create config files:\n\n"
                                    "* Local: shared with other emulators using PandoraGS\n"
                                    "* Portable: in emulator's executable directory"),
                      MessageBox::IconType::question,
                      __UNICODE_STR("Local"), __UNICODE_STR("Portable"));
    isUserDir = (choice != MessageBox::Result::action2);
  }

  config::UnicodeString configDir;
  if (isUserDir) {
    auto parentDir = config::getLocalUserParentDir();
    if (!config::isPathReadable(parentDir.c_str()))
      config::createDirectory(parentDir.c_str());
    configDir = config::toLocalUserConfigDir(parentDir);
  }
  else
    configDir = config::getPortableConfigDir(pluginDir);

  if (!config::createDirectory(configDir.c_str())) {
    MessageBox::show(__UNICODE_STR("Config creation failure"), __UNICODE_STR("Failed to create config directory..."),
                     MessageBox::ActionType::ok, MessageBox::IconType::error);
    throw std::runtime_error("GPUinit: config directory creation failed");
  }
  return configDir;
}


// Driver init (called once)
extern "C" long CALLBACK GPUinit() {
  try {
    config::readEmulatorInfo(g_emulator);
#   ifdef _WINDOWS
      if (g_emulator.type == config::EmulatorType::epsxe)
        ShowCursor(TRUE);
#   endif

    g_windowConfigurator = display::WindowBuilder(__MENU_CURSOR_ID);
    if (g_emulator.type != config::EmulatorType::pluginTest) {
      try {
        g_configDir = config::findConfigDir(g_emulator.pluginDir);
        if (g_configDir.empty())
          g_configDir = createConfigDirectory(g_emulator.pluginDir);
        
        SysLog::init(g_configDir); // redirect default log path to config dir
        SysLog::logDebug(__FILE_NAME__, __LINE__, "GPUinit");

        config::Serializer::readMainConfigFile(g_configDir, g_videoConfig, g_windowConfigurator.windowConfig(), g_inputConfig);
      }
      catch (const std::exception& exc) { // config file not found or corrupted
        SysLog::logWarning(__FILE_NAME__, __LINE__, exc.what());
      }
    }

    g_timer.setSpeedMode(g_videoConfig.enableFramerateLimit ? psemu::SpeedMode::normal : psemu::SpeedMode::none);
    g_timer.setFrameSkipping(g_videoConfig.frameSkip == config::FrameSkipping::adaptative);
    if (g_videoConfig.framerateLimit != config::autodetectFramerate())
      g_timer.setFrequency(g_videoConfig.framerateLimit);

    //...
    return PSE_INIT_SUCCESS;
  }
  catch (const std::exception& exc) {
    SysLog::logError(__FILE_NAME__, __LINE__, exc.what());
    return PSE_ERR_FATAL;
  }
}

// Driver shutdown (called once)
extern "C" long CALLBACK GPUshutdown() {
  SysLog::logDebug(__FILE_NAME__, __LINE__, "GPUshutdown");
  //...
  SysLog::close();
  return PSE_SUCCESS;
}

// ---

#ifdef _WINDOWS
  // Open driver (game started) - win32
  extern "C" long CALLBACK GPUopen(HWND window) {
    SysLog::logDebug(__FILE_NAME__, __LINE__, "GPUopen");
    try {
      pandora::video::disableScreenSaver();

      pandora::hardware::DisplayMode displayMode;
      g_window = g_windowConfigurator.build(window, pandora::system::WindowsApp::instance().handle(), displayMode);

      //...
      return PSE_GPU_SUCCESS;
    }
    catch (const std::exception& exc) {
      SysLog::logError(__FILE_NAME__, __LINE__, exc.what());
      return PSE_ERR_FATAL;
    }
  }
#else
  // Open driver (game started) - linux / unix
  extern "C" long CALLBACK GPUopen(unsigned long* displayId, char* caption, char* configFile) {
    return PSE_GPU_SUCCESS;
  }
#endif

// Close driver (game stopped)
extern "C" long CALLBACK GPUclose() {
  SysLog::logDebug(__FILE_NAME__, __LINE__, "GPUclose");

  g_window.reset();
  pandora::video::restoreScreenSaver();
  return PSE_SUCCESS;
}

// ---

// Display update (called on every vsync)
extern "C" void CALLBACK GPUupdateLace() {
  g_timer.waitPeriod();
}


// -- status control -- --------------------------------------------------------

// Read data from GPU status register
extern "C" unsigned long CALLBACK GPUreadStatus() {
  return 0;
}

// Process data sent to GPU status register
extern "C" void CALLBACK GPUwriteStatus(unsigned long gdata) {

}


// -- data transfers -- --------------------------------------------------------

// Get data transfer mode
extern "C" long CALLBACK GPUgetMode() {
  return 0;
}

// Set data transfer mode (deprecated)
extern "C" void CALLBACK GPUsetMode(unsigned long transferMode) {

}

// ---

// Read data from video memory (VRAM)
extern "C" unsigned long CALLBACK GPUreadData() {
  return 0;
}

// Read entire chunk of data from video memory (VRAM)
extern "C" void CALLBACK GPUreadDataMem(unsigned long* mem, int size) {

}

// ---

// Process and send data to video data register
extern "C" void CALLBACK GPUwriteData(unsigned long gdata) {

}

// Process and send chunk of data to video data register
extern "C" void CALLBACK GPUwriteDataMem(unsigned long* mem, int size) {

}

// Direct memory chain transfer to GPU driver
extern "C" long CALLBACK GPUdmaChain(unsigned long* baseAddress, unsigned long offset) {
  return PSE_SUCCESS;
}


// -- save states -- -----------------------------------------------------------

// Save/load current state
extern "C" long CALLBACK GPUfreeze(unsigned long dataMode, GPUFreeze_t* state) {
  SysLog::logDebug(__FILE_NAME__, __LINE__, "GPUfreeze: %u", dataMode);
  return SAVESTATE_SUCCESS;
}


// -- plugin dialog interface -- -----------------------------------------------

// Open plugin config dialog box
extern "C" long CALLBACK GPUconfigure() {
  return PSE_SUCCESS;
}

// Open plugin 'about' dialog box
extern "C" void CALLBACK GPUabout() {
  
}

// Check if plugin works
extern "C" long CALLBACK GPUtest() {
  return PSE_GPU_SUCCESS; // always OK
}


// -- runtime settings -- ------------------------------------------------------

// Set special display flags
extern "C" void CALLBACK GPUdisplayFlags(unsigned long flags) {
  SysLog::logDebug(__FILE_NAME__, __LINE__, "GPUdisplayFlags: 0x%x", flags);
  
}

// Enable/disable frame limit from emulator
extern "C" void CALLBACK GPUsetframelimit(unsigned long option) {
  SysLog::logDebug(__FILE_NAME__, __LINE__, "GPUsetframelimit: %u", option);
  
}

// Set custom fixes from emulator
extern "C" void CALLBACK GPUsetfix(unsigned long fixBits) {
  SysLog::logDebug(__FILE_NAME__, __LINE__, "GPUsetfix: 0x%x", fixBits);

}

// Set game executable ID (for config profiles associations)
extern "C" void CALLBACK GPUsetExeName(char* gameId) {
  SysLog::logDebug(__FILE_NAME__, __LINE__, "GPUsetExeName: %s", gameId);

}


// -- snapshots -- -------------------------------------------------------------

// Request snapshot (on next display)
extern "C" void CALLBACK GPUmakeSnapshot() {
  SysLog::logDebug(__FILE_NAME__, __LINE__, "GPUmakeSnapshot");

}

// Get screen picture
extern "C" void CALLBACK GPUgetScreenPic(unsigned char* image) {
  SysLog::logDebug(__FILE_NAME__, __LINE__, "GPUgetScreenPic");

}

// Store and display screen picture
extern "C" void CALLBACK GPUshowScreenPic(unsigned char* image) {
  SysLog::logDebug(__FILE_NAME__, __LINE__, "GPUshowScreenPic");
}


// -- miscellaneous -- ---------------------------------------------------------

// Display debug text
extern "C" void CALLBACK GPUdisplayText(char* message) {
  SysLog::logDebug(__FILE_NAME__, __LINE__, "GPUdisplayText: %s", message);

}

// Set gun cursor display and position
extern "C" void CALLBACK GPUcursor(int player, int x, int y) {

}

// Trigger screen vibration
extern "C" void CALLBACK GPUvisualVibration(unsigned long smallRumble, unsigned long bigRumble) {

}


// -- other implementation files -- --------------------------------------------

#include "./psemu_zinc.hpp" // include -> single object file (less overhead + smaller lib size)
#include "./timer.hpp"
