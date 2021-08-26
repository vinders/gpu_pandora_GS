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
#include <cstring>
#include "display/status_register.h"

using namespace display;


// -- hardware info & transfer mode -- -----------------------------------------

void StatusRegister::requestGpuInfo(unsigned long params) noexcept {
  GpuInfoType info = (GpuInfoType)(params & 0xFu);
  switch (info) {
    // texture window status
    case GpuInfoType::textureWindow:
      this->_gpuReadBuffer = (this->_textureWindow.maskWidth | (this->_textureWindow.maskHeight << 5)
                           | (this->_textureWindow.offsetX << 10) | (this->_textureWindow.offsetY << 15));
      break;
    // draw area
    case GpuInfoType::drawAreaStartCoords:
      this->_gpuReadBuffer = (this->_gpuType != GpuVersion::arcadeGpu2)
                           ? (this->_displayState.drawArea.leftX | (this->_displayState.drawArea.topY << 10))
                           : (this->_displayState.drawArea.leftX | (this->_displayState.drawArea.topY << 12));
      break;
    case GpuInfoType::drawAreaEndCoords:
      this->_gpuReadBuffer = (this->_gpuType != GpuVersion::arcadeGpu2)
                           ? (this->_displayState.drawArea.rightX | (this->_displayState.drawArea.bottomY << 10))
                           : (this->_displayState.drawArea.rightX | (this->_displayState.drawArea.bottomY << 12));
      break;
    case GpuInfoType::drawOffset:
      this->_gpuReadBuffer = (this->_gpuType != GpuVersion::arcadeGpu2)
                           ? ( ((unsigned long)this->_displayState.drawOffset.x & 0x7FFu)
                             | (((unsigned long)this->_displayState.drawOffset.y & 0x7FFu) << 11) )
                           : ( ((unsigned long)this->_displayState.drawOffset.x & 0x7FFu)
                             | (((unsigned long)this->_displayState.drawOffset.y & 0x7FFu) << 12) );
      break;

    // hardware version: standard PS1 GPU / special arcade GPU
    case GpuInfoType::gpuVersion:
      this->_gpuReadBuffer = (this->_gpuType != GpuVersion::arcadeGpu2) ? 2u : 1u;
      break;
    // lightgun detection status -> BIOS address if present
    case GpuInfoType::lightgunStatus:
      this->_gpuReadBuffer = this->_activeLightguns ? biosModuleAddress() : 0;
      break;
    // BIOS module address
    case GpuInfoType::biosAddress:
      this->_gpuReadBuffer = biosModuleAddress();
      break;
    default: break;
  }
}

void StatusRegister::setDmaMode(unsigned long params) noexcept {
  // set DMA mode bits
  DmaMode mode = (DmaMode)((params << bitOffset_dmaMode()) & (unsigned long)StatusBits::dmaMode);
  this->_statusControlRegister &= ~((unsigned long)StatusBits::dmaMode | (unsigned long)StatusBits::dmaRequestState);
  this->_statusControlRegister |= (unsigned long)mode;

  // set DMA status bit (+ update data transfer mode)
  switch (mode) {
    case DmaMode::fifoStatus: // query FIFO state -> note: no FIFO implemented (commands are synchronous)
      //if (command FIFO is full)
      //  this->_statusControlRegister |= (unsigned long)StatusBits::dmaRequestState;
      this->_dataTransferMode = DataTransfer::primitives;
      break;
    case DmaMode::cpuToGpu: // CPU -> GPU (write DMA)
      this->_statusControlRegister |= ((unsigned long)mode & (unsigned long)StatusBits::readyForDmaBlock)
                                      >> (bitOffset_readyForDmaWrite() - bitOffset_dmaRequestState());
      this->_dataTransferMode = DataTransfer::vramWrite;
      break;
    case DmaMode::gpuToCpu: // GPU -> CPU (read DMA)
      this->_statusControlRegister |= ((unsigned long)mode & (unsigned long)StatusBits::readyForDmaRead)
                                      >> (bitOffset_readyForDmaRead() - bitOffset_dmaRequestState());
      this->_dataTransferMode = DataTransfer::vramRead;
      break;
    default: 
      this->_dataTransferMode = DataTransfer::primitives;
      break;
  }
}

void StatusRegister::setLightgunCursor(unsigned long player, long x, long y) noexcept {
  if (player >= maxLightgunCursors()) // negative or above max
    return;
  this->_activeLightguns |= (1u << player);

  if (x < 0)
    x = 0;
  else if (x >= (int)this->_vramHeight)
    x = (int)this->_vramHeight - 1;
  if (y < 0)
    y = 0;
  else if (y > 255)
    y = 255;

  this->_lightgunCursors[player].x = x;
  this->_lightgunCursors[player].y = y;
}


// -- display mode control -- --------------------------------------------------

#define __SET_HISTORY(history,cmdId,value)   history[(size_t)cmdId] = (((unsigned long)cmdId << 24) | value)

