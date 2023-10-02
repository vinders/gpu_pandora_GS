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
#include <cstddef>
#include <cstring>
#include <system/preprocessor_tools.h>
#include "display/status_register.h"
#include "display/renderer.h"
#include "display/primitives.h"
#if !defined(_CPP_REVISION) || _CPP_REVISION != 14
# define __if_constexpr if constexpr
#else
# define __if_constexpr if
#endif
#define __MAX_GP0_PARAMS_LENGTH 0xFF
#define __MAX_INT32 0x7FFFFFFF

using namespace display;


// -- helpers -- ---------------------------------------------------------------

// GP0 draw command ID - bitmap components
enum class Gp0DrawCmdBit : unsigned long {
  noTextureBlending = 0x1, ///< Raw texturing (1) / texture blending (0)
  semiTransparent   = 0x2, ///< Semi-transparent (1) / opaque (0)
  textured          = 0x4, ///< Textured (1) / Simple color (0)
  shaded            = 0x10 ///< Gouraud shaded (1) / Flat color (0) -- only for polygons and lines
};
#define __GP0_POLY_CMD_BIT_MASK          0x12
#define __GP0_POLY_CMD_BIT_MASK_TEXTURED 0x17
#define __GP0_LINE_CMD_BIT_MASK          0x12
#define __GP0_TILE_CMD_BIT_MASK          0x02
#define __GP0_TILE_CMD_BIT_MASK_TEXTURED 0x07

static constexpr inline bool hasGp0CommandBit(Gp0DrawCmdBit id, Gp0DrawCmdBit bit) noexcept {
  return (((unsigned long)id & (unsigned long)bit) == (unsigned long)bit);
}


// -- GP0 commands - general -- ------------------------------------------------

static void clearTextureCache(StatusRegister&, Renderer&, uint32_t*) noexcept {

}

static void fillVramRectangle(StatusRegister&, Renderer&, uint32_t*) noexcept {

}

static void requestIrq1(StatusRegister& status, Renderer&, uint32_t*) noexcept {
  status.setIrq1();
}


// -- GP0 commands - primitives -- ---------------------------------------------

template <Gp0DrawCmdBit _CmdId>
static void drawTriangle(StatusRegister&, Renderer&, uint32_t*) noexcept {
  __if_constexpr (hasGp0CommandBit(_CmdId, Gp0DrawCmdBit::textured)) {

  }
  else {
    
  }
}

template <Gp0DrawCmdBit _CmdId>
static void drawQuad(StatusRegister&, Renderer&, uint32_t*) noexcept {
  __if_constexpr (hasGp0CommandBit(_CmdId, Gp0DrawCmdBit::textured)) {

  }
  else {
    
  }
}

// ---

static constexpr inline bool isGp0PolyLineTermination(uint32_t param) noexcept {
  return ((param & 0xF000F000) == 0x50005000);
}

template <Gp0DrawCmdBit _CmdId>
static void drawLine(StatusRegister&, Renderer&, uint32_t*) noexcept {

}

template <Gp0DrawCmdBit _CmdId>
static void drawPolyLine(StatusRegister&, Renderer&, uint32_t*) noexcept {

}

// ---

template <Gp0DrawCmdBit _CmdId>
static void drawCustomTile(StatusRegister&, Renderer&, uint32_t*) noexcept {
  __if_constexpr (hasGp0CommandBit(_CmdId, Gp0DrawCmdBit::textured)) {

  }
  else {
    
  }
}

template <Gp0DrawCmdBit _CmdId>
static void drawTile1x1(StatusRegister&, Renderer&, uint32_t*) noexcept {
  __if_constexpr (hasGp0CommandBit(_CmdId, Gp0DrawCmdBit::textured)) {

  }
  else {
    
  }
}

template <Gp0DrawCmdBit _CmdId>
static void drawTile8x8(StatusRegister&, Renderer&, uint32_t*) noexcept {
  __if_constexpr (hasGp0CommandBit(_CmdId, Gp0DrawCmdBit::textured)) {

  }
  else {
    
  }
}

