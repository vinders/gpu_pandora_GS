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
#include <display/video_api.h>

namespace display {
  /// @brief Optional type of icon to display in a tab
  enum class TabIconType : uint32_t {
    none = 0,
    home,
    settings,
    selector,
    profile,
  };
  /// @brief Optional type of icon to display in a control
  enum class ControlIconType : uint32_t {
    none = 0,
    checked,
    unchecked,
    add,
    edit,
    remove,
    keyboard,
    controller,
    buttonDpad,
    buttonDpadUp,
    buttonDpadDown,
    buttonDpadLeft,
    buttonDpadRight,
    buttonStart,
    buttonSelect,
    buttonL1,
    buttonL2,
    buttonSmallL2,
    buttonR1,
    buttonR2,
    buttonSmallR2,
    buttonTriangle,
    buttonCircle,
    buttonSquare,
    buttonCross,
    buttonL3,
    buttonR3
  };
  const char16_t* toDefaultLabel(ControlIconType type) noexcept;

  /// @brief Icon to display in a control
  class ControlIcon final {
  public:
    ControlIcon(std::shared_ptr<video_api::Texture2D> texture,
                uint32_t offsetX, uint32_t offsetY, uint32_t width, uint32_t height)
      : texture_(std::move(texture)),
        offsetX_(offsetX), offsetY_(offsetY),
        width_(width), height_(height) {}
    ControlIcon(std::shared_ptr<video_api::Texture2D> texture,
                uint32_t offsetX, uint32_t offsetY, uint32_t width, uint32_t height, uint32_t scaling)
      : texture_(std::move(texture)),
        offsetX_(offsetX), offsetY_(offsetY),
        width_(width), height_(height), scaling_(scaling) {
      if (scaling > 1u) {
        offsetX_ *= scaling;
        offsetY_ *= scaling;
      }
    }
    ControlIcon() = default;
    ControlIcon(const ControlIcon&) = default;
    ControlIcon(ControlIcon&&) noexcept = default;
    ControlIcon& operator=(const ControlIcon&) = default;
    ControlIcon& operator=(ControlIcon&&) noexcept = default;
    ~ControlIcon() noexcept = default;
    
    inline const std::shared_ptr<video_api::Texture2D>& texture() const noexcept { return texture_; } ///< Sprite-sheet
    inline std::shared_ptr<video_api::Texture2D>& texture() noexcept { return texture_; } ///< Sprite-sheet
    inline uint32_t offsetX() const noexcept { return offsetX_; } ///< Icon offset-X in sprite-sheet
    inline uint32_t offsetY() const noexcept { return offsetY_; } ///< Icon offset-Y in sprite-sheet
    inline uint32_t contentWidth() const noexcept { return width_; }   ///< Icon width in page
    inline uint32_t contentHeight() const noexcept { return height_; } ///< Icon height in page
    inline uint32_t textureWidth() const noexcept { return width_*scaling_; }   ///< Icon width in sprite-sheet
    inline uint32_t textureHeight() const noexcept { return height_*scaling_; } ///< Icon height in sprite-sheet
    inline uint32_t scaling() const noexcept { return scaling_; } ///< Icon height in sprite-sheet
    
  private:
    std::shared_ptr<video_api::Texture2D> texture_ = nullptr;
    uint32_t offsetX_ = 0;
    uint32_t offsetY_ = 0;
    uint32_t width_ = 0;
    uint32_t height_ = 0;
    uint32_t scaling_ = 1;
  };
  
  // ---
  
