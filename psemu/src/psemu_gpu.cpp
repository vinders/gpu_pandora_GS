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
#include "display/status_lock.h"
#include "display/dma_chain_iterator.h"
#include "display/window_builder.h"
#include "display/renderer.h"
#include "psemu/syslog.h"
#include "psemu/timer.h"
#include "psemu/config_io.h"
#include "psemu/psemu_gpu.h"

using pandora::video::MessageBox;
using namespace psemu;

#define __DECLARE_GLOBALS 1
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
uint32_t g_delayToStart = 0;


// -- entry point -- -----------------------------------------------------------

#ifdef _WINDOWS
# include <system/api/windows_app.h>
# include "_generated/resources.h"
# define __MENU_CURSOR_ID MAKEINTRESOURCEW(IDC_MENU_CURSOR)

  // Main library entry point (Windows)
  BOOL APIENTRY DllMain(HANDLE module, DWORD reason, LPVOID) {
    pandora::system::WindowsApp::instance().init((reason != DLL_PROCESS_DETACH) ? (HINSTANCE)module : nullptr); // attach / detach
    return TRUE;
  }
#else
# define __MENU_CURSOR_ID nullptr
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
  return (_PPDK_HEADER_VERSION << 16) | (libVersionMajor() << 8) | libVersionMinor();
}

#ifndef _WINDOWS
  extern "C" char* GPUgetLibInfos() {
    static char libName[] = LIBRARY_NAME "\nBy Romain Vinders";
    return libName;
  }
#endif


// -- driver base interface -- -------------------------------------------------