void StatusRegister::resetControlCommandHistory(unsigned long history[controlCommandNumber()]) noexcept {
  memset(history, 0, controlCommandNumber()*sizeof(unsigned long));
  __SET_HISTORY(history, ControlCommandId::clearCommandFifo, 0);
  __SET_HISTORY(history, ControlCommandId::ackIrq1, 0);
  __SET_HISTORY(history, ControlCommandId::toggleDisplay, 0x1); // display off
  __SET_HISTORY(history, ControlCommandId::dmaMode, 0);
  __SET_HISTORY(history, ControlCommandId::displayAreaOrigin, 0);
  __SET_HISTORY(history, ControlCommandId::horizontalDisplayRange,((__DEFAULT_RANGE_X1<<12)|__DEFAULT_RANGE_X0)); // range X
  __SET_HISTORY(history, ControlCommandId::verticalDisplayRange,  ((__DEFAULT_RANGE_Y1<<10)|__DEFAULT_RANGE_Y0)); // range Y
  __SET_HISTORY(history, ControlCommandId::displayMode, 0);
}

// ---

void StatusRegister::resetGpu() noexcept {
  // reset status/control register
  _statusControlRegister = statusControlDefaults();
  this->_dataTransferMode = DataTransfer::primitives;
  this->_gpuBusyHackCounter = 0;

  // reset display state
  this->_displayState = DisplayState{};
  this->_texpageBaseX = this->_texpageBaseY = 0;
  this->_isTextureFlipX = this->_isTextureFlipY = false;
  this->_isTextureDecodingIL = false;
  this->_textureWindow = TextureWindow{};
}

void StatusRegister::setDisplayMode(unsigned long params) noexcept {
  unsigned long prevInterlacing = readStatus(StatusBits::verticalInterlacing);

  // update status/control register
  this->_statusControlRegister &= ~(displayModeBits());
  this->_statusControlRegister |= ((params & 0x3Fu) << bitOffset_displayAreaWidth2())
                                | ((params & 0x40u) << (bitOffset_displayAreaWidth1() - 6))
                                | ((params & 0x80u) << (bitOffset_reverseFlag() - 7));
  if (prevInterlacing != readStatus(StatusBits::verticalInterlacing))
    this->_statusControlRegister |= (unsigned long)StatusBits::interlaceField;

  // compute new display area size - X
  if (readStatus<bool>(StatusBits::displayAreaWidth1)) {
    this->_displayState.displayAreaSize.x = 384;
    this->_displayState.cyclesPerPixel = 7; // round(2560 / 368)
  }
  else {
    this->_displayState.displayAreaSize.x = (256 + ((params&0x1)<<6) ) << ((params&0x2)>>1); // 00: 256 / 01: 320 / 02: 512 / 03: 640
    this->_displayState.cyclesPerPixel = __TV_RANGE_AVERAGE_WIDTH_X / this->_displayState.displayAreaSize.x; // 10 / 8 / 5 / 4
  }

  // compute new display area size - Y
  if (readStatus<SmpteStandard>(StatusBits::videoStandard) == SmpteStandard::ntsc)
    this->_displayState.displayAreaSize.y = readStatus<bool>(StatusBits::displayAreaHeight) ? 480 : 240;
  else // PAL
    this->_displayState.displayAreaSize.y = readStatus<bool>(StatusBits::displayAreaHeight) ? 512 : 256;
}

// ---

void StatusRegister::setTexturePageMode(unsigned long params) noexcept {
  if (this->_gpuType != GpuVersion::arcadeGpu2) { // standard PS1 GPU / regular arcade GPUs
    this->_statusControlRegister &= ~(texturePageBits());
    this->_statusControlRegister |= (params & 0x7FFu)
                                  | ((params & 0x800u) << (bitOffset_disableTextures() - 11));
    if (readStatus(StatusBits::texturePageColors) == (unsigned long)TextureColorMode::reserved)
      this->_statusControlRegister &= ~((unsigned long)TextureColorMode::lookupTable8bit); // "reserved" -> to "15-bit" mode

    this->_texpageBaseX = (params & (unsigned long)StatusBits::texturePageBaseX) << 6;
    this->_texpageBaseY = (params & (unsigned long)StatusBits::texturePageBaseY) << 4;
    if (this->_vramHeight == znArcadeVramHeight())
      this->_texpageBaseY |= (params & 0x800u) >> 2;

    this->_isTextureFlipX = (params & 0x1000u);
    this->_isTextureFlipY = (params & 0x2000u);
  }
  else { // special arcade GPU
    this->_statusControlRegister &= ~(arcade2_texturePageBits());
    this->_statusControlRegister |= (params & arcade2_texturePageBits());
    if (readStatus(StatusBits::arcade2_texturePageColors) == (unsigned long)StatusBits::arcade2_texturePageColors)
      this->_statusControlRegister &= ~((unsigned long)TextureColorMode::lookupTable8bit << 2); // "reserved" -> to "15-bit" mode

    this->_texpageBaseX = (params & (unsigned long)StatusBits::texturePageBaseX) << 6;
    this->_texpageBaseY = (params & (unsigned long)StatusBits::arcade2_texturePageAlignedY) << 3;
    this->_isTextureDecodingIL = (params & 0x2000u);
  }
}

