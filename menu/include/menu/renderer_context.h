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
#include <cstddef>
#include <memory>
#include <array>
#include <display/video_api.h>
#include <display/image_loader.h>
#include <display/font.h>

namespace menu {
  enum class FontType : uint32_t { ///< Menu font style
    titles = 0, ///< Title font
    labels,     ///< Label / content font
    inputText,  ///< Control inner text / smaller content font
    COUNT
  };
  
  /// @brief Menu rendering context
  class RendererContext final {
  public:
    /// @brief Initialize menu rendering context
    /// @param fontDirectoryPath  Absolute or relative directory to directory containing fonts
    ///                           (if not empty, must be finished with '/' or '\\')
#   ifdef _WINDOWS
    RendererContext(std::shared_ptr<video_api::Renderer> renderer,
                    const char* fontDirectoryPath, const char* iconSpriteId, const char* iconSpriteAlphaId,
                    const char* ratioPreviewId, uint32_t clientWidth, uint32_t clientHeight)
    : renderer_(std::move(renderer)), imageLoader_(renderer_, iconSpriteId, iconSpriteAlphaId) {
      onSizeChange(clientWidth, clientHeight);
      initFonts(fontDirectoryPath);
      ratioPreview = this->imageLoader_.loadImage(ratioPreviewId, nullptr);
      if (ratioPreview == nullptr)
        ratioPreview = this->imageLoader_.generateSquareIcon(true).texture();
    }
    RendererContext(std::shared_ptr<video_api::Renderer> renderer,
                    const char* fontDirectoryPath, const wchar_t* iconSpriteId, const wchar_t* iconSpriteAlphaId,
                    const wchar_t* ratioPreviewId, uint32_t clientWidth, uint32_t clientHeight)
    : renderer_(std::move(renderer)), imageLoader_(renderer_, iconSpriteId, iconSpriteAlphaId) {
      onSizeChange(clientWidth, clientHeight);
      initFonts(fontDirectoryPath);
      ratioPreview = this->imageLoader_.loadImage(ratioPreviewId, nullptr);
      if (ratioPreview == nullptr)
        ratioPreview = this->imageLoader_.generateSquareIcon(true).texture();
    }
#   else
    RendererContext(std::shared_ptr<video_api::Renderer> renderer,
                    const char* fontDirectoryPath, const char* iconSpritePath,
                    const char* ratioPreviewPath, uint32_t clientWidth, uint32_t clientHeight)
    : renderer_(std::move(renderer)), imageLoader_(renderer_, iconSpritePath) {
      onSizeChange(clientWidth, clientHeight);
      initFonts(fontDirectoryPath);
      ratioPreview = this->imageLoader_.loadImage(ratioPreviewPath);
      if (ratioPreview == nullptr)
        ratioPreview = this->imageLoader_.generateSquareIcon(true).texture();
    }
#   endif

    RendererContext(const RendererContext&) = delete;
    RendererContext(RendererContext&&) noexcept = default;
    RendererContext& operator=(const RendererContext&) = delete;
    RendererContext& operator=(RendererContext&&) noexcept = default;
    inline ~RendererContext() noexcept { release(); }
    
    void release() noexcept; ///< Release resources
    
    /// @brief Report window size change (to adapt UI pixel calculations)
    /// @returns True if size differs from current size
    bool onSizeChange(uint32_t clientWidth, uint32_t clientHeight) noexcept;
    
    // -- accessors --

    inline video_api::Renderer& renderer() noexcept { return *renderer_; }       ///< Video rendered used for menu
    inline display::ImageLoader& imageLoader() noexcept { return imageLoader_; } ///< Menu image/sprite loader
    inline const std::shared_ptr<video_api::Texture2D>& ratioPreviewImage() const noexcept { return ratioPreview; }
    
    /// @brief Get font glyph reader (by font type)
    inline const display::Font& getFont(FontType fontType) const noexcept { return *fonts[(size_t)fontType]; }
    inline display::Font& getFont(FontType fontType) noexcept { return *fonts[(size_t)fontType]; }
    
    inline uint32_t clientWidth() const noexcept { return clientWidth_; }   ///< Window client width (pixels)
    inline uint32_t clientHeight() const noexcept { return clientHeight_; } ///< Window client height (pixels)
    inline float pixelSizeX() const noexcept { return pixelSizeX_; } ///< Horizontal pixel size in shader coords
    inline float pixelSizeY() const noexcept { return pixelSizeY_; } ///< Vertical pixel size in shader coords
    
  private:
    void initFonts(const char* fontDirectoryPath);

  private:
    std::shared_ptr<video_api::Renderer> renderer_ = nullptr;
    std::array<std::unique_ptr<display::Font>, (size_t)FontType::COUNT> fonts{};
    display::ImageLoader imageLoader_;
    std::shared_ptr<video_api::Texture2D> ratioPreview = nullptr;
    uint32_t clientWidth_ = 0;
    uint32_t clientHeight_ = 0;
    float pixelSizeX_ = 1.f;
    float pixelSizeY_ = 1.f;
  };
}