template <Gp0DrawCmdBit _CmdId>
static void drawTile16x16(StatusRegister&, Renderer&, uint32_t*) noexcept {
  __if_constexpr (hasGp0CommandBit(_CmdId, Gp0DrawCmdBit::textured)) {

  }
  else {

  }
}


// -- GP0 commands - framebuffer data transfers -- -----------------------------

unsigned long Primitives::imgWidth__TMP = 0;//TODO: replace with VRAM reader/writer
unsigned long Primitives::imgHeight__TMP = 0;

static void copyVramRectangle(StatusRegister&, Renderer&, uint32_t*) noexcept {

}

static void writeVramRectangle(StatusRegister& status, Renderer&, uint32_t* params) noexcept {
  ++params;
  //unsigned long x = (*params & 0x3FFu);
  //unsigned long y = (status.getGpuVramHeight() == display::psxVramHeight()) ? ((*params >> 16) & 0x1FFu) : ((*params >> 16) & 0x3FFu);

  ++params;
  Primitives::imgWidth__TMP = ((*params - 1u) & 0x3FFu) + 1u;
  Primitives::imgHeight__TMP = (((*params >> 16) - 1u) & 0x1FFu) + 1u;

  status.setDataWriteMode(display::DataTransfer::vramTransfer);
}

static void readVramRectangle(StatusRegister& status, Renderer&, uint32_t*) noexcept {
  status.setDataReadMode(display::DataTransfer::vramTransfer);
  status.setVramReadPending();
}


// -- GP0 commands - rendering attributes -- -----------------------------------

static void setTexturePage(StatusRegister& status , Renderer&, uint32_t* params) noexcept {
  status.setTexturePageMode((unsigned long)*params);
}

static void setTextureWindow(StatusRegister& status, Renderer&, uint32_t* params) noexcept {
  status.setTextureWindow((unsigned long)*params);
}

static void setDrawAreaOrigin(StatusRegister& status, Renderer&, uint32_t* params) noexcept {
  status.setDrawAreaOrigin((unsigned long)*params);
}

static void setDrawAreaEnd(StatusRegister& status, Renderer&, uint32_t* params) noexcept {
  status.setDrawAreaEnd((unsigned long)*params);
}

static void setDrawOffset(StatusRegister& status, Renderer&, uint32_t* params) noexcept {
  status.setDrawOffset((unsigned long)*params);
}

static void setMaskBit(StatusRegister& status, Renderer&, uint32_t* params) noexcept {
  status.setMaskBit((unsigned long)*params);
}


// -- GP0 command table (primitives + rendering attributes) -- -----------------

struct Gp0Command final {
  void (*runner)(StatusRegister&, Renderer&, uint32_t* params) noexcept;
  int paramsLength;
};

#define NOP Gp0Command({ nullptr, 1 })
#define CMD_4X(baseId, idMask, runner, size)  { runner <(Gp0DrawCmdBit)( baseId    & idMask)>, size }, \
                                              { runner <(Gp0DrawCmdBit)((baseId+1) & idMask)>, size }, \
                                              { runner <(Gp0DrawCmdBit)((baseId+2) & idMask)>, size }, \
                                              { runner <(Gp0DrawCmdBit)((baseId+3) & idMask)>, size }
#define CMD_8X(baseId, idMask, runner, size)  CMD_4X(baseId,idMask,runner,size),CMD_4X(baseId+4,idMask,runner,size)

uint32_t g_truncatedParams[__MAX_GP0_PARAMS_LENGTH+1];
int g_truncatedParamsLength = 0;

// ---

