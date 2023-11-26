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

using namespace video_api;
using namespace pandora::system;
using namespace display;

// -- control icons -- ---------------------------------------------------------

const char16_t* display::toDefaultLabel(ControlIconType type) noexcept {
  switch (type) {
    case ControlIconType::add: return u"+";
    case ControlIconType::edit: return u">";
    case ControlIconType::remove: return u"-";
    case ControlIconType::keyboard: return u"Keyboard";
    case ControlIconType::controller: return u"Controller";
    case ControlIconType::buttonDpad: return u"D-pad";
    case ControlIconType::buttonDpadUp: return u"^";
    case ControlIconType::buttonDpadDown: return u"v";
    case ControlIconType::buttonDpadLeft: return u"<";
    case ControlIconType::buttonDpadRight: return u">";
    case ControlIconType::buttonStart: return u"Start";
    case ControlIconType::buttonSelect: return u"Select";
    case ControlIconType::buttonL1: return u"L1";
    case ControlIconType::buttonL2: return u"L2";
    case ControlIconType::buttonR1: return u"R1";
    case ControlIconType::buttonR2: return u"R2";
    case ControlIconType::buttonTriangle: return u"Triangle";
    case ControlIconType::buttonCircle: return u"Circle";
    case ControlIconType::buttonSquare: return u"Square";
    case ControlIconType::buttonCross: return u"X";
    case ControlIconType::buttonL3: return u"L3";
    case ControlIconType::buttonR3: return u"R3";
    default: return nullptr;
  };
}

ControlIcon ImageLoader::getIcon(ControlIconType type) {
  if (iconsSprite != nullptr) {
    switch (type) {
      case ControlIconType::checked:   return ControlIcon(iconsSprite, 0, 0, 64,22);
      case ControlIconType::unchecked: return ControlIcon(iconsSprite, 0,24, 64,22);
      case ControlIconType::tabHome:      return ControlIcon(tabsSprite, 0,0, 48,48);
      case ControlIconType::tabSettings:  return ControlIcon(tabsSprite, 0,48, 48,48);
      case ControlIconType::tabSelector:  return ControlIcon(tabsSprite, 0,96, 48,48);
      case ControlIconType::tabProfile:   return ControlIcon(tabsSprite, 0,144, 48,48);
      case ControlIconType::add:    return ControlIcon(iconsSprite, 1,48, 19,19);
      case ControlIconType::edit:   return ControlIcon(iconsSprite, 1,69, 19,19);
      case ControlIconType::remove: return ControlIcon(iconsSprite, 1,90, 19,19);
      case ControlIconType::keyboard:   return ControlIcon(iconsSprite, 1,111, 22,22);
      case ControlIconType::controller: return ControlIcon(iconsSprite, 1,135, 22,22);
      case ControlIconType::buttonDpad:      return ControlIcon(iconsSprite, 26,86, 38,38);
      case ControlIconType::buttonDpadUp:    return ControlIcon(iconsSprite, 26,48, 14,17);
      case ControlIconType::buttonDpadDown:  return ControlIcon(iconsSprite, 26,67, 14,17);
      case ControlIconType::buttonDpadLeft:  return ControlIcon(iconsSprite, 27,60, 44,70);
      case ControlIconType::buttonDpadRight: return ControlIcon(iconsSprite, 48,60, 44,48);
      case ControlIconType::buttonStart:  return ControlIcon(iconsSprite, 0,167, 26,14);
      case ControlIconType::buttonSelect: return ControlIcon(iconsSprite, 0,187, 26,12);
      case ControlIconType::buttonL1: return ControlIcon(iconsSprite, 26,126, 38,19);
      case ControlIconType::buttonL2: return ControlIcon(iconsSprite, 26,145, 38,19);
      case ControlIconType::buttonR1: return ControlIcon(iconsSprite, 26,164, 38,19);
      case ControlIconType::buttonR2: return ControlIcon(iconsSprite, 26,183, 38,19);
      case ControlIconType::buttonTriangle: return ControlIcon(iconsSprite,  1,204, 26,26);
      case ControlIconType::buttonCircle:   return ControlIcon(iconsSprite, 28,204, 26,26);
      case ControlIconType::buttonSquare:   return ControlIcon(iconsSprite,  1,231, 26,26);
      case ControlIconType::buttonCross:    return ControlIcon(iconsSprite, 28,231, 26,26);
      case ControlIconType::buttonL3:       return ControlIcon(iconsSprite,  1,258, 26,26);
      case ControlIconType::buttonR3:       return ControlIcon(iconsSprite, 28,258, 26,26);
      default: break;
    }
  }
  return ControlIcon(nullptr, 0,0, 0,0);
}

#define BASE_ICON_SIZE 22

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