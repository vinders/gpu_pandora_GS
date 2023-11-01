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
#define _CRT_SECURE_NO_WARNINGS
#include <cstdlib>
#include <stdexcept>
#include <memory>
#ifdef _WINDOWS
# include <system/api/windows_api.h>
# include <shlobj.h>
#endif
#include "display/font.h"
#include <ft2build.h>
#include FT_FREETYPE_H

using namespace video_api;
using namespace display;


std::string getSystemFontPath() {
# ifdef _WINDOWS
  PWSTR folderPath = nullptr;
  bool isSuccess = (SHGetKnownFolderPath(FOLDERID_Fonts, 0, nullptr, &folderPath) == S_OK);

  std::string path = "%windir%\\Fonts\\";
  if (isSuccess && folderPath != nullptr) {
    size_t bufferSize = wcslen((const wchar_t*)folderPath)*2 + 1;
    std::unique_ptr<char[]> buffer(new char[bufferSize]);

    if (wcstombs(buffer.get(), (const wchar_t*)folderPath, bufferSize)) {
      path = buffer.get();
      if (path.back() != '/' && path.back() != '\\')
        path += '\\';
    }
  }
  CoTaskMemFree((LPVOID)folderPath);

  return path + "arial.ttf";
# else
  return "/usr/share/fonts/truetype/arial.ttf";
# endif
}

// -----------------------------------------------------------------------------

static FT_Library freetypeInstance = nullptr;
static int32_t freetypeUsageCount = 0;

Font::Font(Renderer& renderer, const char* baseFontPath, uint32_t heightPixels, uint32_t customWidthPixels) {
  if (freetypeInstance == nullptr) { // init library
    if (FT_Init_FreeType(&freetypeInstance) || freetypeInstance == nullptr)
      throw std::runtime_error("Font: could not init FreeType Library");
  }
  ++freetypeUsageCount;

  FT_Face baseFace = nullptr; // load main font face
  if (FT_New_Face(freetypeInstance, baseFontPath, 0, &baseFace) || baseFace == nullptr)
    throw std::runtime_error("Font: failed to load font");
  baseFontFace = baseFace;
  FT_Set_Pixel_Sizes(baseFace, customWidthPixels, heightPixels);

  FT_Face systemFace = nullptr; // optional fallback (if available)
  auto systemFontPath = getSystemFontPath();
  if (FT_New_Face(freetypeInstance, systemFontPath.c_str(), 0, &systemFace) == 0) {
    systemFontFace = systemFace;
    FT_Set_Pixel_Sizes(systemFace, customWidthPixels, heightPixels);
  }

  // load ASCII character glyphs
  for (char32_t code = U' '; code <= U'~'; ++code) {
    readGlyphFromFont(renderer, code);
  }
  // load "unknown" glyph (or create it, if not available)
  if (readGlyphFromFont(renderer, UnknownGlyphCode()) == glyphs.end())
    generateUnknownGlyph(renderer, heightPixels);
  xHeight = getGlyph(renderer, U'x')->bearingTop;
  clearBuffer();
}

void Font::release() noexcept {
  clearBuffer();
  if (freetypeInstance) {
    if (baseFontFace)
      FT_Done_Face((FT_Face)baseFontFace);
    if (systemFontFace)
      FT_Done_Face((FT_Face)systemFontFace);
    if (--freetypeUsageCount <= 0)
      FT_Done_FreeType(freetypeInstance);

    glyphs.clear();
  }
}

void Font::allocBuffer(size_t minSize) {
  if (bufferSize < minSize) {
    bufferSize = ((minSize + 0xF) & ~(size_t)0xF); // 16-byte aligned
    buffer = buffer ? (uint32_t*)realloc(buffer, bufferSize) : (uint32_t*)malloc(bufferSize);
    if (buffer == nullptr)
      throw std::bad_alloc{};
  }
}

void Font::clearBuffer() noexcept {
  if (buffer) {
    free(buffer);
    buffer = nullptr;
    bufferSize = 0;
  }
}

// -----------------------------------------------------------------------------

Font::GlyphMap::iterator Font::readGlyphFromFont(Renderer& renderer, char32_t code) {
  FT_Face face = (FT_Face)baseFontFace;
  if (FT_Load_Char(face, (FT_ULong)code, FT_LOAD_RENDER) || face->glyph == nullptr) {
    // fallback on system font
    face = (FT_Face)systemFontFace;
    if (face == nullptr || FT_Load_Char(face, (FT_ULong)code, FT_LOAD_RENDER) || face->glyph == nullptr)
      return glyphs.end(); // glyph not found in any font -> exit here
  }

  const auto* faceGlyph = face->glyph;
  if (faceGlyph->bitmap.width > 0 && faceGlyph->bitmap.rows > 0) {
    allocBuffer(static_cast<size_t>(faceGlyph->bitmap.width) * static_cast<size_t>(faceGlyph->bitmap.rows) * (size_t)4u);
    uint32_t* destIt = buffer;
    unsigned char* srcIt = faceGlyph->bitmap.buffer;
    for (unsigned int row = 0; row < faceGlyph->bitmap.rows; ++row) {
      for (unsigned int px = 0; px < faceGlyph->bitmap.width; ++px, ++destIt, ++srcIt) {
        *destIt = (((uint32_t)*srcIt) << 24) | 0xFFFFFFu; // alpha(8) to RGBA(32)
      }
    }

    Texture2DParams params(faceGlyph->bitmap.width, faceGlyph->bitmap.rows,
                           DataFormat::rgba8_sRGB, 1, 1, 0, ResourceUsage::staticGpu, 1);
    return glyphs.emplace(code, std::make_shared<FontGlyph>(Texture2D(renderer, params, (const uint8_t**)&buffer),
                                                            faceGlyph->bitmap.width, faceGlyph->bitmap.rows,
                                                            faceGlyph->bitmap_left, faceGlyph->bitmap_top,
                                                            faceGlyph->advance.x)).first;
  }
  return glyphs.emplace(code, std::make_shared<FontGlyph>(Texture2D{}, faceGlyph->bitmap.width, faceGlyph->bitmap.rows,
                                                          faceGlyph->bitmap_left, faceGlyph->bitmap_top,
                                                          faceGlyph->advance.x)).first;
  // don't clear buffer here: if a bunch of characters are loaded, we don't want to realloc everytime
  // -> cleared initially at the end of the constructor + cleared using clearBuffer()
}

void Font::generateUnknownGlyph(Renderer& renderer, uint32_t heightPixels) {
  const uint32_t height = (heightPixels << 1) / 3;
  const uint32_t width = (height >> 1);
  size_t dataSize = (size_t)width * (size_t)height * (size_t)4u;

  allocBuffer(dataSize);
  memset(buffer, 0xFF, dataSize); // fill rectangle

  Texture2DParams params(width, height, DataFormat::rgba8_sRGB, 1, 1, 0, ResourceUsage::staticGpu, 1);
  glyphs.emplace(UnknownGlyphCode(), std::make_shared<FontGlyph>(Texture2D(renderer, params, (const uint8_t**)&buffer),
                                                                 width, height, 1, height, (width << 6)));
}
