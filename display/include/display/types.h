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

namespace display {
  // -- status/control register bits -- ----------------------------------------
  
  /// @brief SMPTE video standards
  /// @remarks Values use offset for StatusBits components
  enum class SmpteStandard : unsigned long {
    ntsc  = 0x00000000u, ///< NTSC: 240p/29.97Hz - 480i/59.94Hz
    pal   = 0x00100000u  ///< PAL:  256p/25.00Hz - 512i/50.00Hz
  };
  /// @brief Direct memory access modes
  /// @remarks Values use offset for StatusBits components
  enum class DmaMode : unsigned long {
    off        = 0x00000000u, ///< 00: off
    fifoStatus = 0x20000000u, ///< 01: check FIFO status
    cpuToGpu   = 0x40000000u, ///< 10: DMA CPU -> GPU (write)
    gpuToCpu   = 0x60000000u  ///< 11: DMA GPU -> CPU (read)
  };
  /// @brief Color data storage for textures
  /// @remarks Values use offset for StatusBits components
  enum class TextureColorMode : uint32_t {
    lookupTable4bit  = 0x00000000u, ///< 00: 4-bit lookup table
    lookupTable8bit  = 0x00000080u, ///< 01: 8-bit lookup table
    directColor15bit = 0x00000100u, ///< 10: direct 15-bit
    reserved         = 0x00000180u  ///< 11: reserved (same as 'directColor15bit')
  };
  /// @brief Semi-transparency modes
  /// @remarks Values use offset for StatusBits components
  /// @note    back = pixel read in framebuffer; front = semi-transparent pixel
  enum class BlendingMode : uint32_t {                        
    mean       = 0x00000000u, ///< 00: 0.5*back + 0.5*front
    add        = 0x00000020u, ///< 01: 1.0*back + 1.0*front
    subtract   = 0x00000040u, ///< 10: 1.0*back - 1.0*front
    addQuarter = 0x00000060u  ///< 11: 1.0*back + 0.25*front
  };

  // ---

  /// @brief GPU control/status register bits
  enum class StatusBits : unsigned long {
    // data exchange status
    texturePageBaseX    = 0x0000000Fu, ///< Texture page X base (multiplied by 64 bytes).
    texturePageBaseY    = 0x00000010u, ///< Texture page Y base (multiplied by 256 lines).
    semiTransparency    = 0x00000060u, ///< Semi-transparency mode (blending type).
    texturePageColors   = 0x00000180u, ///< Texture color mode (4-bit/8-bit lookup table / 15-bit colors).
    dithering           = 0x00000200u, ///< Color 24-bit->15-bit dithering (15-bit output: on/strip-LSB -- 24-bit output: on/off).
    drawToDisplay       = 0x00000400u, ///< Enable/disable drawing to display area.
    forceSetMaskBit     = 0x00000800u, ///< Set mask bit when drawing pixels (on: set to 1 / off: use bit15 of texture color or 0 if untextured).
    enableMask          = 0x00001000u, ///< Disable drawing to pixels with mask bit (on/off).
    interlaceField      = 0x00002000u, ///< Interlace field: top field (odd) = 1 / bottom field (even) = 0 (always 1 when 'verticalInterlacing' is off).
    reverseFlag         = 0x00004000u, ///< Reverse flag: display scrolls down 2 lines + colored regions are distorted (on / off).
    disableTextures     = 0x00008000u, ///< Draw without textures when GP1(09).0 is set to 1 (on / off).

