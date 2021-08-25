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

#include <cstdint>
#include "display/types.h"

namespace display {
  /// @brief GPU display control commands (GP1)
  enum class ControlCommandId : unsigned long {
    resetGpu              = 0x00u, ///< Reset GPU (status, buffer, memory, display...)
    clearCommandFifo      = 0x01u, ///< Clear command buffer (GP0 FIFO)
    ackIrq1               = 0x02u, ///< Acknowledge GPU interrupt (IRQ1)
    toggleDisplay         = 0x03u, ///< Enable/disable display
    dmaMode               = 0x04u, ///< Set direct memory access direction or data request
    displayAreaOrigin     = 0x05u, ///< Set origin of display area in VRAM
    horizontalDisplayRange= 0x06u, ///< Set horizontal display range (within which the display area is visible)
    verticalDisplayRange  = 0x07u, ///< Set vertical display range on screen (within which the display area is visible)
    displayMode           = 0x08u, ///< Set display mode (resolution, color, standard, interlacing...)
    allowTextureDisable   = 0x09u, ///< Allow disabling textures via GP0(E1).11
    arcadeTextureDisable  = 0x20u, ///< Enable/disable textures (only for arcade GPU -> ZiNc)
    requestGpuInfo        = 0x10u  ///< Request GPU info (GPU type, draw area/offset, texture window...) -> into GPUREAD register
  };
  static constexpr inline size_t controlCommandNumber() noexcept { return 0x40u; } ///< Max number of GP1 commands

  /// @brief GPU info to read (in GPU info request)
  /// @remarks If the value is none of these, the previous GPUREAD value must be kept.
  enum class GpuInfoType : unsigned long {
    textureWindow       = 0x2u, ///< Read texture window infos - GP0(E2) (20 bit, MSBs to 0)
    drawAreaStartCoords = 0x3u, ///< Read draw area top-left location - GP0(E3) (20 bit, MSBs to 0)
    drawAreaEndCoords   = 0x4u, ///< Read draw area bottom-right location - GP0(E4) (20 bit, MSBs to 0)
    drawOffset          = 0x5u, ///< Read draw offset - GP0(E5) (22 bit)
    gpuVersion          = 0x7u, ///< GPU version (1: arcade prototype (ZiNc: GPU type 2) / 2: usual 208-pin GPU)
    lightgunStatus      = 0x8u, ///< Lightgun status
    biosAddress         = 0xFu  ///< BIOS module address (GPU patch?)
  };

  // ---

  /// @brief Display control/status register + rendering attributes
  class StatusRegister final {
  public:
    StatusRegister() = default;
    StatusRegister(const StatusRegister&) = default;
    StatusRegister& operator=(const StatusRegister&) = default;

    // -- read status register -- ----------------------------------------------

    /// @brief Read GPUSTAT register (GPU status/control register)
    inline unsigned long getStatusControlRegister() noexcept {
      if (this->_gpuBusyHackCounter) { // emulate "busy" GPU (useful when emulator doesn't use threads)
        if ((--_gpuBusyHackCounter) & 1)
          this->_statusControlRegister &= ~((unsigned long)StatusBits::readyForCommands | (unsigned long)StatusBits::readyForDmaBlock);
        else
          this->_statusControlRegister |= ((unsigned long)StatusBits::readyForCommands | (unsigned long)StatusBits::readyForDmaBlock);
      }
      return this->_statusControlRegister;
    }
    /// @brief Read GPUREAD register (result of latest request)
    inline unsigned long getGpuReadBuffer() const noexcept { return this->_gpuReadBuffer; }

    /// @brief Read bit value of GPUSTAT register
    inline unsigned long readStatus(StatusBits bits) const noexcept { return (this->_statusControlRegister & (unsigned long)bits); }
    template <typename _Type>
    inline _Type readStatus(StatusBits bits) const noexcept { return (_Type)(this->_statusControlRegister & (unsigned long)bits); }


    // -- read display state & texture mode -- ---------------------------------
    
    /// @brief Get current display state: display area/range, draw area/offset/status
    inline const DisplayState& getDisplayState() const noexcept { return this->_displayState; }

