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
#include <memory>
#ifdef _WINDOWS
# include <system/api/windows_app.h>
# include <system/api/windows_api.h>
#endif
#include "display/image_loader.h"

#define CHECKBOX_ICON_SIZE  64
#define BASE_ICON_SIZE  22

using namespace video_api;
using namespace pandora::system;
using namespace display;

// -- control icons -- ---------------------------------------------------------

const char16_t* display::toDefaultLabel(ControlIconType type) noexcept {
  switch (type) {
    case ControlIconType::info:       return u"Info";
    case ControlIconType::options:    return u"Options";
    case ControlIconType::load:       return u"Load";
    case ControlIconType::save:       return u"Save";
    case ControlIconType::apply:      return u"Apply";
    case ControlIconType::cancel:     return u"Cancel";
    case ControlIconType::add:        return u"Add";
    case ControlIconType::edit:       return u"Edit";
    case ControlIconType::remove:     return u"Remove";
    case ControlIconType::keyboard:   return u"Keyboard";
    case ControlIconType::controller: return u"Controller";
    default: return nullptr;
  };
}

ControlIcon ImageLoader::getIcon(ControlIconType type) {
  if (iconsSprite != nullptr) {
    switch (type) {
      case ControlIconType::none: break;
      case ControlIconType::checked:   return ControlIcon(iconsSprite, 0,0, CHECKBOX_ICON_SIZE,BASE_ICON_SIZE);
      case ControlIconType::unchecked: return ControlIcon(iconsSprite, CHECKBOX_ICON_SIZE,0, CHECKBOX_ICON_SIZE,BASE_ICON_SIZE);
      default: return ControlIcon(iconsSprite, 2*CHECKBOX_ICON_SIZE + BASE_ICON_SIZE*((uint32_t)type - 3),0, BASE_ICON_SIZE,BASE_ICON_SIZE);
    }
  }
  return ControlIcon(nullptr, 0,0, 0,0);
}

ControlIcon ImageLoader::generateSquareIcon(bool isFilled) {
  assert(renderer != nullptr);
  std::unique_ptr<uint8_t[]> imageData(new uint8_t[BASE_ICON_SIZE*BASE_ICON_SIZE*4]);

  if (isFilled) { // filled -> square
    memset(imageData.get(), 0x80, BASE_ICON_SIZE*BASE_ICON_SIZE*4);
  }
  else { // unchecked -> border
    constexpr const intptr_t lineSize = static_cast<intptr_t>(BASE_ICON_SIZE*4);
    uint8_t* lastLine = imageData.get() + static_cast<intptr_t>((BASE_ICON_SIZE-1)*BASE_ICON_SIZE*4);

    memset(imageData.get(), 0x80, (size_t)lineSize); // top line
    for (uint8_t* line = imageData.get() + lineSize; line < lastLine;) {
      *(uint32_t*)line = 0x80808080u;
      line += (intptr_t)4;
      memset(line, 0, (BASE_ICON_SIZE-2)*4);
      line += lineSize - (intptr_t)8;
      *(uint32_t*)line = 0x80808080u;
      line += (intptr_t)4;
    }
    memset(lastLine, 0x80, (size_t)lineSize); // bottom line
  }

  const uint8_t* initData = imageData.get();
  auto texture = std::make_shared<Texture2D>(*renderer, Texture2DParams(BASE_ICON_SIZE, BASE_ICON_SIZE, DataFormat::rgba8_sRGB,
                                                                        1u, 1u, 0, ResourceUsage::staticGpu, 1u), &initData);
  return ControlIcon(std::move(texture), 0,0, BASE_ICON_SIZE,BASE_ICON_SIZE);
}


// -- load image -- ------------------------------------------------------------