    // control status
    displayAreaWidth1   = 0x00010000u, ///< Display area width in framebuffer - part 1 (0: use part2 / 1: 368 (384 with overscan)).
    displayAreaWidth2   = 0x00060000u, ///< Display area width in framebuffer - part 2 (256/320/512/640).
    displayAreaHeight   = 0x00080000u, ///< Display area height in framebuffer (NTSC: 224/448 (240/480 with overscan) -- PAL: 256/512 (underscan of 264/528))
                                       ///  Height is only doubled (480 or 512) when 'verticalInterlacing' is also on.
    videoStandard       = 0x00100000u, ///< SMPTE video standard (0: NTSC / 1: PAL).
    colorDepth          = 0x00200000u, ///< Display area color depth mode (1: 24-bit direct mode / 0: 15-bit direct mode).
                                        ///  No effect on draw area (always 15-bit colors).
    verticalInterlacing = 0x00400000u, ///< Interlace mode (on/off).
    disableDisplay      = 0x00800000u, ///< Enable/disable display (1: draw black area / 0: draw normal display).
    interruptReq1       = 0x01000000u, ///< Interrupt request IRQ1 received (on/off).
    dmaRequestState     = 0x02000000u, ///< Direct memory access request state, based on 'dmaMode'
                                       ///  (off: 0; fifoStatus: FIFO ready(1)/full(0); cpuToGpu: 'readyForDmaWrite'; gpuToCpu: 'readyForDmaRead').
    readyForCommands    = 0x04000000u, ///< GPU availability (1: idle and ready for command / 0: busy or waiting to receive data).
    readyForDmaRead     = 0x08000000u, ///< DMA read status (1: ready to send image / 0: busy).
                                       ///  Set to 0 when command + params received (GP0(C0)).
    readyForDmaBlock    = 0x10000000u, ///< GPU readiness for DMA blocks (1: ready to receive commands / 0: busy).
                                       ///  Normally set to 0 when command + params received.
                                       ///  For polygon and line commands, immediately set to 0 (before receiving vertex data).
    dmaMode             = 0x60000000u, ///< Direct memory access mode (off/fifoStatus/cpuToGpu/gpuToCpu)
    drawingOddLines     = 0x80000000u, ///< Current field drawn (interlaced) or current scanline (progressive) (1: odd lines / 0: even lines).
                                       ///  Changes per field (interlaced) or per scaline (progressive). Set to 0 during VBlank.

    // data exchange status - special arcade GPU
    arcade2_texturePageBaseY   = 0x00000070u,
    arcade2_texturePageAlignedY= 0x00000060u,
    arcade2_semiTransparency   = 0x00000180u,
    arcade2_texturePageColors  = 0x00000600u
  };
  constexpr inline int bitOffset_forceSetMaskBit() noexcept { return 11; } ///< Offset of StatusBits::forceSetMaskBit
  constexpr inline int bitOffset_reverseFlag() noexcept     { return 14; } ///< Offset of StatusBits::reverseFlag
  constexpr inline int bitOffset_disableTextures() noexcept { return 15; } ///< Offset of StatusBits::disableTextures
  constexpr inline int bitOffset_displayAreaWidth1() noexcept{ return 16; }///< Offset of StatusBits::displayAreaWidth1
  constexpr inline int bitOffset_displayAreaWidth2() noexcept{ return 17; }///< Offset of StatusBits::displayAreaWidth2
  constexpr inline int bitOffset_dmaRequestState() noexcept { return 25; } ///< Offset of StatusBits::dmaRequestState
  constexpr inline int bitOffset_readyForDmaRead() noexcept { return 27; } ///< Offset of StatusBits::readyForDmaRead
  constexpr inline int bitOffset_readyForDmaWrite() noexcept{ return 28; } ///< Offset of StatusBits::readyForDmaWrite
  constexpr inline int bitOffset_dmaMode() noexcept         { return 29; } ///< Offset of StatusBits::dmaMode

  constexpr inline unsigned long statusControlDefaults() noexcept { ///< Default value of status register (after reset)
    return ((unsigned long)StatusBits::readyForDmaBlock | (unsigned long)StatusBits::readyForCommands
          | (unsigned long)StatusBits::disableDisplay   | (unsigned long)StatusBits::interlaceField);
  }
  constexpr inline unsigned long displayModeBits() noexcept { ///< All bits affected by display mode command (GP1(0x08))
    return ((unsigned long)StatusBits::verticalInterlacing | (unsigned long)StatusBits::colorDepth
          | (unsigned long)StatusBits::videoStandard       | (unsigned long)StatusBits::displayAreaHeight
          | (unsigned long)StatusBits::displayAreaWidth1   | (unsigned long)StatusBits::displayAreaWidth2
          | (unsigned long)StatusBits::reverseFlag);
  }
  constexpr inline unsigned long texturePageBits() noexcept { ///< All bits affected by texture page command (GP0(0xE1))
    return ((unsigned long)StatusBits::texturePageBaseX | (unsigned long)StatusBits::texturePageBaseY
          | (unsigned long)StatusBits::semiTransparency | (unsigned long)StatusBits::texturePageColors
          | (unsigned long)StatusBits::dithering        | (unsigned long)StatusBits::drawToDisplay
          | (unsigned long)StatusBits::disableTextures);
  }
  constexpr inline unsigned long arcade2_texturePageBits() noexcept { ///< Texture page bits for special arcade GPU
    return ((unsigned long)StatusBits::texturePageBaseX         | (unsigned long)StatusBits::arcade2_texturePageBaseY
          | (unsigned long)StatusBits::arcade2_semiTransparency | (unsigned long)StatusBits::arcade2_texturePageColors
          | (unsigned long)0x00001800u); // unknown content -> ignore mask/dithering/drawToDisplay with arcadeGpu2
  }
  
  
  // -- hardware management -- -------------------------------------------------