    /// @brief Verify if textures are disabled
    inline bool areTexturesDisabled() const noexcept {
      return (this->_isTexturingDisabled && readStatus<bool>(StatusBits::disableTextures));
    }
    inline bool getTexpageBaseX() const noexcept { return this->_texpageBaseX; } ///< Read texture page base X
    inline bool getTexpageBaseY() const noexcept { return this->_texpageBaseY; } ///< Read texture page base Y
    inline bool isTextureFlipX() const noexcept { return this->_isTextureFlipX; } ///< Verify texture flip-X status
    inline bool isTextureFlipY() const noexcept { return this->_isTextureFlipY; } ///< Verify texture flip-Y status
    inline bool isTextureDecodingIL() const noexcept { return this->_isTextureDecodingIL; } ///< Special arcade GPU texture mode

    /// @brief Get current texture window settings (repeat cropped texture area)
    // @remarks Texture coord transform:  texcoord = (texcoord & ~(sizeMask)) | (offset & sizeMask);
    //          -> acts as if area within texture window was repeated throughout texture page
    inline const TextureWindow& getTextureWindow() const noexcept { return this->_textureWindow; }


    // -- hardware info & transfer mode -- -------------------------------------

    /// @brief GPU info request (GP1(0x10)) -> store result in GPUREAD register
    void requestGpuInfo(unsigned long params) noexcept;
    /// @brief Manually set GPUSTAT register (when loading save-state)
    inline void setStatusControlRegister(unsigned long value) noexcept { this->_statusControlRegister = value; }
    /// @brief Manually store value in GPUREAD register (during DMA or when loading save-state)
    inline void setGpuReadBuffer(unsigned long buffer) noexcept { this->_gpuReadBuffer = buffer; }

    /// @brief Change hardware version + VRAM height (only required for non-standard/arcade GPU)
    inline void setGpuType(GpuVersion hwVersion, unsigned long vramHeight) noexcept {
      this->_gpuType = hwVersion;
      this->_vramHeight = vramHeight;
    }
    /// @brief Get hardware version
    inline GpuVersion getGpuVersion() const noexcept { return this->_gpuType; }
    /// @brief Get VRAM height
    inline unsigned long getGpuVramHeight() const noexcept { return this->_vramHeight; }

    /// @brief Set direct-memory-access mode (GP1(0x04)) -> set data transfer mode in register status
    void setDmaMode(unsigned long params) noexcept;
    /// @brief Set current data transfer mode (after receiving/completing VRAM read/write commands)
    inline void setDataTransferMode(DataTransfer mode) noexcept { this->_dataTransferMode = mode; }
    /// @brief Get actual current data transfer mode
    /// @remarks Equal to DMA mode of register status in most cases (but may differ for compatibility reasons)
    inline DataTransfer getDataTransferMode() const noexcept { return this->_dataTransferMode; }

    /// @brief Remove all lightgun cursors
    inline void clearLightgunCursors() noexcept { this->_activeLightguns = 0; }
    /// @brief Enable lightgun cursor + set position
    void setLightgunCursor(unsigned long player, long x, long y) noexcept;
    /// @brief Read bit-map containing active lightgun flags (bit index == player index)
    inline unsigned long getActiveLightgunsMap() const noexcept { return this->_activeLightguns; }
    /// @brief Get array of lightgun cursor positions
    /// @warning - Verify if a lightgun is active ('getActiveLightgunsMap()') before reading its position;
    ///          - Max array index: 'maxLightgunCursors()'.
    inline const Point* lightgunCursorPositions() const noexcept { return this->_lightgunCursors; }


    // -- command helpers -- ---------------------------------------------------

    /// @brief Extract command ID from first block of GP0 command
    static constexpr inline unsigned long getGp0CommandId(unsigned long gdata) noexcept { return ((gdata >> 24) & 0xFFu); }
    /// @brief Extract command ID from first block of GP1 command
    static constexpr inline ControlCommandId getGp1CommandId(unsigned long gdata) noexcept {
      return (ControlCommandId)((gdata >> 24) & 0x3Fu);
    }
    /// @brief Verify if a command ID is a duplicate of the GPU info request command ID
    static constexpr inline bool isGpuInfoRequestMirror(ControlCommandId commandId) noexcept {
      return ((unsigned long)commandId & (unsigned long)display::ControlCommandId::requestGpuInfo);
    }


    // -- command synchronization -- -------------------------------------------

    /// @brief Clear pending command flags (GP1(0x01))
    inline void clearPendingCommands() noexcept { this->_gpuBusyHackCounter = 0; }