#ifdef _WINDOWS
static std::shared_ptr<Texture2D> bitmapToTexture(HBITMAP bitmapHandle, HBITMAP alphaHandle, Renderer& renderer) {
  std::shared_ptr<video_api::Texture2D> texture = nullptr;

  if (bitmapHandle != nullptr) {
    HDC hdc = GetDC(nullptr);
    if (hdc != nullptr) {
      BITMAPINFO bitmapInfo{};
      bitmapInfo.bmiHeader.biSize = sizeof(bitmapInfo.bmiHeader); 
      if (GetDIBits(hdc, bitmapHandle, 0, 0, nullptr, &bitmapInfo, DIB_RGB_COLORS) != 0) { // get bitmap size
        try {
          BYTE* pixels = new BYTE[bitmapInfo.bmiHeader.biSizeImage]; // create bitmap buffer
          bitmapInfo.bmiHeader.biCompression = BI_RGB;

          if (GetDIBits(hdc, bitmapHandle, 0, bitmapInfo.bmiHeader.biHeight, (LPVOID)pixels, &bitmapInfo, DIB_RGB_COLORS) != 0 && pixels) {
            const uint32_t width = bitmapInfo.bmiHeader.biWidth;
            const uint32_t height = bitmapInfo.bmiHeader.biHeight;
            Texture2DParams textureParams(width, height, DataFormat::rgba8_sRGB, 1u, 1u, 0, ResourceUsage::staticGpu, 1u);
            std::unique_ptr<uint8_t[]> output(new uint8_t[(size_t)width * (size_t)height * (size_t)4]); // RGBA texture data

            // read color pixels
            const uint32_t* srcIt = (const uint32_t*)pixels;
            for (uint32_t lines = height; lines; --lines) {
              uint32_t* destIt = ((uint32_t*)output.get()) + ((intptr_t)width*((intptr_t)lines - 1)); // bitmaps are stored from bottom to top -> reverse
              for (uint32_t rows = width; rows; --rows, ++srcIt, ++destIt) {
                uint32_t bgr = *srcIt;
                *destIt = ((bgr >> 16) & 0xFFu) | (bgr & 0xFF00u) | ((bgr << 16) & 0xFF0000u) | 0xFF000000u;
              }
            }
            // if available, read alpha pixels
            if (alphaHandle != nullptr && GetDIBits(hdc, alphaHandle, 0, bitmapInfo.bmiHeader.biHeight,
                                                    (LPVOID)pixels, &bitmapInfo, DIB_RGB_COLORS) != 0 && pixels) {
              const uint32_t* srcAlphaIt = (const uint32_t*)pixels;
              for (uint32_t lines = height; lines; --lines) {
                uint32_t* destIt = ((uint32_t*)output.get()) + ((intptr_t)width*((intptr_t)lines - 1));
                for (uint32_t rows = width; rows; --rows, ++srcAlphaIt, ++destIt)
                  *destIt = (*destIt & 0xFFFFFFu) | (*srcAlphaIt << 24);
              }
            }
            const uint8_t* initData = (const uint8_t*)output.get();
            texture = std::make_shared<Texture2D>(renderer, textureParams, &initData);
          }
        }
        catch (...) { texture = nullptr; }
      }
      ReleaseDC(nullptr, hdc);
    }
    DeleteObject(bitmapHandle);
  }
  if (alphaHandle)
    DeleteObject(alphaHandle);
  return texture;
}

std::shared_ptr<video_api::Texture2D> ImageLoader::loadImage(const char* id, const char* alphaId) {
  if (renderer != nullptr) {
    auto& appInstance = WindowsApp::instance();
    HINSTANCE hInstance = appInstance.isInitialized() ? (HINSTANCE)appInstance.handle() : GetModuleHandle(NULL);
    return bitmapToTexture(LoadBitmapA(hInstance, id), alphaId ? LoadBitmapA(hInstance, alphaId) : nullptr, *renderer);
  }
  return nullptr;
}
std::shared_ptr<video_api::Texture2D> ImageLoader::loadImage(const wchar_t* id, const wchar_t* alphaId) {
  if (renderer != nullptr) {
    auto& appInstance = WindowsApp::instance();
    HINSTANCE hInstance = appInstance.isInitialized() ? (HINSTANCE)appInstance.handle() : GetModuleHandle(NULL);
    //return bitmapToTexture(hInstance, FindResourceW(hInstance, id, (LPCWSTR)RT_BITMAP), *renderer);
    return bitmapToTexture(LoadBitmapW(hInstance, id), alphaId ? LoadBitmapW(hInstance, alphaId) : nullptr, *renderer);
  }
  return nullptr;
}

#else
std::shared_ptr<video_api::Texture2D> ImageLoader::loadImage(const char* path) {
  //...
  return nullptr;
}
#endif