  /// @brief GPU hardware version
  enum class GpuVersion : unsigned long {
    psxGpu208pin = 0, ///< Standard PS1 GPU (208-pin)
    arcadeGpu1  = 1,  ///< Standard arcade GPU (close to PS1 GPU)
    arcadeGpu2   = 2  ///< Special arcade GPU
  };
  constexpr inline unsigned long psxVramHeight() noexcept { return 512; }       ///< Standard GPU VRAM height (texels)
  constexpr inline unsigned long znArcadeVramHeight() noexcept { return 1024; } ///< Special arcade GPU VRAM height (texels)
  constexpr inline unsigned long maxLightgunCursors() noexcept { return 8u; }   ///< Max number of lightgun cursors
  constexpr inline unsigned long biosModuleAddress() noexcept { return 0xBFC03720uL; } ///< BIOS module address returned as GPU info (GPU patch?)

  /// @brief Data transfer mode: primitives / VRAM transfer
  /// @remarks Represents current data transfer (set after receiving or completing VRAM transfer commands)
  enum class DataTransfer : long {
    primitives = 0,
    vramWrite  = 1,
    vramRead   = 2
  };


  // -- display state -- -------------------------------------------------------

# define __TV_RANGE_OFFSET_X        0x260 // offset to first visible pixel on most CRT TVs
# define __TV_RANGE_AVERAGE_WIDTH_X 2560  // average range width on most CRT TVs
# define __TV_RANGE_CENTER_Y_NTSC   0x88  // middle line offset (NTSC)
# define __TV_RANGE_CENTER_Y_PAL    0xA3  // middle line offset (PAL)

# define __DEFAULT_RANGE_X0  __TV_RANGE_OFFSET_X
# define __DEFAULT_RANGE_X1  (__DEFAULT_RANGE_X0+__TV_RANGE_AVERAGE_WIDTH_X)
# define __DEFAULT_RANGE_Y0  0x10
# define __DEFAULT_RANGE_Y1  (__DEFAULT_RANGE_Y0+240)
  
  /// @brief Pixel position or size
  struct Point final {
    long x = 0;
    long y = 0;
  };
  /// @brief Pixel range boundaries (rectangle)
  struct Rectangle final {
    long leftX = 0;
    long rightX = 0;
    long topY = 0;
    long bottomY = 0;
  };
  /// @brief Texture window location
  struct TextureWindow final {
    long offsetX = 0;
    long offsetY = 0;
    long maskWidth = 256;
    long maskHeight = 256;
    bool isEnabled = false;
  };

  /// @brief Display mode: display/draw area, range, offset, status
  struct DisplayState final {
    Rectangle displayRange{ __DEFAULT_RANGE_X0, __DEFAULT_RANGE_X1,
                            __DEFAULT_RANGE_Y0, __DEFAULT_RANGE_Y1 }; ///< Display range (horizontal/vertical screen range)
    Rectangle drawArea;              ///< Drawing area boundaries: x0/x1 [0;1023], y0/y1 [0;511]
    Point drawOffset;                ///< Drawing offset: x/y [-1024; 1023]
    Point displayAreaSize{ 256,240 };///< Display area pixel size
    Point displayOrigin;             ///< Display source area in VRAM: x [0;1023], y [0;511]
    long cyclesPerPixel = __TV_RANGE_AVERAGE_WIDTH_X/256;
  };
}
