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
#include <functional>
#include <unordered_map>
#include <system/align.h>

#ifdef _WINDOWS
# define __RES_PATH_CHAR wchar_t
#else
# define __RES_PATH_CHAR char
#endif

namespace display {
  /// @brief Font-map character descriptor (used with font-map spritesheet)
  __align_prefix(16)
  struct CharDescriptor final {
    inline uint32_t id() const noexcept { return ((uint32_t*)data)[0]; } ///< Character code point (unicode)
    inline uint8_t  x() const noexcept { return data[8]; }               ///< Horizontal image location in font-map spritesheet
    inline uint32_t y() const noexcept { return ((uint32_t*)data)[1]; }  ///< Vertical image location in font-map spritesheet
    inline uint8_t width() const noexcept { return data[10]; }           ///< Character image width
    inline uint8_t height() const noexcept { return data[11]; }          ///< Character image height
    inline int8_t  offsetX() const noexcept { return (int8_t)data[12]; } ///< Left padding before character (may be negative)
    inline uint8_t offsetY() const noexcept { return data[13]; }         ///< Top padding before character
    inline uint8_t advanceX() const noexcept { return data[14]; }        ///< Next caret position after 'x'
    inline uint8_t advanceY() const noexcept { return data[15]; }        ///< Next line position after 'y' (== line_height - offsetY)
      
    inline void id(uint32_t charId) noexcept { ((uint32_t*)data)[0] = charId; } ///< Set character code point (unicode)
    inline void x(uint8_t x) noexcept { data[8] = x; }                      ///< Set horizontal image location
    inline void y(uint32_t y) noexcept { ((uint32_t*)data)[1] = y; }        ///< Set vertical image location
    inline void width(uint8_t w) noexcept { data[10] = w; }                 ///< Set character image width
    inline void height(uint8_t h) noexcept { data[11] = h; }                ///< Set character image height
    inline void offsetX(int8_t padX) noexcept { data[12] = (uint8_t)padX; } ///< Set left padding before character
    inline void offsetY(uint8_t padY) noexcept { data[13] = padY; }         ///< Set top padding before character
    inline void advanceX(uint8_t advX) noexcept { data[14] = advX; }        ///< Set next caret position after 'x'
    inline void advanceY(uint8_t advY) noexcept { data[15] = advY; }        ///< Set next line position after 'y'
    uint8_t data[16];
  } __align_suffix(16);

  // ---

  /// @brief Font-map character descriptors
  /// @remarks This should be combined with a spritesheet (texture) containing all the characters
  class FontMap final {
  public:
    FontMap() = default;
    FontMap(const FontMap&) = default;
    FontMap(FontMap&&) noexcept = default;
    FontMap& operator=(const FontMap&) = default;
    FontMap& operator=(FontMap&&) noexcept = default;
    ~FontMap() noexcept {}

    /// @brief Initialize font-map descriptors
    /// @param charDescriptors  Array of character location descriptors (must not be NULL)
    /// @param length           Length of array 'charDescriptors'
    FontMap(const CharDescriptor* charDescriptors, uint32_t length, uint32_t baseLineOffset)
      : _baseLineOffset(baseLineOffset) {
      for (const CharDescriptor* it = charDescriptors; length; ++it, --length)
        this->_descriptors[it->id()] = *it;
    }
    
    // -- builders --
    
    /// @brief Load font-map character descriptors from binary file
    /// @param descriptorPath  Resource path (file path on linux/mac, resource name on windows)
    /// @param moduleInstance  Library/DLL module instance (required on Windows)
    /// @throws runtime_error or bad_alloc on failure
    static FontMap loadDescriptorFile(const __RES_PATH_CHAR* descriptorPath, void* moduleInstance);
    /// @brief Load font-map spritesheet from image file
    /// @param imagePath       Resource path (file path on linux/mac, resource name on windows)
    /// @param moduleInstance  Library/DLL module instance (required on Windows)
    /// @param textureLoader   Lambda/function creating Texture2D from image data
    ///                        (ex: with D3D11, call CreateWICTextureFromMemoryEx in lambda)
    /// @throws runtime_error or bad_alloc on failure
    static void loadSpritesheet(const __RES_PATH_CHAR* imagePath, void* moduleInstance,
                                const std::function<void(const char*,size_t)>& textureLoader);
    
    // -- accessors --

    inline size_t charCount() const noexcept { return _descriptors.size(); }    ///< Number of character locations
    inline uint32_t baseLineOffset() const noexcept { return _baseLineOffset; } ///< Vertical offset of character base-line

    /// @brief Find character descriptor by ID/char-code (returns NULL if not found)
    inline const CharDescriptor* find(uint32_t charCode) const noexcept {
      auto it = this->_descriptors.find(charCode);
      return (it != this->_descriptors.end()) ? &(it->second) : nullptr;
    }

  private:
    std::unordered_map<uint32_t,CharDescriptor> _descriptors;
    uint32_t _baseLineOffset = 0;
  };
}