static constexpr const Gp0Command g_gp0CommandTable[0x100] = {
  // 0x00: general
  NOP,
  { clearTextureCache, 1 },
  { fillVramRectangle, 3 },
  NOP, // 0x03: unknown
  _P_DUPLICATE_24X_COMMA(NOP),
  _P_DUPLICATE_3X_COMMA(NOP), 
  { requestIrq1, 1 },

  // 0x20: draw polygons (triangles/quads)
  CMD_4X(0x20, __GP0_POLY_CMD_BIT_MASK,          drawTriangle, 4),
  CMD_4X(0x24, __GP0_POLY_CMD_BIT_MASK_TEXTURED, drawTriangle, 7),
  CMD_4X(0x28, __GP0_POLY_CMD_BIT_MASK,          drawQuad, 5),
  CMD_4X(0x2C, __GP0_POLY_CMD_BIT_MASK_TEXTURED, drawQuad, 9),
  CMD_4X(0x30, __GP0_POLY_CMD_BIT_MASK,          drawTriangle, 6),
  CMD_4X(0x34, __GP0_POLY_CMD_BIT_MASK_TEXTURED, drawTriangle, 9),
  CMD_4X(0x38, __GP0_POLY_CMD_BIT_MASK,          drawQuad, 8),
  CMD_4X(0x3C, __GP0_POLY_CMD_BIT_MASK_TEXTURED, drawQuad, 12),

  // 0x40: draw lines
  CMD_8X(0x40, __GP0_LINE_CMD_BIT_MASK, drawLine, 3),
  CMD_8X(0x48, __GP0_LINE_CMD_BIT_MASK, drawPolyLine, __MAX_GP0_PARAMS_LENGTH-1),
  CMD_8X(0x50, __GP0_LINE_CMD_BIT_MASK, drawLine, 4),
  CMD_8X(0x58, __GP0_LINE_CMD_BIT_MASK, drawPolyLine, __MAX_GP0_PARAMS_LENGTH),

  // 0x60: draw tiles (rectangles/sprites)
  CMD_4X(0x60, __GP0_TILE_CMD_BIT_MASK,          drawCustomTile, 3),
  CMD_4X(0x64, __GP0_TILE_CMD_BIT_MASK_TEXTURED, drawCustomTile, 4),
  CMD_4X(0x68, __GP0_TILE_CMD_BIT_MASK,          drawTile1x1, 2),
  CMD_4X(0x6C, __GP0_TILE_CMD_BIT_MASK_TEXTURED, drawTile1x1, 3),
  CMD_4X(0x70, __GP0_TILE_CMD_BIT_MASK,          drawTile8x8, 2),
  CMD_4X(0x74, __GP0_TILE_CMD_BIT_MASK_TEXTURED, drawTile8x8, 3),
  CMD_4X(0x78, __GP0_TILE_CMD_BIT_MASK,          drawTile16x16, 2),
  CMD_4X(0x7C, __GP0_TILE_CMD_BIT_MASK_TEXTURED, drawTile16x16, 3),

  // 0x80: framebuffer data transfers
  _P_DUPLICATE_32X_COMMA(Gp0Command({ copyVramRectangle, 4 })),
  _P_DUPLICATE_32X_COMMA(Gp0Command({ writeVramRectangle, 3 })),
  _P_DUPLICATE_32X_COMMA(Gp0Command({ readVramRectangle, 3 })),

  // 0xE0: rendering attributes
  NOP,
  { setTexturePage, 1 },
  { setTextureWindow, 1 },
  { setDrawAreaOrigin, 1 },
  { setDrawAreaEnd, 1 },
  { setDrawOffset, 1 },
  { setMaskBit, 1 },
  NOP,
  _P_DUPLICATE_8X_COMMA(NOP),

  // 0xF0: reserved (not implemented)
  _P_DUPLICATE_16X_COMMA(NOP)
};

// ---

static inline bool canGp0CommandBeSkipped(uint32_t firstMemBlock) noexcept { // verify if a command can be skipped
  firstMemBlock &= 0xFF000000u;
  return (firstMemBlock >= 0x20000000u && firstMemBlock < 0x80000000u);
}
static constexpr inline bool isGp0PolyLineCommand(int maxSize) noexcept { // verify if a command is a poly-line
  return (maxSize >= __MAX_GP0_PARAMS_LENGTH-1);
}
static constexpr inline bool isGp0ShadedPolyLine(int maxSize) noexcept {  // verify if poly-line is gouraud-shaded
  return (maxSize == __MAX_GP0_PARAMS_LENGTH);
}

