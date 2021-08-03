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
#include <stdexcept>
#include <system/api/windows_api.h>
#include "display/font_map.h"

using namespace display;


FontMap FontMap::loadDescriptorFile(const __RES_PATH_CHAR* descriptorPath, void* moduleInstance) {
# ifdef _WINDOWS
    HRSRC fileInfo = FindResource((HMODULE)moduleInstance, descriptorPath, RT_RCDATA);
    if (fileInfo == nullptr)
      throw std::runtime_error("FontMap: descriptor file not found");
    HGLOBAL fileRes = LoadResource((HMODULE)moduleInstance, fileInfo);
    if (fileRes == nullptr)
      throw std::runtime_error("FontMap: descriptor load failed");
    const uint32_t* data = (const uint32_t*)LockResource(fileRes);
    if (data == nullptr)
      throw std::runtime_error("FontMap: descriptor not readable");

    uint32_t baseLine = *data;
    ++data;
    uint32_t arrayLength = *data;
    ++data;
    return FontMap((const CharDescriptor*)data, arrayLength, baseLine);
    
# else
    throw std::runtime_error("FontMap: not implemented");
    //...
# endif
}

void FontMap::loadSpritesheet(const __RES_PATH_CHAR* imagePath, void* moduleInstance,
                              const std::function<void(const char*,size_t)>& textureLoader) {
# ifdef _WINDOWS
    HRSRC fileInfo = FindResource((HMODULE)moduleInstance, imagePath, RT_RCDATA);
    if (fileInfo == nullptr)
      throw std::runtime_error("FontMap: image file not found");
    DWORD byteSize = SizeofResource((HMODULE)moduleInstance, fileInfo);
    HGLOBAL fileRes = LoadResource((HMODULE)moduleInstance, fileInfo);
    if (fileRes == nullptr || byteSize == 0)
      throw std::runtime_error("FontMap: image load failed");
    const char* data = (const char*)LockResource(fileRes);
    if (data == nullptr)
      throw std::runtime_error("FontMap: image not readable");

    return textureLoader(data, byteSize);
    
# else
    //... read files from ./resources/
    return false;
# endif
}
