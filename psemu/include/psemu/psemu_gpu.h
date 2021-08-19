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
#pragma once

#ifdef _WINDOWS
# include <system/api/windows_api.h>
# ifndef CALLBACK
#   define CALLBACK __stdcall
# endif
#else
# define CALLBACK
#endif

# define  _PPDK_HEADER_VERSION       1uL // PSEmu 1.x

  // plugin type returned by PSEgetLibType (types can be merged)
# define  PSE_LT_CDR  1L
# define  PSE_LT_GPU  2L
# define  PSE_LT_SPU  4L
# define  PSE_LT_PAD  8L

  // plugin return codes
# define  PSE_SUCCESS                 0L // every interface function should return this value if successful
# define  PSE_ERR_FATAL              -1L // undefined error but fatal one, that kills all functionality
# define  PSE_INIT_SUCCESS            0L // initialization went OK
# define  PSE_INIT_ERR_NOTCONFIGURED -2L // this driver is not configured
# define  PSE_INIT_ERR_NOHARDWARE    -3L // this driver can not operate properly on this hardware or hardware is not detected
# define  SAVESTATE_SUCCESS           1L
# define  SAVESTATE_ERR               0L

  // GPU test return values
# define  PSE_GPU_SUCCESS             0L // success, everything configured, and went OK
# define  PSE_GPU_ERR_NOTCONFIGURED   1L // this driver is not configured

# define  PSE_GPU_FLAGS_WINDOWED      1L // this driver requests window mode (gpuQueryS.flags)
# define  PSE_GPU_STATUS_WINDOWWRONG  1L // this driver cannot operate in this window mode (gpuQueryS.status)

  // CDR test return values
# define  PSE_CDR_SUCCESS             0L  // success, everything configured, and went OK
# define  PSE_CDR_ERR                -40L // driver error
# define  PSE_CDR_ERR_NOTCONFIGURED   PSE_CDR_ERR - 0 // this driver is not configured
# define  PSE_CDR_ERR_NOREAD          PSE_CDR_ERR - 1 // this driver is unable to read data from medium
# define  PSE_CDR_WARN                40L // driver warnings
  // if this driver emulates lame mode (ie. can read only 2048 tracks and sector header is emulated)
  // this might happen to CDROMS that do not support RAW mode reading
# define  PSE_CDR_WARN_LAMECD         PSE_CDR_WARN + 0