// Get remaining length of poly-line params (variable length)
// - memSize:       array size of 'mem'.
// - maxSize:       maximum total length of poly-line params.
// - existingSize:  length of poly-line params already read (from previous call(s)).
// - returns: actual length remaining (or __MAX_INT32 if termination not found and max size not reached).
static inline int getPolyLineRemLength(uint32_t* mem, int memSize, int maxSize, int existingSize) {
  if (maxSize < existingSize)
    return 0;
  int length = maxSize - existingSize; // if no termination found and enough memSize, use maxSize

  uint32_t* endOfMem;
  if (memSize < length) {
    length = __MAX_INT32; // memSize too small: if no termination found, maxSize won't be reached
    endOfMem = mem + (intptr_t)memSize;
  }
  else
    endOfMem = mem + (intptr_t)length;

  if (isGp0ShadedPolyLine(maxSize)) {
    intptr_t memMinSize = (existingSize < 4) ? 4 - (intptr_t)existingSize : 0;
    for (uint32_t* it = mem + memMinSize; it < endOfMem; it += 2) {
      if (isGp0PolyLineTermination(*it)) {
        length = (static_cast<int>((const char*)it - (const char*)mem) / (int)sizeof(*mem)) + 1;
        break;
      }
    }
  }
  else {
    intptr_t memMinSize = (existingSize < 3) ? 3 - (intptr_t)existingSize : 0;
    for (uint32_t* it = mem + memMinSize; it < endOfMem; ++it) {
      if (isGp0PolyLineTermination(*it)) {
        length = (static_cast<int>((const char*)it - (const char*)mem) / (int)sizeof(*mem)) + 1;
        break;
      }
    }
  }
  return length;
}


// -- GP0 command interface -- -------------------------------------------------

// Clear pending command data buffer
void Primitives::clearCommandBuffer() noexcept {
  g_truncatedParamsLength = 0;
  //TODO: clear current VRAM transfer ???
}

// Run GP0 rendering command (drawing & rendering attributes)
// returns: size used by current command
int Primitives::runGp0Command(StatusRegister& status, Renderer& renderer,
                              uint32_t* mem, int size, bool isFrameSkipped) noexcept {
  const auto& command = (g_truncatedParamsLength == 0)
                      ? g_gp0CommandTable[StatusRegister::getGp0CommandId((unsigned long)*mem)]
                      : g_gp0CommandTable[StatusRegister::getGp0CommandId((unsigned long)*g_truncatedParams)];

  if (command.runner != nullptr) { // implemented operation
    int remainingLength = (isGp0PolyLineCommand(command.paramsLength))
                        ? getPolyLineRemLength(mem, size, command.paramsLength, g_truncatedParamsLength) // variable length
                        : command.paramsLength - g_truncatedParamsLength; // fixed length

    // full command / end of truncated command
    if (remainingLength <= size) {
      if (g_truncatedParamsLength > 0) {
        memcpy(&g_truncatedParams[g_truncatedParamsLength], mem, remainingLength*sizeof(uint32_t));
        mem = &g_truncatedParams[0];
        g_truncatedParamsLength = 0;
      }

      if (!isFrameSkipped || !canGp0CommandBeSkipped(*mem))
        command.runner(status, renderer, mem);
      size = remainingLength;
    }
    // truncated -> store current data blocks
    else {
      memcpy(&g_truncatedParams[g_truncatedParamsLength], mem, size*sizeof(uint32_t));
      g_truncatedParamsLength += size;
    }
  }
  else { // NOP -> ignore
    g_truncatedParamsLength = 0;
    size = 1;
  }
  return size;
}
