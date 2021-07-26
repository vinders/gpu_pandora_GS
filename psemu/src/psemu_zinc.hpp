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
#include "psemu/psemu_gpu.h"
// this implementation is included in psemu_gpu.cpp -> single object file (less overhead + smaller lib size)

/// @brief ZiNc config structure
typedef struct GPUOTAG {
	unsigned long version;        // version of structure - currently 1
	long          window;         // window handle
	unsigned long screenRotation; // 0 = 0CW, 1 = 90CW, 2 = 180CW, 3 = 270CW = 90CCW
	unsigned long gpuVersion;     // 0 = a, 1 = b, 2 = c
	const char*   gameName;       // game title string
	const char*   cfgFile;        // config filepath string
} GPUConfiguration_t;


// -- driver base interface -- -------------------------------------------------

// ZiNc driver init (called once)
extern "C" long CALLBACK ZN_GPUinit() // always set VRAM size to 2MB if ZN interface is used
{
	//...
	return GPUinit();
}
// ZiNc driver shutdown (called once)
extern "C" long CALLBACK ZN_GPUshutdown() { return GPUshutdown(); }

// Open ZiNc driver (game started)
extern "C" long CALLBACK ZN_GPUopen(void* cfgData) {
	GPUConfiguration_t* config = (GPUConfiguration_t*)cfgData;
	if (config == nullptr || config->version != 1)
		return PSE_ERR_FATAL;

	//... tile fix
	//... set version

# ifdef _WINDOWS
	  return GPUopen((HWND)config->window);
# else
	  return GPUopen(&config->window, config->gameName, config->cfgFile);
# endif
}
// Close ZiNc driver (game started)
extern "C" long CALLBACK ZN_GPUclose() { return GPUclose(); }

// Display update (called on every vsync)
extern "C" void CALLBACK ZN_GPUupdateLace() { GPUupdateLace(); }


// -- status control -- --------------------------------------------------------

extern "C" unsigned long CALLBACK ZN_GPUreadStatus() { return GPUreadStatus(); }
extern "C" void CALLBACK ZN_GPUwriteStatus(unsigned long gdata) { GPUwriteStatus(gdata); }


// -- data transfers -- --------------------------------------------------------

extern "C" long CALLBACK ZN_GPUgetMode() { return GPUgetMode(); }
extern "C" void CALLBACK ZN_GPUsetMode(unsigned long transferMode) { GPUsetMode(transferMode); }

extern "C" unsigned long CALLBACK ZN_GPUreadData() { return GPUreadData(); }
extern "C" void CALLBACK ZN_GPUwriteData(unsigned long gdata) { GPUwriteDataMem(&gdata, 1); }

extern "C" long CALLBACK ZN_GPUdmaSliceOut(unsigned long* baseAddress, unsigned long offset, unsigned long size) {
	GPUreadDataMem(baseAddress + offset, size);
	return 0L;
}
extern "C" long CALLBACK ZN_GPUdmaSliceIn(unsigned long* baseAddress, unsigned long offset, unsigned long size) {
	GPUwriteDataMem(baseAddress + offset, size);
	return 0L;
}
extern "C" long CALLBACK ZN_GPUdmaChain(unsigned long* baseAddress, unsigned long offset) { return GPUdmaChain(baseAddress, offset); }


// -- save states -- -----------------------------------------------------------

extern "C" long CALLBACK ZN_GPUfreeze(unsigned long dataMode, void* state) { return GPUfreeze(dataMode, (GPUFreeze_t*)state); }


// -- runtime settings -- ------------------------------------------------------

extern "C" long CALLBACK ZN_GPUtest() { return GPUtest(); }
extern "C" void CALLBACK ZN_GPUdisplayFlags(unsigned long flags) { GPUdisplayFlags(flags); }


// -- snapshots -- -------------------------------------------------------------

extern "C" void CALLBACK ZN_GPUmakeSnapshot() { GPUmakeSnapshot(); }
extern "C" void CALLBACK ZN_GPUgetScreenPic(unsigned char* image) { GPUgetScreenPic(image); }
extern "C" void CALLBACK ZN_GPUshowScreenPic(unsigned char* image) { GPUshowScreenPic(image); }


// -- miscellaneous -- ---------------------------------------------------------

#ifndef _WINDOWS
  // Keyboard event management
  extern "C" void CALLBACK ZN_GPUkeypressed(int keycode) {
    //...
  }
#endif
