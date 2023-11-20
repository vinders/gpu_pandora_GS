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
  /// @brief Optional type of icon to display in a control
  enum class ControlIconType : uint32_t {
    none = 0,
    checked,
    unchecked,
    home,
    settings,
    selector,
    profile,
    add,
    edit,
    remove,
    keyboard,
    controller,
    buttonDpad,
    buttonDpadY,
    buttonDpadUp,
    buttonDpadDown,
    buttonDpadLeft,
    buttonDpadRight,
    buttonStart,
    buttonSelect,
    buttonL1,
    buttonL2,
    buttonR1,
    buttonR2,
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
    inline uint32_t width() const noexcept { return width_; }   ///< Icon width in sprite-sheet
    inline uint32_t height() const noexcept { return height_; } ///< Icon height in sprite-sheet
    
  private:
    std::shared_ptr<video_api::Texture2D> texture_ = nullptr;
    uint32_t offsetX_ = 0;
    uint32_t offsetY_ = 0;
    uint32_t width_ = 0;
    uint32_t height_ = 0;
  };
  
  // ---
  
  /// @brief Menu image/icon loader
  class ImageLoader final {
  public:
#   ifdef _WINDOWS
    ImageLoader(std::shared_ptr<video_api::Renderer> renderer, const char* iconSpriteId, const char* iconSpriteAlphaId)
      : renderer(std::move(renderer)) { iconsSprite = loadImage(iconSpriteId, iconSpriteAlphaId); }
    ImageLoader(std::shared_ptr<video_api::Renderer> renderer, const wchar_t* iconSpriteId, const wchar_t* iconSpriteAlphaId)
      : renderer(std::move(renderer)) { iconsSprite = loadImage(iconSpriteId, iconSpriteAlphaId); }
#   else
    ImageLoader(std::shared_ptr<video_api::Renderer> renderer, const char* iconSpritePath)
      : renderer(std::move(renderer)) { iconsSprite = loadImage(iconSpritePath); }
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
    
    /// @brief Load icon to display in a control
    ControlIcon getIcon(ControlIconType type);
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

  private:
    std::shared_ptr<video_api::Renderer> renderer = nullptr;
    std::shared_ptr<video_api::Texture2D> iconsSprite = nullptr;
  };
}
