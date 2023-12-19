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
    RendererContext(std::shared_ptr<video_api::Renderer> renderer, const char* fontDirectoryPath,
                    const char* logoId, const char* logoAlphaId,
                    const char* logo2xId, const char* logo2xAlphaId,
                    const char* iconSpriteId, const char* iconSpriteAlphaId,
                    const char* iconSprite2xId, const char* iconSprite2xAlphaId,
                    const char* tabSpriteId, const char* tabSpriteAlphaId,
                    const char* tabSprite2xId, const char* tabSprite2xAlphaId,
                    const char* ratioPreviewId, const char* radialGradientId,
                    uint32_t clientWidth, uint32_t clientHeight)
    : renderer_(std::move(renderer)),
      imageLoader_(renderer_, logoId, logoAlphaId, logo2xId, logo2xAlphaId,
                   iconSpriteId, iconSpriteAlphaId, iconSprite2xId, iconSprite2xAlphaId,
                   tabSpriteId, tabSpriteAlphaId, tabSprite2xId, tabSprite2xAlphaId, radialGradientId),
      fontDirectoryPath(fontDirectoryPath) {
      onSizeChange(clientWidth, clientHeight);
      initFonts();
      ratioPreview = this->imageLoader_.loadImage(ratioPreviewId, nullptr);
      if (ratioPreview == nullptr)
        ratioPreview = this->imageLoader_.generateSquareIcon(true).texture();
    }
    RendererContext(std::shared_ptr<video_api::Renderer> renderer, const char* fontDirectoryPath,
                    const wchar_t* logoId, const wchar_t* logoAlphaId,
                    const wchar_t* logo2xId, const wchar_t* logo2xAlphaId,
                    const wchar_t* iconSpriteId, const wchar_t* iconSpriteAlphaId,
                    const wchar_t* iconSprite2xId, const wchar_t* iconSprite2xAlphaId,
                    const wchar_t* tabSpriteId, const wchar_t* tabSpriteAlphaId,
                    const wchar_t* tabSprite2xId, const wchar_t* tabSprite2xAlphaId,
                    const wchar_t* ratioPreviewId, const wchar_t* radialGradientId,
                    uint32_t clientWidth, uint32_t clientHeight)
    : renderer_(std::move(renderer)),
      imageLoader_(renderer_, logoId, logoAlphaId, logo2xId, logo2xAlphaId,
                   iconSpriteId, iconSpriteAlphaId, iconSprite2xId, iconSprite2xAlphaId,
                   tabSpriteId, tabSpriteAlphaId, tabSprite2xId, tabSprite2xAlphaId, radialGradientId),
      fontDirectoryPath(fontDirectoryPath) {
      onSizeChange(clientWidth, clientHeight);
      initFonts();
      ratioPreview = this->imageLoader_.loadImage(ratioPreviewId, nullptr);
      if (ratioPreview == nullptr)
        ratioPreview = this->imageLoader_.generateSquareIcon(true).texture();
    }
#   else
    RendererContext(std::shared_ptr<video_api::Renderer> renderer, const char* fontDirectoryPath,
                    const char* logoId, const char* logo2xId,
                    const char* iconSpritePath, const char* iconSprite2xPath,
                    const char* tabSpritePath, const char* tabSprite2xPath,
                    const char* ratioPreviewPath, const char* radialGradientPath,
                    uint32_t clientWidth, uint32_t clientHeight)
    : renderer_(std::move(renderer)),
      imageLoader_(renderer_, logoId, logo2xId, iconSpritePath, iconSprite2xPath, tabSpritePath, tabSprite2xPath, radialGradientPath),
      fontDirectoryPath(fontDirectoryPath) {
      onSizeChange(clientWidth, clientHeight);
      initFonts();
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
    
    inline uint32_t clientWidth() const noexcept { return clientWidth_; }   ///< Window client width (pixels) -- scaled
    inline uint32_t clientHeight() const noexcept { return clientHeight_; } ///< Window client height (pixels) -- scaled
    inline uint32_t originalWidth() const noexcept { return originalWidth_; }   ///< Window client width (pixels) -- original
    inline uint32_t originalHeight() const noexcept { return originalHeight_; } ///< Window client height (pixels) -- original
    inline uint32_t scaling() const noexcept { return scaling_; }    ///< Window content scaling (factor for high-DPI)
    inline float pixelSizeX() const noexcept { return pixelSizeX_; } ///< Horizontal pixel size in shader coords
    inline float pixelSizeY() const noexcept { return pixelSizeY_; } ///< Vertical pixel size in shader coords
    
  private:
    void initFonts();

  private:
    std::shared_ptr<video_api::Renderer> renderer_ = nullptr;
    std::array<std::unique_ptr<display::Font>, (size_t)FontType::COUNT> fonts{};
    display::ImageLoader imageLoader_;
    std::shared_ptr<video_api::Texture2D> ratioPreview = nullptr;
    uint32_t clientWidth_ = 0;
    uint32_t clientHeight_ = 0;
    uint32_t originalWidth_ = 0;
    uint32_t originalHeight_ = 0;
    uint32_t scaling_ = 1;
    float pixelSizeX_ = 1.f;
    float pixelSizeY_ = 1.f;
    const char* fontDirectoryPath = "./";
  };
}