void StatusRegister::setTextureWindow(unsigned long params) noexcept {
  // texture window width/height must be a power of 2 (or 0 for 256) -> multiplied by 8 for texels
  // -> params may be invalid: verify bit by bit
  if (params & 0x01)      // xxxx1
    this->_textureWindow.maskWidth = 8;
  else if (params & 0x02) // xxx10
    this->_textureWindow.maskWidth = 16;
  else if (params & 0x04) // xx100
    this->_textureWindow.maskWidth = 32;
  else if (params & 0x08) // x1000
    this->_textureWindow.maskWidth = 64;
  else if (params & 0x10) // 10000
    this->_textureWindow.maskWidth = 128;
  else                    // 00000
    this->_textureWindow.maskWidth = 256;

  if (params & (0x01<<5))      // xxxx1
    this->_textureWindow.maskHeight = 8;
  else if (params & (0x02<<5)) // xxx10
    this->_textureWindow.maskHeight = 16;
  else if (params & (0x04<<5)) // xx100
    this->_textureWindow.maskHeight = 32;
  else if (params & (0x08<<5)) // x1000
    this->_textureWindow.maskHeight = 64;
  else if (params & (0x10<<5)) // 10000
    this->_textureWindow.maskHeight = 128;
  else                         // 00000
    this->_textureWindow.maskHeight = 256;
  
  // texture window offset: x multiple of width, y multiple of height, offset+size <= 256
  long filterX = (0x100000 - (this->_textureWindow.maskWidth >> 3));
  long filterY = (0x100000 - (this->_textureWindow.maskHeight >> 3));
  this->_textureWindow.offsetX = (((params >> 10) & 0x1Fu) & filterX) << 3;
  this->_textureWindow.offsetY = (((params >> 15) & 0x1Fu) & filterY) << 3;

  this->_textureWindow.isEnabled = (this->_textureWindow.offsetX || this->_textureWindow.offsetY
                                 || this->_textureWindow.maskWidth < 256 || this->_textureWindow.maskHeight < 256);
}


// -- display area/range & draw area -- --------------------------------------

void StatusRegister::setDisplayAreaOrigin(unsigned long params) noexcept {
  this->_displayState.displayOrigin.x = (params & 0x3FF); // [0; 1023]
  if (this->_vramHeight == psxVramHeight())
    this->_displayState.displayOrigin.y = ((params >> 10) & 0x1FFu); // [0; 511]
  else if (this->_gpuType == GpuVersion::arcadeGpu2)
    this->_displayState.displayOrigin.y = ((params >> 12) & 0x3FFu);
  else
    this->_displayState.displayOrigin.y = ((params >> 10) & 0x3FFu);
}

void StatusRegister::setHorizontalDisplayRange(unsigned long params) noexcept {
  this->_displayState.displayRange.leftX = (params & 0xFFFu);
  this->_displayState.displayRange.rightX = ((params >> 12) & 0xFFFu);
}

void StatusRegister::setVerticalDisplayRange(unsigned long params) noexcept {
  this->_displayState.displayRange.bottomY = ((params >> 10) & 0x3FFu);
  this->_displayState.displayRange.topY = (params & 0x3FFu);
}

// ---

void StatusRegister::setDrawAreaOrigin(unsigned long params) noexcept {
  this->_displayState.drawArea.leftX = (params & 0x3FFu); // [0; 1023]
  if (this->_vramHeight == psxVramHeight())
    this->_displayState.drawArea.topY = ((params >> 10) & 0x1FFu); // [0; 511]
  else if (this->_gpuType == GpuVersion::arcadeGpu2)
    this->_displayState.drawArea.topY = ((params >> 12) & 0x3FFu);
  else
    this->_displayState.drawArea.topY = ((params >> 10) & 0x3FFu);
}

void StatusRegister::setDrawAreaEnd(unsigned long params) noexcept {
  this->_displayState.drawArea.rightX = (params & 0x3FFu); // [0; 1023]
  if (this->_vramHeight == psxVramHeight())
    this->_displayState.drawArea.bottomY = ((params >> 10) & 0x1FFu); // [0; 511]
  else if (this->_gpuType == GpuVersion::arcadeGpu2)
    this->_displayState.drawArea.bottomY = ((params >> 12) & 0x3FFu);
  else
    this->_displayState.drawArea.bottomY = ((params >> 10) & 0x3FFu);
}

void StatusRegister::setDrawOffset(unsigned long params) noexcept {
  this->_displayState.drawOffset.x = (long)(params & 0x7FF); // [-1024; 1023]
  if (params & 0x400u) // negative
    this->_displayState.drawOffset.x |= ~(long)0x7FF;

  if (this->_gpuType == GpuVersion::arcadeGpu2)
    this->_displayState.drawOffset.y = (long)((params >> 12) & 0x7FF);
  else
    this->_displayState.drawOffset.y = (long)((params >> 11) & 0x7FF); // [-1024; 1023]
  if (this->_displayState.drawOffset.y & 0x400) // negative
    this->_displayState.drawOffset.y |= ~(long)0x7FF;
}
