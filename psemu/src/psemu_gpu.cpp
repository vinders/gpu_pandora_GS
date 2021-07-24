/*******************************************************************************
Pandora GS - PSEmu-compatible GPU driver
Copyright (C) 2021  Romain Vinders

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation, version 2
of the License.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details (LICENSE file).
--------------------------------------------------------------------------------
PSEmu Plugin Developer Kit Header definition - (C)1998 Vision Thing
This file can be used only to develop PSEmu Plugins. Other usage is highly prohibited.
*******************************************************************************/
#include "_generated/library_info.h"
#include "psemu/psemu_gpu.h"


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

// Driver init (called once)
extern "C" long CALLBACK GPUinit() {
  return PSE_INIT_SUCCESS;
}

// Driver shutdown (called once)
extern "C" long CALLBACK GPUshutdown() {
  return PSE_SUCCESS;
}

// ---

#ifdef _WINDOWS
  // Open driver (game started) - win32
  extern "C" long CALLBACK GPUopen(HWND window) {
    return PSE_GPU_SUCCESS;
  }
#else
  // Open driver (game started) - linux / unix
  extern "C" long CALLBACK GPUopen(unsigned long* displayId, char* caption, char* configFile) {
    return PSE_GPU_SUCCESS;
}
#endif

// Close driver (game stopped)
extern "C" long CALLBACK GPUclose() {
  return PSE_SUCCESS;
}

// ---

// Display update (called on every vsync)
extern "C" void CALLBACK GPUupdateLace() {

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
  
}

// Enable/disable frame limit from emulator
extern "C" void CALLBACK GPUsetframelimit(unsigned long option) {
  
}

// Set custom fixes from emulator
extern "C" void CALLBACK GPUsetfix(unsigned long fixBits) {
}

// Set game executable ID (for config profiles associations)
extern "C" void CALLBACK GPUsetExeName(char* gameId) {

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

}

// Set gun cursor display and position
extern "C" void CALLBACK GPUcursor(int player, int x, int y) {

}

// Trigger screen vibration
extern "C" void CALLBACK GPUvisualVibration(unsigned long smallRumble, unsigned long bigRumble) {

}
