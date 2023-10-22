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

const char32_t* display::toDefaultLabel(ControlIconType type) noexcept {
  switch (type) {
    case ControlIconType::checked:    return U"v";
    case ControlIconType::unchecked:  return U"x";
    case ControlIconType::info:       return U"Info";
    case ControlIconType::options:    return U"Options";
    case ControlIconType::load:       return U"Load";
    case ControlIconType::save:       return U"Save";
    case ControlIconType::apply:      return U"Apply";
    case ControlIconType::cancel:     return U"Cancel";
    case ControlIconType::add:        return U"Add";
    case ControlIconType::edit:       return U"Edit";
    case ControlIconType::remove:     return U"Remove";
    case ControlIconType::importFile: return U"Import";
    case ControlIconType::exportFile: return U"Export";
    default: return U"";
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

/*//https://stackoverflow.com/questions/8657155/getting-bitmap-pixel-values-using-the-windows-getdibits-function
//HBITMAP bitmapHandle = (HBITMAP)LoadImage(0, L"C:/tmp/Foo.bmp", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
  BITMAP bitmap;
  if (bitmapHandle == nullptr || GetObject(bitmapHandle , sizeof(bitmap) , &bitmap) == 0)
    return nullptr;

  HDC dcBitmap = CreateCompatibleDC(NULL);
  auto region = SelectObject(dcBitmap, bitmapHandle);
  if (region == nullptr || region == HGDI_ERROR)
    return nullptr;

  BITMAPINFO bmpInfo{};
  bmpInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
  bmpInfo.bmiHeader.biWidth = bitmap.bmWidth;
  bmpInfo.bmiHeader.biHeight = -bitmap.bmHeight;
  bmpInfo.bmiHeader.biPlanes = 1;
  bmpInfo.bmiHeader.biBitCount = 24;
  bmpInfo.bmiHeader.biCompression = BI_RGB;

  COLORREF* pixel = new COLORREF [ bitmap.bmWidth * bitmap.bmHeight ];
  GetDIBits(dcBitmap, bitmapHandle, 0, bitmap.bmHeight, pixel, &bmpInfo, DIB_RGB_COLORS);
*/

/*//https://stackoverflow.com/questions/26233848/c-read-pixels-with-getdibits
HBITMAP GetScreenBmp( HDC hdc) {
    // Get screen dimensions
    int nScreenWidth = GetSystemMetrics(SM_CXSCREEN);
    int nScreenHeight = GetSystemMetrics(SM_CYSCREEN);

    // Create compatible DC, create a compatible bitmap and copy the screen using BitBlt()
    HDC hCaptureDC  = CreateCompatibleDC(hdc);
    HBITMAP hBitmap = CreateCompatibleBitmap(hdc, nScreenWidth, nScreenHeight);
    HGDIOBJ hOld = SelectObject(hCaptureDC, hBitmap); 
    BOOL bOK = BitBlt(hCaptureDC,0,0,nScreenWidth, nScreenHeight, hdc,0,0,SRCCOPY|CAPTUREBLT); 

    SelectObject(hCaptureDC, hOld); // always select the previously selected object once done
    DeleteDC(hCaptureDC);
    return hBitmap;
}
int main() {
    HDC hdc = GetDC(0);

    HBITMAP hBitmap = GetScreenBmp(hdc);

    BITMAPINFO MyBMInfo = {0};
    MyBMInfo.bmiHeader.biSize = sizeof(MyBMInfo.bmiHeader); 

    // Get the BITMAPINFO structure from the bitmap
    if(0 == GetDIBits(hdc, hBitmap, 0, 0, NULL, &MyBMInfo, DIB_RGB_COLORS)) {
        cout << "error" << endl;
    }

    // create the bitmap buffer
    BYTE* lpPixels = new BYTE[MyBMInfo.bmiHeader.biSizeImage];

    // Better do this here - the original bitmap might have BI_BITFILEDS, which makes it
    // necessary to read the color table - you might not want this.
    MyBMInfo.bmiHeader.biCompression = BI_RGB;  

    // get the actual bitmap buffer
    if(0 == GetDIBits(hdc, hBitmap, 0, MyBMInfo.bmiHeader.biHeight, (LPVOID)lpPixels, &MyBMInfo, DIB_RGB_COLORS)) {
        cout << "error2" << endl;
    }

    for(int i = 0; i < 100; i++) {
        cout << (int)lpPixels[i];
    }

    DeleteObject(hBitmap);
    ReleaseDC(NULL, hdc);
    delete[] lpPixels;
    return 0;
}
*/

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
            std::unique_ptr<uint8_t[]> output(new uint8_t[width * height * 4]); // RGBA texture data

            // read color pixels
            const uint32_t* srcIt = (const uint32_t*)pixels;
            for (uint32_t lines = height; lines; --lines) {
              uint32_t* destIt = ((uint32_t*)output.get()) + (intptr_t)(width*(lines - 1)); // bitmaps are stored from bottom to top -> reverse
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
                uint32_t* destIt = ((uint32_t*)output.get()) + (intptr_t)(width*(lines - 1));
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
    return bitmapToTexture(LoadBitmapA(hInstance, id), LoadBitmapA(hInstance, alphaId), *renderer);
  }
  return nullptr;
}
std::shared_ptr<video_api::Texture2D> ImageLoader::loadImage(const wchar_t* id, const wchar_t* alphaId) {
  if (renderer != nullptr) {
    auto& appInstance = WindowsApp::instance();
    HINSTANCE hInstance = appInstance.isInitialized() ? (HINSTANCE)appInstance.handle() : GetModuleHandle(NULL);
    //return bitmapToTexture(hInstance, FindResourceW(hInstance, id, (LPCWSTR)RT_BITMAP), *renderer);
    return bitmapToTexture(LoadBitmapW(hInstance, id), LoadBitmapW(hInstance, alphaId), *renderer);
  }
  return nullptr;
}
#else
std::shared_ptr<video_api::Texture2D> ImageLoader::loadImage(const char* path) {
  
}
#endif