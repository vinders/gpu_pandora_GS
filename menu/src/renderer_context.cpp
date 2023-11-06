/*******************************************************************************
Pandora GS - PSEmu-compatible GPU driver
Copyright (C) 2023  Romain Vinders

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation, version 2 of the License.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details (LICENSE file).
*******************************************************************************/
#include <cassert>
#include <cstring>
#include <display/geometry.h>
#include "menu/renderer_context.h"

// get 'define' string size (including ending zero)
#define CONST_STRING_SIZE(str)  (sizeof(str)/sizeof(char))
// append filename to path buffer (also copy ending zero)
#define APPEND_FILENAME_TO_BUFFER(buffer, fileName)  memcpy(buffer, fileName, CONST_STRING_SIZE(fileName))

#define TITLES_FONT_FILENAME "Orbitron-Medium.ttf"
#define LABELS_FONT_FILENAME "Coda-Regular.ttf"
#define INPUT_FONT_FILENAME  "Coda-Regular.ttf"
#define MAX_FONT_FILENAME_SIZE  20

using namespace video_api;
using namespace menu;


void RendererContext::initFonts(const char* fontDirectoryPath) {
  static_assert(CONST_STRING_SIZE(TITLES_FONT_FILENAME) <= MAX_FONT_FILENAME_SIZE
             && CONST_STRING_SIZE(LABELS_FONT_FILENAME) <= MAX_FONT_FILENAME_SIZE
             && CONST_STRING_SIZE(INPUT_FONT_FILENAME) <= MAX_FONT_FILENAME_SIZE,
             "RendererContext::initFonts: buffer size too short");

  // create path buffer for UI fonts
  size_t fontDirectoryPathSize = strlen(fontDirectoryPath);
  std::unique_ptr<char[]> pathBuffer;
  if (fontDirectoryPathSize) {
    assert(fontDirectoryPath[fontDirectoryPathSize - 1] == '/' || fontDirectoryPath[fontDirectoryPathSize - 1] == '\\');
    pathBuffer.reset(new char[fontDirectoryPathSize + MAX_FONT_FILENAME_SIZE]);
    memcpy(pathBuffer.get(), fontDirectoryPath, fontDirectoryPathSize*sizeof(char));
  }
  else {
    fontDirectoryPathSize = (size_t)2u;
    pathBuffer.reset(new char[MAX_FONT_FILENAME_SIZE + 2]);
    pathBuffer.get()[0] = '.';
    pathBuffer.get()[1] = '/';
  }
  char* bufferSuffixIt = pathBuffer.get() + (intptr_t)fontDirectoryPathSize;
  
  // load UI fonts
  assert(this->renderer_ != nullptr);
  APPEND_FILENAME_TO_BUFFER(bufferSuffixIt, TITLES_FONT_FILENAME);
  fonts[(size_t)FontType::titles].reset(new display::Font(*(this->renderer_), pathBuffer.get(), 24u));
  APPEND_FILENAME_TO_BUFFER(bufferSuffixIt, LABELS_FONT_FILENAME);
  fonts[(size_t)FontType::labels].reset(new display::Font(*(this->renderer_), pathBuffer.get(), 15u));
  APPEND_FILENAME_TO_BUFFER(bufferSuffixIt, INPUT_FONT_FILENAME);
  fonts[(size_t)FontType::inputText].reset(new display::Font(*(this->renderer_), pathBuffer.get(), 13u));
}

void RendererContext::release() noexcept {
  if (renderer_ != nullptr) {
    // release resources BEFORE renderer
    for (auto& font : fonts)
      font.reset(nullptr);
    imageLoader_.release();
    
    renderer_ = nullptr;
  }
}

bool RendererContext::onSizeChange(uint32_t clientWidth, uint32_t clientHeight) noexcept {
  if (this->clientWidth_ != clientWidth || this->clientHeight_ != clientHeight) {
    this->clientWidth_ = clientWidth;
    this->clientHeight_ = clientHeight;
    pixelSizeX_ = display::ToPixelSize(clientWidth);
    pixelSizeY_ = display::ToPixelSize(clientHeight);
    return true;
  }
  return false;
}