    /// @brief Set status flag 'readyForCommands': when the GPU is ready for new commands
    inline void setGpuIdle() noexcept { this->_statusControlRegister |= (unsigned long)StatusBits::readyForCommands; }
    /// @brief Clear status flag 'readyForCommands': - when the GPU is busy with a command execution
    ///                                              - when the GPU is waiting for DMA data blocks
    inline void setGpuBusy() noexcept { this->_statusControlRegister &= ~(unsigned long)StatusBits::readyForCommands; }

    /// @brief Clear status flag 'readyForDmaBlock': - when receiving polygon/line primitive (immediately, before reading vertex params)
    ///                                              - after receiving any other GP0 command+params (all params received)
    inline void setGp0CommandReceived() noexcept {
      this->_statusControlRegister &= (this->_statusControlRegister & (unsigned long)DmaMode::cpuToGpu)
                                      ? ~((unsigned long)StatusBits::readyForDmaBlock | (unsigned long)StatusBits::dmaRequestState)
                                      : ~(unsigned long)StatusBits::readyForDmaBlock;
    }
    /// @brief Restore status flag 'readyForDmaBlock': after completing command processing
    inline void setGp0CommandFinished() noexcept {
      this->_statusControlRegister |= (this->_statusControlRegister & (unsigned long)DmaMode::cpuToGpu)
                                      ? ((unsigned long)StatusBits::readyForDmaBlock | (unsigned long)StatusBits::dmaRequestState)
                                      : (unsigned long)StatusBits::readyForDmaBlock;
      if (this->_useGpuBusyHack)
        this->_gpuBusyHackCounter = 4;
    }

    /// @brief Set status flag 'readyForDmaRead': after receiving VRAM read command+params (GP0(0xC0))
    inline void setVramReadPending() noexcept {
      this->_statusControlRegister |= (this->_statusControlRegister & (unsigned long)DmaMode::gpuToCpu)
                                      ? ((unsigned long)StatusBits::readyForDmaRead | (unsigned long)StatusBits::dmaRequestState)
                                      : (unsigned long)StatusBits::readyForDmaRead;
    }
    /// @brief Clear status flag 'readyForDmaRead': after completing data transfer
    inline void setVramReadFinished() noexcept {
      this->_statusControlRegister &= (this->_statusControlRegister & (unsigned long)DmaMode::gpuToCpu)
                                      ? ~((unsigned long)StatusBits::readyForDmaRead | (unsigned long)StatusBits::dmaRequestState)
                                      : ~(unsigned long)StatusBits::readyForDmaRead;
    }

    /// @brief Raise GPU interruption request (IRQ1)
    inline void setIrq1() noexcept { this->_statusControlRegister |= (unsigned long)StatusBits::interruptReq1; }
    /// @brief Acknowledge GPU interruption request (GP1(0x02))
    inline void ackIrq1() noexcept { this->_statusControlRegister &= ~((unsigned long)StatusBits::interruptReq1); }


    // -- display mode control -- ----------------------------------------------

    /// @brief Reset GPU status and display mode (GP1(0x00))
    /// @note  _isTexturingDisabled (GP1(0x09)) is not affected by reset
    void resetGpu() noexcept;
    static void resetControlCommandHistory(unsigned long* history) noexcept; ///< Update command history values after a reset

    /// @brief Enable/disable display output (GP1(0x03))
    /// @note  when disabled, a black frame is drawn.
    inline void toggleDisplay(unsigned long params) noexcept {
      if (params & 0x1u)
        this->_statusControlRegister |= (unsigned long)StatusBits::disableDisplay;
      else
        this->_statusControlRegister &= ~((unsigned long)StatusBits::disableDisplay);
    }
    /// @brief Change general display settings: SMPTE standard, display resolution, color depth... (GP1(0x08))
    /// @remarks - Color depth: 24-bit output must use pre-rendered assets (textures, MDEC): GPU always draws 15-bit colors in draw area.
    ///          - Vertical interlacing: only impacts output if 'displayAreaHeight' bit is set too.
    ///          - Reverse flag: display scrolls down by 2 lines + colored regions are distorted (black/white regions still OK)
    ///                          -> PAL/NTSC color clock conversion?
    void setDisplayMode(unsigned long params) noexcept;

