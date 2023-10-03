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
              int32_t offsetLeft, int32_t bearingTop, uint32_t advance)
    : texture(std::move(texture)), width(width), height(height),
      offsetLeft(offsetLeft), bearingTop(bearingTop), advance(advance) {}
    FontGlyph() = default;
    FontGlyph(const FontGlyph&) = default;
    FontGlyph(FontGlyph&&) = default;
    FontGlyph& operator=(const FontGlyph&) = default;
    FontGlyph& operator=(FontGlyph&&) = default;
    ~FontGlyph() noexcept = default;

    video_api::Texture2D texture; ///< Glyph texture -- may be empty for invisible glyphs (whitespaces...)
    int32_t width = 0;      ///< width of glyph
    int32_t height = 0;     ///< height of glyph
    int32_t offsetLeft = 0; ///< offset from origin to left of glyph
    int32_t bearingTop = 0; ///< offset from baseline to top of glyph
    uint32_t advance = 0;   ///< offset to advance to next glyph
  };

  // ---

  /// @brief Font texture generator
  class Font final {
  public:
    static constexpr inline char32_t UnknownGlyphCode() noexcept { return (char32_t)0xFFFD; } ///< Unknown symbol representation
    using GlyphMap = std::unordered_map<char32_t, std::shared_ptr<FontGlyph> >;

    /// @brief Load font face + preload ASCII characters
    Font(video_api::Renderer& renderer, const char* fontPath,
         uint32_t heightPixels, uint32_t customWidthPixels = 0);
    Font(const Font&) = delete;
    Font(Font&&) noexcept = delete;
    Font& operator=(const Font&) = delete;
    Font& operator=(Font&&) noexcept = delete;
    ~Font() noexcept { release(); }
    void release() noexcept;
    void clearBuffer() noexcept; ///< Clear buffer after getting all required glyphs

    inline const GlyphMap& storedGlyphs() const noexcept { return glyphs; } ///< Access currently stored glyphs
    inline GlyphMap& storedGlyphs() noexcept { return glyphs; } ///< Access currently stored glyphs
    inline uint32_t XHeight() const noexcept { return xHeight; } ///< X-Height of the font face

    inline const std::shared_ptr<FontGlyph>& getGlyph(video_api::Renderer& renderer, char32_t code) noexcept { ///< Get (or load) character glyph
      auto it = glyphs.find(code);
      if (it == glyphs.end()) {
        it = readGlyphFromFont(renderer, code);
        if (it == glyphs.end())
          it = glyphs.find(UnknownGlyphCode());
      }
      return it->second;
    }
      
  private:
    GlyphMap::iterator readGlyphFromFont(video_api::Renderer& renderer, char32_t code);
    void generateUnknownGlyph(video_api::Renderer& renderer, uint32_t heightPixels);
    void allocBuffer(size_t minSize);

  private:
    GlyphMap glyphs;
    uint32_t* buffer = nullptr;
    size_t bufferSize = 0;
    uint32_t xHeight = 0;
    void* baseFontFace = nullptr;   // main font used to retrieve glyphs
    void* systemFontFace = nullptr; // fallback for special glyphs (not found in base font)
  };
}
