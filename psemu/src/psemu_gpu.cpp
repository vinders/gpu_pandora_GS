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
#include "config/presets.h"
#include "display/status_register.h"
#include "display/window_builder.h"
#include "display/renderer.h"
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
pandora::memory::LightString g_gameId;

display::WindowBuilder g_windowConfigurator = nullptr;
std::unique_ptr<pandora::video::Window> g_window = nullptr;
display::Renderer g_renderer;
display::StatusRegister g_statusRegister;
unsigned long g_statusControlHistory[display::controlCommandNumber()];
Timer g_timer;


// -- entry point / directory management -- ------------------------------------

#ifdef _WINDOWS
# include <system/api/windows_app.h>
# include "_generated/resources.h"

  // Main library entry point (Windows)
  BOOL APIENTRY DllMain(HANDLE module, DWORD reason, LPVOID) {
    pandora::system::WindowsApp::instance().init((reason != DLL_PROCESS_DETACH) ? (HINSTANCE)module : nullptr); // attach / detach
    return TRUE;
  }
#endif

// ---

// Show message-box to choose config directory + create it
static config::UnicodeString createConfigDirectory(const config::UnicodeString& pluginDir) {
  bool isUserDir = true;
  if (config::isPathWritable(pluginDir.c_str())) {
    auto choice = MessageBox::show(__UNICODE_STR("First config initialization"),
                      __UNICODE_STR("Please choose where to create config files:\n\n"
                                    "* Local: shared with other emulators (per user)\n"
                                    "* Portable: in emulator's directory"),
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

// Driver init (called once)
extern "C" long CALLBACK GPUinit() {
  try {
    // identify emulator
    config::readEmulatorInfo(g_emulator);

    // identify config directory (or ask for location + create it)
    g_windowConfigurator = display::WindowBuilder(__MENU_CURSOR_ID);
    try {
      g_configDir = config::findConfigDir(g_emulator.pluginDir);
      if (g_configDir.empty())
        g_configDir = createConfigDirectory(g_emulator.pluginDir);
    }
    catch (const std::exception& exc) { SysLog::logError(__FILE_NAME__, __LINE__, exc.what()); }

    SysLog::init(g_configDir); // redirect default log path to config dir
    SysLog::logDebug(__FILE_NAME__, __LINE__, "GPUinit");

    // try to load global config (on failure, use default config)
    try {
      config::Serializer::readGlobalConfigFile(g_configDir, g_videoConfig, g_windowConfigurator.windowConfig(), g_inputConfig);
    }
    catch (const std::exception& exc) {
      if (!g_configDir.empty() && !config::isPathReadable(g_configDir + config::globalConfigFileName())) { // not found -> create it
        g_inputConfig.initDefaultMapping();
        config::Serializer::writeGlobalConfigFile(g_configDir, g_videoConfig, g_windowConfigurator.windowConfig(), g_inputConfig);
      }
      else // file corrupted or alloc failure
        SysLog::logError(__FILE_NAME__, __LINE__, exc.what());
    }

    g_statusRegister = display::StatusRegister{};
    display::StatusRegister::resetControlCommandHistory(g_statusControlHistory);
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

// Open driver (game started)
#ifdef _WINDOWS
extern "C" long CALLBACK GPUopen(HWND window) {
#else
extern "C" long CALLBACK GPUopen(unsigned long* displayId, char* caption, char* configFile) {
#endif
  SysLog::logDebug(__FILE_NAME__, __LINE__, "GPUopen");
  try {
    // load list of renderer config profiles
    std::vector<config::ProfileMenuTile> profiles;
    try {
      config::Serializer::readProfileListFile(g_configDir, profiles);
    }
    catch (const std::exception& exc) {
      if (!g_configDir.empty() && !config::isPathReadable(g_configDir + config::profileListFileName())) // not found -> create it
        config::Serializer::writeProfileListFile(g_configDir, std::vector<config::ProfileLabel>{});
      else // file corrupted
        SysLog::logError(__FILE_NAME__, __LINE__, exc.what());
    }

    // try to load profile (associated with game ID, if available)
    config::RendererProfile rendererConfig;
    if (g_gameId == "DEMO_999.99") { // psxtest_gpu -> use accurate settings
      config::loadPreset(config::PresetId::psxAccurate, rendererConfig);
      g_windowConfigurator.windowConfig().windowMode = config::WindowMode::window;
    }
    else if (!profiles.empty()) { // normal game -> load profile associated with game
      try {
        auto targetId = config::Serializer::readGameProfileBinding(g_configDir, g_gameId.c_str());
        if (targetId & __CONFIG_PRESET_FLAG) { // target is a preset
          config::loadPreset((config::PresetId)targetId, rendererConfig);
        }
        else { // target is a profile
          config::ProfileMenuTile* targetProfile = &profiles[0]; // default to first profile (if target not found)
          for (auto& it : profiles) {
            if (it.id == targetId) {
              targetProfile = &it;
              break;
            }
          }
          config::Serializer::readProfileConfigFile(targetProfile->file, rendererConfig);
        }
      }
      catch (const std::exception& exc) {
        if (!profiles.empty()) // corrupted profile or alloc failure
          SysLog::logError(__FILE_NAME__, __LINE__, exc.what());
      }
    }

    // create output window
    pandora::hardware::DisplayMode displayMode;
    config::readEmulatorOptions(g_emulator);
    g_windowConfigurator.windowConfig().isWideSource = g_emulator.widescreenHack;
#   ifdef _WINDOWS
      if (g_emulator.isCursorHidden)
        ShowCursor(TRUE);
      g_window = g_windowConfigurator.build(window, pandora::system::WindowsApp::instance().handle(), displayMode);
#   else
      g_window = pandora::video::Window::Builder{}.create("PGS_WINDOW", caption);
#   endif
    g_window->clearClientArea();
    pandora::video::disableScreenSaver();

    // create 3D renderer
    display::Viewport viewport = (g_windowConfigurator.windowConfig().windowMode == config::WindowMode::window)
                               ? display::Viewport(displayMode.height, g_windowConfigurator.windowConfig().isWideSource)
                               : display::Viewport(displayMode, rendererConfig.screenStretching, rendererConfig.screenCropping,
                                                   g_windowConfigurator.windowConfig().isWideSource);
    g_window->setMinClientAreaSize(viewport.minWindowWidth(), viewport.minWindowHeight());
    g_renderer = display::Renderer(g_window->handle(), displayMode, viewport, rendererConfig);

    // configure sync timer
    g_timer.setSpeedMode(g_videoConfig.enableFramerateLimit ? psemu::SpeedMode::normal : psemu::SpeedMode::none);
    g_timer.setFrameSkipping(g_videoConfig.frameSkip == config::FrameSkipping::adaptative);
    if (g_videoConfig.framerateLimit != config::autodetectFramerate())
      g_timer.setFrequency(g_videoConfig.framerateLimit);

    // set event handlers
    /*g_window->setWindowHandler(...);
    g_window->setPositionHandler(...);
    g_window->setKeyboardHandler(...);
    g_window->setMouseHandler(...);*/
    return PSE_GPU_SUCCESS;
  }
  catch (const std::exception& exc) {
    SysLog::logError(__FILE_NAME__, __LINE__, exc.what());
    return PSE_ERR_FATAL;
  }
}

// Close driver (game stopped)
extern "C" long CALLBACK GPUclose() {
  SysLog::logDebug(__FILE_NAME__, __LINE__, "GPUclose");
  g_renderer = display::Renderer{};

  pandora::video::restoreScreenSaver();
# ifdef _WINDOWS
    if (g_emulator.isCursorHidden)
      ShowCursor(FALSE);
# endif
  g_window.reset();
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
  return g_statusRegister.getStatusControlRegister();
}

// Process data sent to GPU status register - GP1 commands
extern "C" void CALLBACK GPUwriteStatus(unsigned long gdata) {
  auto commandId = display::StatusRegister::getGp1CommandId(gdata);
  switch (commandId) {
    // general GPU status
    case display::ControlCommandId::resetGpu: {
      SysLog::logDebug(__FILE_NAME__, __LINE__, "GP1(00): reset");
      g_statusRegister.resetGpu();
      display::StatusRegister::resetControlCommandHistory(g_statusControlHistory);
      if (g_videoConfig.framerateLimit == config::autodetectFramerate())
        g_timer.setFrequency(display::SmpteStandard::ntsc, false);
      break;
    }
    case display::ControlCommandId::clearCommandFifo: g_statusRegister.clearPendingCommands(); break;
    case display::ControlCommandId::ackIrq1:          g_statusRegister.ackIrq1(); break;
    case display::ControlCommandId::dmaMode: {
      g_statusControlHistory[(size_t)display::ControlCommandId::dmaMode] = gdata;
      g_statusRegister.setDmaMode(gdata);
      break;
    }
    // display state
    case display::ControlCommandId::toggleDisplay: {
      if (g_statusControlHistory[(size_t)display::ControlCommandId::toggleDisplay] != gdata) {
        SysLog::logDebug(__FILE_NAME__, __LINE__, "GP1(03): toggleDisplay: 0x%x", gdata);
        g_statusControlHistory[(size_t)display::ControlCommandId::toggleDisplay] = gdata;
        g_statusRegister.toggleDisplay(gdata);
      }
      break;
    }
    case display::ControlCommandId::displayAreaOrigin: {
      if (g_statusControlHistory[(size_t)display::ControlCommandId::displayAreaOrigin] != gdata) {
        g_statusControlHistory[(size_t)display::ControlCommandId::displayAreaOrigin] = gdata;
        g_statusRegister.setDisplayAreaOrigin(gdata);
      }
      break;
    }
    case display::ControlCommandId::horizontalDisplayRange: {
      if (g_statusControlHistory[(size_t)display::ControlCommandId::horizontalDisplayRange] != gdata) {
        SysLog::logDebug(__FILE_NAME__, __LINE__, "GP1(06): horizontalDisplayRange: 0x%x", gdata);
        g_statusControlHistory[(size_t)display::ControlCommandId::horizontalDisplayRange] = gdata;
        g_statusRegister.setHorizontalDisplayRange(gdata);
      }
      break;
    }
    case display::ControlCommandId::verticalDisplayRange: {
      if (g_statusControlHistory[(size_t)display::ControlCommandId::verticalDisplayRange] != gdata) {
        SysLog::logDebug(__FILE_NAME__, __LINE__, "GP1(07): verticalDisplayRange: 0x%x", gdata);
        g_statusControlHistory[(size_t)display::ControlCommandId::verticalDisplayRange] = gdata;
        g_statusRegister.setVerticalDisplayRange(gdata);
      }
      break;
    }
    case display::ControlCommandId::displayMode: {
      if (g_statusControlHistory[(size_t)display::ControlCommandId::displayMode] != gdata) {
        SysLog::logDebug(__FILE_NAME__, __LINE__, "GP1(08): displayMode: 0x%x", gdata);
        g_statusControlHistory[(size_t)display::ControlCommandId::displayMode] = gdata;
        g_statusRegister.setDisplayMode(gdata);

        if (g_videoConfig.framerateLimit == config::autodetectFramerate())
          g_timer.setFrequency(g_statusRegister.readStatus<display::SmpteStandard>(display::StatusBits::videoStandard),
                               g_statusRegister.readStatus<bool>(display::StatusBits::verticalInterlacing));
      }
      break;
    }
    // texture disabled / debug mode
    case display::ControlCommandId::allowTextureDisable: {
      g_statusControlHistory[(size_t)display::ControlCommandId::allowTextureDisable] = gdata;
      g_statusRegister.allowTextureDisable(gdata);
      break;
    }
    case display::ControlCommandId::arcadeTextureDisable: {
      if (g_statusRegister.getGpuVersion() != display::GpuVersion::psxGpu208pin) {
        g_statusControlHistory[(size_t)display::ControlCommandId::arcadeTextureDisable] = gdata;
        g_statusRegister.arcadeTextureDisable(gdata);
      }
      break;
    }
    // GPU info request
    case display::ControlCommandId::requestGpuInfo: g_statusRegister.requestGpuInfo(gdata); break;
    default: {
      if (display::StatusRegister::isGpuInfoRequestMirror(commandId))
        g_statusRegister.requestGpuInfo(gdata);
      else
        SysLog::logInfo(__FILE_NAME__, __LINE__, "GP1(%x): unknown command", (int)commandId);
      break;
    }
  }
}


// -- data transfers -- --------------------------------------------------------

// Get data transfer mode
extern "C" long CALLBACK GPUgetMode() {
  return (long)g_statusRegister.getDataTransferMode();
}
// Set data transfer mode (emulator initiates data transfer)
extern "C" void CALLBACK GPUsetMode(unsigned long transferMode) {
  if ((transferMode & (unsigned long)display::DataTransfer::vramWrite)) {
    if (g_statusRegister.readStatus<display::DmaMode>(display::StatusBits::dmaMode) == display::DmaMode::cpuToGpu)
      g_statusRegister.setDataTransferMode(display::DataTransfer::vramWrite);
  }
  else if ((transferMode & (unsigned long)display::DataTransfer::vramRead)) {
    if (g_statusRegister.readStatus<display::DmaMode>(display::StatusBits::dmaMode) == display::DmaMode::gpuToCpu)
      g_statusRegister.setDataTransferMode(display::DataTransfer::vramRead);
  }
  else
    g_statusRegister.setDataTransferMode(display::DataTransfer::primitives);
}

// ---

// Receive response data to VRAM transfer or GPU info request (GPUREAD)
extern "C" unsigned long CALLBACK GPUreadData() {
  unsigned long gdata;
  GPUreadDataMem(&gdata, 1);
  return g_statusRegister.getGpuReadBuffer();
}

// Read entire chunk of data from video memory (VRAM)
extern "C" void CALLBACK GPUreadDataMem(unsigned long* mem, int size) {

}

// ---

// Process and send data to video data register - GP0 commands
extern "C" void CALLBACK GPUwriteData(unsigned long gdata) {
  GPUwriteDataMem(&gdata, 1);
}

// Process and send chunk of data to video data register - GP0 commands
extern "C" void CALLBACK GPUwriteDataMem(unsigned long* mem, int size) {
  while (size) {
    // GP0 command (primitive/attribute)
    if (g_statusRegister.getDataTransferMode() == display::DataTransfer::primitives) {
      --size;

      auto commandId = display::StatusRegister::getGp0CommandId(*mem);
      switch (commandId) {//TODO replace with lookup table: handler fct pointer + data length + skippable
        //TODO: load/store (VRAM transfer cmd): setDataTransferMode vramRead/Write
        case 0x1Fu: g_statusRegister.setIrq1(); break;
        case 0xE1u: g_statusRegister.setTexturePageMode(*mem); break;
        case 0xE2u: g_statusRegister.setTextureWindow(*mem); break;
        case 0xE3u: g_statusRegister.setDrawAreaOrigin(*mem);  break;
        case 0xE4u: g_statusRegister.setDrawAreaEnd(*mem); break;
        case 0xE5u: g_statusRegister.setDrawOffset(*mem); break;
        case 0xE6u: g_statusRegister.setMaskBit(*mem); break;
      }
    }
    // VRAM transfer (continuous DMA)
    else {
      --size;
    }
  }
}

// Direct memory chain transfer to GPU driver (linked-list DMA)
extern "C" long CALLBACK GPUdmaChain(unsigned long* baseAddress, unsigned long index) {
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
  if (g_configDir.empty())
    GPUinit();

  //...
  return PSE_SUCCESS;
}

// Open plugin 'about' dialog box
extern "C" void CALLBACK GPUabout() {
# if defined(_WINDOWS) && defined(_VIDEO_D3D11_SUPPORT)
#   if !defined(_VIDEO_D3D11_VERSION) || _VIDEO_D3D11_VERSION != 110
#     define __ABOUT_3DAPI_NAME "Direct3D 11.1"
#   else
#     define __ABOUT_3DAPI_NAME "Direct3D 11.0"
#   endif
# else
#   define __ABOUT_3DAPI_NAME "Vulkan 1.2"
# endif
  MessageBox::show("About " LIBRARY_NAME " Renderer...",
                   LIBRARY_NAME ", by Romain Vinders\n"
#                  if defined(_WINDOWS)
#                    if _P_MIN_WINDOWS_VERSION >= 10
                       __ABOUT_3DAPI_NAME " - Windows 10 (RS2) or higher\n"
#                    else
                       __ABOUT_3DAPI_NAME " - Windows 7 or higher\n"
#                    endif
#                  elif defined(__APPLE__)
                     __ABOUT_3DAPI_NAME " - Mac OS 10.12 or higher\n"
#                  else
                     __ABOUT_3DAPI_NAME " - Linux\n"
#                  endif
                   "Version " LIBRARY_VERSION "\n\n"
                   "Special thanks:\n"
                   "- Nocash, Doomed, J. Walker: for their detailed specs\n"
                   "- Pete, Tapeq, iCatButler: for sharing public sources\n"
                   "- Amidog: for his useful test tools\n",
                   MessageBox::ActionType::ok, MessageBox::IconType::info, true);
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

// Enable/disable frame limit from emulator: 1=on / 0=off
extern "C" void CALLBACK GPUsetframelimit(unsigned long option) {
  SysLog::logDebug(__FILE_NAME__, __LINE__, "GPUsetframelimit: %u", option);

}

// Set custom fixes from emulator
//  0x0001 = GPU busy hack
extern "C" void CALLBACK GPUsetfix(unsigned long fixBits) {
  SysLog::logDebug(__FILE_NAME__, __LINE__, "GPUsetfix: 0x%x", fixBits);
  g_statusRegister.enableBusyGpuHack(fixBits & 0x0001);
}

// Set game executable ID (for config profiles associations)
extern "C" void CALLBACK GPUsetExeName(char* gameId) {
  SysLog::logDebug(__FILE_NAME__, __LINE__, "GPUsetExeName: %s", gameId);
  g_gameId = pandora::memory::LightString(gameId);
}


// -- snapshots -- -------------------------------------------------------------

// Request snapshot (on next display)
extern "C" void CALLBACK GPUmakeSnapshot() {

}

// Get screen picture
extern "C" void CALLBACK GPUgetScreenPic(unsigned char* image) {

}

// Store and display screen picture
extern "C" void CALLBACK GPUshowScreenPic(unsigned char* image) {

}


// -- miscellaneous -- ---------------------------------------------------------

// Display debug text
extern "C" void CALLBACK GPUdisplayText(char* message) {
  SysLog::logDebug(__FILE_NAME__, __LINE__, "GPUdisplayText: %s", message);

}

// Set gun cursor display and position: player=0-7, x=0-511, y=0-255
extern "C" void CALLBACK GPUcursor(int player, int x, int y) {
  g_statusRegister.setLightgunCursor((unsigned long)player, (long)x, (long)y);
}

// Trigger screen vibration
extern "C" void CALLBACK GPUvisualVibration(unsigned long smallRumble, unsigned long bigRumble) {

}


// -- other implementation files -- --------------------------------------------

#include "./psemu_zinc.hpp" // include -> single object file (less overhead + smaller lib size)
#include "./timer.hpp"