    /// @brief Enable/disable textures - standard PS1 GPU + some arcade GPUs (GP1(0x09))
    inline void allowTextureDisable(unsigned long params) noexcept {
      this->_isTexturingDisabled = (params & 0x1u);
    }
    /// @brief Enable/disable textures - special arcade GPU (GP1(0x20))
    inline void arcadeTextureDisable(unsigned long params) noexcept {
      params &= 0xFFFFu;
      if (params == 0x501u)
        this->_isTexturingDisabled = false;
      else if (params == 0x504u)
        this->_isTexturingDisabled = true;
    }
    /// @brief Enable special GPU hack (to alternate busy/idle state a few times when reading status register after a command)
    /// @remarks Useful when emulator doesn't use threads -> all commands are synchronous (and GPU never seems "busy")
    inline void enableBusyGpuHack(bool isEnabled) noexcept { this->_useGpuBusyHack = isEnabled; }


    // -- display area/range & draw area -- ------------------------------------

    /// @brief Set upper/left display source address in VRAM (GP1(0x05))
    void setDisplayAreaOrigin(unsigned long params) noexcept;
    /// @brief Set horizontal range within which the display area is visible (GP1(0x06))
    /// @warning Many games set the display range BEFORE display size and video standard -> only raw values are stored here.
    void setHorizontalDisplayRange(unsigned long params) noexcept;
    /// @brief Set vertical range within which the display area is visible (GP1(0x06))
    /// @warning Many games set the display range BEFORE display size and video standard -> only raw values are stored here.
    void setVerticalDisplayRange(unsigned long params) noexcept;

    /// @brief Set drawing area top/left corner position (GP0(0xE3)) -> when rendering shapes, pixels outside of it are clipped
    /// @remarks VRAM transfer and fill commands are NOT clipped by drawing area.
    void setDrawAreaOrigin(unsigned long params) noexcept;
    /// @brief Set drawing area bottom/right corner position (GP0(0xE4)) -> when rendering shapes, pixels outside of it are clipped
    /// @remarks VRAM transfer and fill commands are NOT clipped by drawing area.
    void setDrawAreaEnd(unsigned long params) noexcept;
    /// @brief Set drawing offset (GP0(0xE5)) -> rendered shapes are relative to it
    /// @remarks VRAM transfer and fill commands are NOT relative to drawing offset (they use absolute coords).
    void setDrawOffset(unsigned long params) noexcept;


    // -- texture management -- ------------------------------------------------

    /// @brief Set texture page: draw mode settings (GP0(0xE1))
    /// @remarks Texture page XY, semi-transparency, colors: only used for lines, rectangles, untextured-polygons
    ///          (textured polygon commands have their own texpage attribute)
    void setTexturePageMode(unsigned long params) noexcept;
    /// @brief Set texture window settings: repeat cropped texture area (GP0(0xE2))
    // @remarks Texture coord transform:  texcoord = (texcoord & ~(sizeMask)) | (offset & sizeMask);
    //          -> acts as if area within texture window was repeated throughout texture page
    void setTextureWindow(unsigned long params) noexcept;

    /// @brief Set mask settings for drawing: source value for mask bit + write protection (GP0(0xE6))
    /// @remarks Mask settings affect all rendering commands + CPU->VRAM and VRAM->VRAM copies (note: the FillVRAM command is not affected)
    inline void setMaskBit(unsigned long params) noexcept {
      this->_statusControlRegister &= ~((unsigned long)StatusBits::enableMask | (unsigned long)StatusBits::forceSetMaskBit);
      this->_statusControlRegister |= ((params << bitOffset_forceSetMaskBit())
                                      & ((unsigned long)StatusBits::enableMask | (unsigned long)StatusBits::forceSetMaskBit));
    }

  private:
    // status/control registers
    unsigned long _statusControlRegister = statusControlDefaults();
    unsigned long _gpuReadBuffer = 0x400uL;
    DataTransfer _dataTransferMode = DataTransfer::primitives;

    // display state
    DisplayState _displayState;
    unsigned long _texpageBaseX = 0;
    unsigned long _texpageBaseY = 0;
    TextureWindow _textureWindow;

    bool _isTextureFlipX = false;
    bool _isTextureFlipY = false;
    bool _isTextureDecodingIL = false; // only for special arcade GPU
    bool _isTexturingDisabled = false;
    bool _useGpuBusyHack = false;
    uint32_t _gpuBusyHackCounter = 0;

    // lightgun
    unsigned long _activeLightguns = 0;
    Point _lightgunCursors[maxLightgunCursors()]{{ 0,0 }};

    // hardware info
    GpuVersion _gpuType = GpuVersion::psxGpu208pin;
    unsigned long _vramHeight = psxVramHeight();
  };
}