  /// @brief Menu image/icon loader
  class ImageLoader final {
  public:
#   ifdef _WINDOWS
    ImageLoader(std::shared_ptr<video_api::Renderer> renderer,
                const char* logoId, const char* logoAlphaId,
                const char* logo2xId, const char* logo2xAlphaId,
                const char* iconSpriteId, const char* iconSpriteAlphaId,
                const char* iconSprite2xId, const char* iconSprite2xAlphaId,
                const char* tabSpriteId, const char* tabSpriteAlphaId,
                const char* tabSprite2xId, const char* tabSprite2xAlphaId,
                const char* radialGradientId)
      : renderer(std::move(renderer)), radialGradientId(radialGradientId) {
      iconsSprite = loadImage(iconSpriteId, iconSpriteAlphaId);
      iconsSprite2x = loadImage(iconSprite2xId, iconSprite2xAlphaId);
      tabsSprite = loadImage(tabSpriteId, tabSpriteAlphaId);
      tabsSprite2x = loadImage(tabSprite2xId, tabSprite2xAlphaId);
      logo = loadImage(logoId, logoAlphaId);
      logo2x = loadImage(logo2xId, logo2xAlphaId);
    }
    ImageLoader(std::shared_ptr<video_api::Renderer> renderer,
                const wchar_t* logoId, const wchar_t* logoAlphaId,
                const wchar_t* logo2xId, const wchar_t* logo2xAlphaId,
                const wchar_t* iconSpriteId, const wchar_t* iconSpriteAlphaId,
                const wchar_t* iconSprite2xId, const wchar_t* iconSprite2xAlphaId,
                const wchar_t* tabSpriteId, const wchar_t* tabSpriteAlphaId,
                const wchar_t* tabSprite2xId, const wchar_t* tabSprite2xAlphaId,
                const wchar_t* radialGradientId)
      : renderer(std::move(renderer)), radialGradientWideId(radialGradientId) {
      iconsSprite = loadImage(iconSpriteId, iconSpriteAlphaId);
      iconsSprite2x = loadImage(iconSprite2xId, iconSprite2xAlphaId);
      tabsSprite = loadImage(tabSpriteId, tabSpriteAlphaId);
      tabsSprite2x = loadImage(tabSprite2xId, tabSprite2xAlphaId);
      logo = loadImage(logoId, logoAlphaId);
      logo2x = loadImage(logo2xId, logo2xAlphaId);
    }
#   else
    ImageLoader(std::shared_ptr<video_api::Renderer> renderer,
                const char* logoPath, const char* logo2xPath,
                const char* iconSpritePath, const char* iconSprite2xPath,
                const char* tabSpritePath, const char* tabSprite2xPath, const char* radialGradientPath)
      : renderer(std::move(renderer)) {
      iconsSprite = loadImage(iconSpritePath);
      iconsSprite2x = loadImage(iconSprite2xPath);
      tabsSprite = loadImage(tabSpritePath);
      tabsSprite2x = loadImage(tabSprite2xPath);
      logo = loadImage(logoPath);
      logo2x = loadImage(logo2xPath);
    }
#   endif

    ImageLoader() = default;
    ImageLoader(const ImageLoader&) = default;
    ImageLoader(ImageLoader&&) noexcept = default;
    ImageLoader& operator=(const ImageLoader&) = default;
    ImageLoader& operator=(ImageLoader&&) noexcept = default;
    ~ImageLoader() noexcept { release(); }

    inline void release() noexcept {
      iconsSprite = nullptr;
      renderer = nullptr;
    }
    
    /// @brief Load logo texture
    ControlIcon getLogo(uint32_t themeIndex, uint32_t themeCount, uint32_t scaling);
    /// @brief Load icon to display in a tab
    ControlIcon getTabIcon(TabIconType type, uint32_t scaling);
    /// @brief Load icon to display in a control
    ControlIcon getIcon(ControlIconType type, uint32_t scaling);
    /// @brief Generate square icon for a control (e.g. to use as a placeholder, if no icon is available)
    ControlIcon generateSquareIcon(bool isFilled);
    
#   ifdef _WINDOWS
    /// @brief Load image file (package)
    std::shared_ptr<video_api::Texture2D> loadImage(const char* id, const char* alphaId);
    std::shared_ptr<video_api::Texture2D> loadImage(const wchar_t* id, const wchar_t* alphaId);
#   else
    /// @brief Load image file (path)
    std::shared_ptr<video_api::Texture2D> loadImage(const char* path);
#   endif

    std::shared_ptr<video_api::Texture2D> loadRadialGradient(const uint8_t rgbaColor[4]);

  private:
    std::shared_ptr<video_api::Renderer> renderer = nullptr;
    std::shared_ptr<video_api::Texture2D> iconsSprite = nullptr;
    std::shared_ptr<video_api::Texture2D> tabsSprite = nullptr;
    std::shared_ptr<video_api::Texture2D> iconsSprite2x = nullptr;
    std::shared_ptr<video_api::Texture2D> tabsSprite2x = nullptr;
    std::shared_ptr<video_api::Texture2D> logo = nullptr;
    std::shared_ptr<video_api::Texture2D> logo2x = nullptr;
#   ifdef _WINDOWS
    const char* radialGradientId = nullptr;
    const wchar_t* radialGradientWideId = nullptr;
#   else
    const char* radialGradientPath = nullptr;
#   endif
  };
}