extern "C" {
  // -- plugin library info -- -------------------------------------------------

  /// @brief Get library name
  char* CALLBACK PSEgetLibName();
  /// @brief Get PSemu library type (GPU)
  unsigned long CALLBACK PSEgetLibType();
  /// @brief Get library version ID
  unsigned long CALLBACK PSEgetLibVersion();


  // -- driver base interface -- -----------------------------------------------

  /// @brief Driver init (called once)
  /// @returns Success indicator
  long CALLBACK GPUinit();
  /// @brief Driver shutdown (called once)
  /// @returns Success indicator
  long CALLBACK GPUshutdown();

# ifdef _WINDOWS
    /// @brief Open driver (game started)
    long CALLBACK GPUopen(HWND window);
# else
    /// @brief Open driver (game started)
    long CALLBACK GPUopen(unsigned long* displayId, char* caption, char* configFile);
# endif
  /// @brief Close driver (game stopped)
  long CALLBACK GPUclose();

  /// @brief Display update (called on every vsync)
  void CALLBACK GPUupdateLace();


  // -- status control -- ------------------------------------------------------

  /// @brief Read data from GPU status register
  /// @returns GPU status register data
  unsigned long CALLBACK GPUreadStatus();
  /// @brief Process data sent to GPU status register - GP1 commands
  /// @param gdata  Status register command
  void CALLBACK GPUwriteStatus(unsigned long gdata);


  // -- data transfers -- ------------------------------------------------------

  /// @brief Get data transfer mode
  /// @returns Image transfer mode
  long CALLBACK GPUgetMode();
  /// @brief Set data transfer mode (deprecated)
  /// @param transferMode  Image transfer mode
  void CALLBACK GPUsetMode(unsigned long transferMode);

  /// @brief Receive response data to VRAM transfer or GPU info request (GPUREAD)
  /// @returns Raw GPU data
  unsigned long CALLBACK GPUreadData();
  /// @brief Read entire chunk of data from video memory (VRAM)
  /// @param mem   Pointer to chunk of data (destination)
  /// @param size  Memory chunk size
  void CALLBACK GPUreadDataMem(unsigned long* mem, int size);

  /// @brief Process and send data to video data register - GP0 commands
  void CALLBACK GPUwriteData(unsigned long gdata);
  /// @brief Process and send chunk of data to video data register - GP0 commands
  /// @param mem   Pointer to chunk of data (source)
  /// @param size  Memory chunk size
  void CALLBACK GPUwriteDataMem(unsigned long* mem, int size);
  /// @brief Direct memory chain transfer to GPU driver
  /// @param baseAddress  Pointer to memory chain
  /// @param offset       Memory offset
  long CALLBACK GPUdmaChain(unsigned long* baseAddress, unsigned long offset);
  
  
  // -- save states -- ---------------------------------------------------------
  
  /// @brief Save-state data structure
  typedef struct GPUFREEZETAG {
    unsigned long freezeVersion;    ///< System version: always 1 (set by emulator)
    unsigned long status;           ///< Current GPU status
    unsigned long controlReg[256];  ///< Latest control register values
    unsigned char psxVram[1024 * 1024 * 2]; ///< Current VRAM image
  } GPUFreeze_t;

  /// @brief Save/load current state
  /// @param dataMode  Transaction type (0 = setter / 1 = getter / 2 = slot selection)
  /// @param mem       Save-state structure pointer (to read or write)
  long CALLBACK GPUfreeze(unsigned long dataMode, GPUFreeze_t* state);


  // -- plugin dialog interface -- ---------------------------------------------

  /// @brief Open plugin config dialog box
  long CALLBACK GPUconfigure();
  /// @brief Open plugin 'about' dialog box
  void CALLBACK GPUabout();

  /// @brief Check if plugin works
  /// @returns PSE_GPU_SUCCESS (ok), PSE_GPU_ERR_NOTCONFIGURED (warning) or PSE_ERR_FATAL (error).
  long CALLBACK GPUtest();


  // -- runtime settings -- ----------------------------------------------------

  /// @brief Set special display flags
  void CALLBACK GPUdisplayFlags(unsigned long flags);
  /// @brief Enable/disable frame limit from emulator
  /// @param option  Status (1 = limit / 0 = none)
  void CALLBACK GPUsetframelimit(unsigned long option);
  /// @brief Set custom fixes from emulator
  void CALLBACK GPUsetfix(unsigned long fixBits);
  /// @brief Set game executable ID (for config profiles associations)
  void CALLBACK GPUsetExeName(char* gameId);


  // -- snapshots -- -----------------------------------------------------------

  /// @brief Request snapshot (on next display)
  void CALLBACK GPUmakeSnapshot();
  /// @brief Get screen picture
  /// @param mem  Allocated screen picture container 128x96 px (24b/px: 8-8-8 bit BGR, no header)
  void CALLBACK GPUgetScreenPic(unsigned char* image);
  /// @brief Store and display screen picture
  /// @param mem  Screen picture data 128x96 px (24b/px: 8-8-8 bit BGR, no header)
  void CALLBACK GPUshowScreenPic(unsigned char* image);


  // -- miscellaneous -- -------------------------------------------------------

  /// @brief Display debug message
  void CALLBACK GPUdisplayText(char* message);
  /// @brief Set gun cursor display and position
  /// @param player  Player with current cursor (0-7)
  /// @param x       Cursor horizontal position (0-511)
  /// @param y       Cursor vertical position (0-255)
  void CALLBACK GPUcursor(int player, int x, int y);
  /// @brief Trigger screen vibration
  /// @param smallRumble  Small rumble value
  /// @param bigRumble    Big rumble value (if != 0, 'small' will be ignored)
  void CALLBACK GPUvisualVibration(unsigned long smallRumble, unsigned long bigRumble);
}
