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
#pragma once

#include <cstdint>
#include <memory>
#include <unordered_map>
#include "display/video_api.h"

namespace display {
  /// @brief Font character/symbol container: texture + size/offset info
  struct FontGlyph final {
    FontGlyph(video_api::Texture2D texture, int32_t width, int32_t height,
              int32_t offsetLeft, int32_t bearingTop, uint32_t advance, uint32_t scaling = 1)
    : texture(std::move(texture)), width((float)width), height((float)height),
      offsetLeft((float)offsetLeft), bearingTop((float)bearingTop), advance((float)advance) {
      if (scaling != 1) {
        this->width /= (float)scaling;
        this->height /= (float)scaling;
        this->offsetLeft /= (float)scaling;
        this->bearingTop /= (float)scaling;
        this->advance /= (float)scaling;
      }
    }
    FontGlyph() = default;
    FontGlyph(FontGlyph&&) = default;
    FontGlyph& operator=(FontGlyph&&) = default;
    ~FontGlyph() noexcept = default;

    video_api::Texture2D texture; ///< Glyph texture -- may be empty for invisible glyphs (whitespaces...)
    float width = 0.f;      ///< width of glyph
    float height = 0.f;     ///< height of glyph
    float offsetLeft = 0.f; ///< offset from origin to left of glyph
    float bearingTop = 0.f; ///< offset from baseline to top of glyph
    float advance = 0.f;   ///< offset to advance to next glyph
  };

  // ---

  /// @brief Font texture generator
  class Font final {
  public:
    static constexpr inline char32_t UnknownGlyphCode() noexcept { return (char32_t)0xFFFD; } ///< Unknown symbol representation
    using GlyphMap = std::unordered_map<char32_t, std::unique_ptr<FontGlyph> >;

    /// @brief Load font face + preload ASCII characters
    Font(video_api::Renderer& renderer, const char* fontPath,
         uint32_t heightPixels, uint32_t customWidthPixels = 0, uint32_t scaling = 1);
    Font(const Font&) = delete;
    Font(Font&&) noexcept = delete;
    Font& operator=(const Font&) = delete;
    Font& operator=(Font&&) noexcept = delete;
    ~Font() noexcept { release(); }
    void release() noexcept; ///< Release all glyphs and font (all objects using glyphs must have been released before!)
    void clearBuffer() noexcept; ///< Clear buffer after getting all required glyphs

    inline const GlyphMap& storedGlyphs() const noexcept { return glyphs; } ///< Access currently stored glyphs
    inline uint32_t XHeight() const noexcept { return xHeight; } ///< X-Height of the font face

    inline const FontGlyph& getGlyph(video_api::Renderer& renderer, char32_t code) noexcept { ///< Get (or load) character glyph
      auto it = glyphs.find(code);
      if (it == glyphs.end()) {
        it = readGlyphFromFont(renderer, code);
        if (it == glyphs.end())
          it = glyphs.find(UnknownGlyphCode());
      }
      return *(it->second);
    }
      
  private:
    GlyphMap::iterator readGlyphFromFont(video_api::Renderer& renderer, char32_t code);
    void generateUnknownGlyph(video_api::Renderer& renderer, uint32_t heightPixels);
    void allocBuffer(size_t minSize);

  private:
    GlyphMap glyphs;
    uint32_t* buffer = nullptr;
    size_t bufferSize = 0;
    uint32_t scaling = 1;
    uint32_t xHeight = 0;
    void* baseFontFace = nullptr;   // main font used to retrieve glyphs
    void* systemFontFace = nullptr; // fallback for special glyphs (not found in base font)
  };
}