// Driver init (called once)
extern "C" long CALLBACK GPUinit() {
  try {
    // identify emulator
    config::readEmulatorInfo(g_emulator);
    // no need to wait during black frames before startup -> accelerate boot time
    g_delayToStart = (g_emulator.type == config::EmulatorType::epsxe) ? 560 : 220; // TODO: detect first rendered primitives instead

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

    // load global config (on failure, keep default config)
    loadGlobalConfig(g_configDir, g_videoConfig, g_windowConfigurator.windowConfig(), g_inputConfig);

    g_statusRegister = display::StatusRegister{}; // reset status
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
  //TODO: save game/profile association

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
    config::RendererProfile rendererConfig;

    // GPU test -> use accurate settings
    if (g_gameId == PSX_GPU_TEST_ID) { 
      config::loadPreset(config::PresetId::psxAccurate, rendererConfig);
      g_windowConfigurator.windowConfig().windowMode = config::WindowMode::window;
    }
    // normal game -> load config profile associated with game ID (if available)
    else {
      std::vector<config::ProfileMenuTile> profiles = readListOfProfiles(g_configDir);
      loadGameConfigProfile(g_configDir, g_gameId, profiles, rendererConfig);
      //TODO: don't reload if GPUopen is called again but not GPUinit
      // -> if user changed active profile, don't "reset" to associated profile everytime the window is open
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
    g_timer.setSpeedMode(g_videoConfig.enableFramerateLimit ? SpeedMode::normal : SpeedMode::none);
    g_timer.setFrameSkipping(g_videoConfig.enableFrameSkip);
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
  if (g_delayToStart) {
    --g_delayToStart;
    if (g_delayToStart == 0) {
      g_timer.reset();
    }
  }
  else {
    g_timer.waitPeriod();
    //TODO: configurable turbo speed: 2x   = skip 1 frame / 2 (+ only waitPeriod() when not skipped);  4x = skip 3 frames / 4;  ... up to 8x
    //TODO: configurable slow motion: 1/2x = waitPeriod() called twice;  1/4x = called 4x;  ... up to 1/8x
  }
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
      //TODO: reset current VRAM transfer (remaining data...)
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
      g_statusControlHistory[(size_t)display::ControlCommandId::displayAreaOrigin] = gdata;
      g_statusRegister.setDisplayAreaOrigin(gdata);
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
  return ((long)g_statusRegister.getDataWriteMode() | ((long)g_statusRegister.getDataReadMode() << 1));
}
// Set data transfer mode (emulator initiates data transfer)
extern "C" void CALLBACK GPUsetMode(unsigned long transferMode) {} // deprecated

// ---

// Receive response data to VRAM transfer or GPU info request (GPUREAD)
extern "C" unsigned long CALLBACK GPUreadData() {
  unsigned long gdata;
  GPUreadDataMem(&gdata, 1);
  return g_statusRegister.getGpuReadBuffer();
}

// Read entire chunk of data from video memory (VRAM)
extern "C" void CALLBACK GPUreadDataMem(unsigned long* mem, int size) {
  if (g_statusRegister.getDataReadMode() == display::DataTransfer::vramTransfer) {
    display::GpuBusyStatusLock gpuBusyLock(g_statusRegister);

    //...
    //g_statusRegister.setGpuReadBuffer(...);

    g_statusRegister.setDataReadMode(display::DataTransfer::command);
    g_statusRegister.setVramReadFinished();
  }
}

// ---

// Process and send data to video data register - GP0 commands
extern "C" void CALLBACK GPUwriteData(unsigned long gdata) {
  GPUwriteDataMem(&gdata, 1);
}

// Process and send chunk of data to video data register - GP0 commands
extern "C" void CALLBACK GPUwriteDataMem(unsigned long* mem, int size) {
  display::GpuBusyStatusLock gpuBusyLock(g_statusRegister);
  display::Gp0CommandStatusLock gp0CommandLock(g_statusRegister);

  while (size > 0) {
    // VRAM transfer (continuous DMA)
    if (g_statusRegister.getDataWriteMode() == display::DataTransfer::vramTransfer) {
      //while cols/rows remaining
        //if size == 0 before end of cols/rows, return;
        //copy data as a texture ???
        //watch out for alignement (if odd width -> padded or mis-aligned ???)
        --size;

      // stop vram transfer
      g_statusRegister.setDataWriteMode(display::DataTransfer::command);
    }
    // GP0 command (primitive/attribute)
    else {
      size_t cmdSize = 1;
      auto commandId = display::StatusRegister::getGp0CommandId(*mem);
      switch (commandId) {//TODO replace with lookup table: handler fct pointer + data length + skippable
        case 0x1Fu: g_statusRegister.setIrq1(); break;
        case 0xE1u: g_statusRegister.setTexturePageMode(*mem); break;
        case 0xE2u: g_statusRegister.setTextureWindow(*mem); break;
        case 0xE3u: g_statusRegister.setDrawAreaOrigin(*mem);  break;
        case 0xE4u: g_statusRegister.setDrawAreaEnd(*mem); break;
        case 0xE5u: g_statusRegister.setDrawOffset(*mem); break;
        case 0xE6u: g_statusRegister.setMaskBit(*mem); break;
        default: {
          if (commandId >= 0xA0 && commandId < 0xC0) {
            //unsigned long x = (mem[1] & 0x3FFu);
            //unsigned long y = (g_statusRegister.getGpuVramHeight() == display::psxVramHeight()) ? ((mem[1] >> 16) & 0x1FFu) : ((mem[1] >> 16) & 0x3FFu);
            //imgWidth = ((mem[2] - 1u) & 0x3FFu) + 1u;
            //imgHeight = (((mem[2] >> 16) - 1u) & 0x1FFu) + 1u;

            g_statusRegister.setDataWriteMode(display::DataTransfer::vramTransfer);
            cmdSize = 2;
          }
          else if (commandId >= 0xC0 && commandId < 0xE0) {
            g_statusRegister.setDataReadMode(display::DataTransfer::vramTransfer);
            g_statusRegister.setVramReadPending();
            cmdSize = 2;
          }
          break;
        }
      }
      size -= (int)cmdSize;
      mem += cmdSize;;
    }
  }
}

// Direct memory chain transfer to GPU driver (linked-list DMA)
extern "C" long CALLBACK GPUdmaChain(unsigned long* baseAddress, unsigned long index) {
  display::GpuBusyStatusLock gpuBusyLock(g_statusRegister);
  unsigned long* dmaBlock = nullptr;
  int blockSize = 0;

  if (g_statusRegister.getGpuVramHeight() == display::psxVramHeight()) {
    display::DmaChainIterator<display::psxRamSize()> it(baseAddress, index);
    while (it.readNext(&dmaBlock, blockSize)) {
      if (blockSize > 0)
        GPUwriteDataMem(dmaBlock, blockSize);
    }
  }
  else {
    display::DmaChainIterator<display::znArcadeRamSize()> it(baseAddress, index);
    while (it.readNext(&dmaBlock, blockSize)) {
      if (blockSize > 0)
        GPUwriteDataMem(dmaBlock, blockSize);
    }
  }
  return PSE_SUCCESS;
}


// -- save states -- -----------------------------------------------------------

long g_saveStateSlot = 0;

// Save/load current state
extern "C" long CALLBACK GPUfreeze(unsigned long dataMode, GPUFreeze_t* state) {
  SysLog::logDebug(__FILE_NAME__, __LINE__, "GPUfreeze: %u", dataMode);
  if (state == nullptr)
    return SAVESTATE_ERR;

  // change save-state slot
  if (dataMode == PSE_SELECT_STATE) {
    long slotIndex = *((long*)state);
    if (slotIndex < 0 || slotIndex > 8)
      return SAVESTATE_ERR;

    g_saveStateSlot = slotIndex;//TODO: display thumbnail
  }
  else {
    if (state->freezeVersion != 1)
      return SAVESTATE_ERR;

    // save status + vram
    if (dataMode == PSE_SAVE_STATE) {
      state->status = g_statusRegister.readStatus((display::StatusBits)0xFFFFFFFFu) // not getStatusControlRegister -> avoid triggering 'GPU busy'
                    | ((unsigned long)display::StatusBits::readyForCommands | (unsigned long)display::StatusBits::readyForDmaBlock);

      memcpy(state->control, g_statusControlHistory, display::controlCommandNumber()*sizeof(unsigned long));
      state->control[0x11] = g_statusRegister.getGpuReadBuffer();

      //memcpy(state->psxVram, g_vram, 1024*g_statusRegister.getGpuVramHeight()*2);//TODO
    }
    // load status + vram
    else if (dataMode == PSE_LOAD_STATE) {
      //memcpy(g_vram, state->psxVram, 1024*g_statusRegister.getGpuVramHeight()*2);//TODO
      //reset texture area//TODO

      GPUwriteStatus(state->control[(size_t)display::ControlCommandId::resetGpu]);
      GPUwriteStatus(state->control[(size_t)display::ControlCommandId::clearCommandFifo]);
      GPUwriteStatus(state->control[(size_t)display::ControlCommandId::displayAreaOrigin]);
      GPUwriteStatus(state->control[(size_t)display::ControlCommandId::horizontalDisplayRange]);
      GPUwriteStatus(state->control[(size_t)display::ControlCommandId::verticalDisplayRange]);
      GPUwriteStatus(state->control[(size_t)display::ControlCommandId::displayMode]);
      GPUwriteStatus(state->control[(size_t)display::ControlCommandId::toggleDisplay]);
      if (state->control[(size_t)display::ControlCommandId::allowTextureDisable]) // avoid reset if empty
        GPUwriteStatus(state->control[(size_t)display::ControlCommandId::allowTextureDisable]);
      else if (state->control[(size_t)display::ControlCommandId::arcadeTextureDisable]) // avoid reset if empty
        GPUwriteStatus(state->control[(size_t)display::ControlCommandId::arcadeTextureDisable]);
      GPUwriteStatus(state->control[(size_t)display::ControlCommandId::dmaMode]);

      g_statusRegister.setStatusControlRegister(state->status);
      g_statusRegister.setGpuReadBuffer(state->control[0x11]);
      g_timer.reset();
    }
    else
      return SAVESTATE_ERR;
  }
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
//  0x1 = analog (digital if bit not set)
//  0x2 = mouse
//  0x0F00 = 0:digital / 1:analog / 2:mouse / 3:lightgun
//  (flags&0XF000)>>12 = number
extern "C" void CALLBACK GPUdisplayFlags(unsigned long flags) {
  SysLog::logDebug(__FILE_NAME__, __LINE__, "GPUdisplayFlags: 0x%x", flags);

  g_inputConfig.hintMenuOnMouseMove = (flags & 0x202); // don't display menu on mouse move, if mouse input (or lightgun)
  if ((flags & 0x0F00) == 0x300 && g_statusRegister.getActiveLightgunsMap() == 0)
    g_statusRegister.setLightgunCursor(0,0,0); // report lightgun in status register (if not yet registered)

  //TODO: display thumbnails
}

// Enable/disable frame limit from emulator: 1=on / 0=off
extern "C" void CALLBACK GPUsetframelimit(unsigned long option) {
  SysLog::logDebug(__FILE_NAME__, __LINE__, "GPUsetframelimit: %u", option);
  bool enableLimit = (option & 0x1);

  if (enableLimit != g_videoConfig.enableFramerateLimit) {
    g_videoConfig.enableFramerateLimit = enableLimit;
    if (enableLimit) {
      g_timer.setSpeedMode(SpeedMode::normal);
      g_timer.reset();
    }
    else
      g_timer.setSpeedMode(SpeedMode::none);
  }
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
#include "./config_io.hpp"
