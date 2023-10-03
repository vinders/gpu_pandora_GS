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
# include <WICTextureLoader.h>
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

  if (isFilled) { // filled -> white square
    memset(imageData.get(), 0xFF, BASE_ICON_SIZE*BASE_ICON_SIZE*4);
  }
  else { // unchecked -> white border
    constexpr const intptr_t lineSize = static_cast<intptr_t>(BASE_ICON_SIZE*4);
    uint8_t* lastLine = imageData.get() + static_cast<intptr_t>((BASE_ICON_SIZE-1)*BASE_ICON_SIZE*4);

    memset(imageData.get(), 0xFF, (size_t)lineSize); // top line
    for (uint8_t* line = imageData.get() + lineSize; line < lastLine;) {
      *(uint32_t*)line = 0xFFFFFFFFu;
      line += (intptr_t)4;
      memset(line, 0, (BASE_ICON_SIZE-2)*4);
      line += lineSize - (intptr_t)8;
      *(uint32_t*)line = 0xFFFFFFFFu;
      line += (intptr_t)4;
    }
    memset(lastLine, 0xFF, (size_t)lineSize); // bottom line
  }

  const uint8_t* initData = imageData.get();
  auto texture = std::make_shared<Texture2D>(*renderer, Texture2DParams(BASE_ICON_SIZE, BASE_ICON_SIZE, DataFormat::rgba8_sRGB,
                                                                        1u, 1u, 0, ResourceUsage::staticGpu, 1u), &initData);
  return ControlIcon(std::move(texture), 0,0, BASE_ICON_SIZE,BASE_ICON_SIZE);
}


// -- load image -- ------------------------------------------------------------

#ifdef _WINDOWS
static std::shared_ptr<Texture2D> bitmapToTexture(HINSTANCE hInstance, HRSRC imageResHandle, Renderer& renderer) {
  if (imageResHandle == nullptr)
    return nullptr;
  auto imageDataHandle = LoadResource(hInstance, imageResHandle);
  if (imageDataHandle == nullptr)
    return nullptr;
  std::shared_ptr<video_api::Texture2D> texture = nullptr;

  //https://learn.microsoft.com/en-us/windows/win32/wic/-wic-bitmapsources-howto-loadfromresource
  //https://learn.microsoft.com/en-us/windows/win32/direct2d/how-to-load-a-bitmap-from-a-resource
  auto imageFile = LockResource(imageDataHandle);
  if (imageFile != nullptr) {
    auto imageFileSize = SizeofResource(hInstance, imageResHandle);
    if (imageFileSize) {
      try {
        ID3D11Resource* imageRes = nullptr;
        ID3D11ShaderResourceView* resourceView = nullptr;
        if (SUCCEEDED(DirectX::CreateWICTextureFromMemoryEx((ID3D11Device*)renderer.device(),
                                                            (const uint8_t*)imageFile, (size_t)imageFileSize, 0, D3D11_USAGE_IMMUTABLE,
                                                            D3D11_BIND_SHADER_RESOURCE, 0, 0, DirectX::WIC_LOADER_FLAGS::WIC_LOADER_FORCE_SRGB,
                                                            &imageRes, &resourceView))) {
          D3D11_TEXTURE2D_DESC descriptor{};
          ((ID3D11Texture2D*)imageRes)->GetDesc(&descriptor);
          texture = std::make_shared<Texture2D>((TextureHandle)imageRes, (TextureView)resourceView, descriptor.Width*4,
                                                descriptor.Height, (uint8_t)1, ResourceUsage::staticGpu);
        }
      }
      catch (...) { texture = nullptr; }
    }
    UnlockResource(imageDataHandle);
  }
  FreeResource(imageDataHandle);
  return texture;
}

std::shared_ptr<video_api::Texture2D> ImageLoader::loadImage(const char* id) {
  if (renderer != nullptr) {
    auto& appInstance = WindowsApp::instance();
    HINSTANCE hInstance = appInstance.isInitialized() ? (HINSTANCE)appInstance.handle() : GetModuleHandle(NULL);
    return bitmapToTexture(hInstance, FindResourceA(hInstance, id, (LPCSTR)RT_BITMAP), *renderer);
  }
  return nullptr;
}
std::shared_ptr<video_api::Texture2D> ImageLoader::loadImage(const wchar_t* id) {
  if (renderer != nullptr) {
    auto& appInstance = WindowsApp::instance();
    HINSTANCE hInstance = appInstance.isInitialized() ? (HINSTANCE)appInstance.handle() : GetModuleHandle(NULL);
    return bitmapToTexture(hInstance, FindResourceW(hInstance, id, (LPCWSTR)RT_BITMAP), *renderer);
  }
  return nullptr;
}
#else
std::shared_ptr<video_api::Texture2D> ImageLoader::loadImage(const char* path) {
  
}
#endif