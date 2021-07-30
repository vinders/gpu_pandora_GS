/*******************************************************************************
Pandora GS - PSEmu-compatible GPU driver
Copyright (C) 2021  Romain Vinders

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation, version 2
of the License.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details (LICENSE file).
*******************************************************************************/
// this implementation is included in config.cpp -> single object file (less overhead + smaller lib size)
#include <cstring>
#include <stdexcept>
#include <io/encoder.h>
#include "config/types.h"

using namespace config;


// -- types - string containers --

CharCodeArray::CharCodeArray(const char* utf8, size_t length) {
  if (length) {
    if (utf8[length - 1] == '\0' && --length == 0)
      return;

    _array = (uint32_t*)malloc(length * sizeof(*_array));
    if (_array == nullptr)
      throw std::bad_alloc();

    size_t codeSize;
    for (uint32_t* it = _array; length; ++it, ++_length) {
      *it = pandora::io::Encoder::Utf8::decode(utf8, length, codeSize);
      utf8 += (intptr_t)codeSize;
      length -= codeSize;
    }
    if (_length < length) {
      uint32_t* resized = (uint32_t*)realloc(_array, _length * sizeof(*_array));
      if (resized)
        _array = resized;
    }
  }
}

CharCodeArray::CharCodeArray(const char16_t* utf16, size_t length) {
  if (length) {
    if (utf16[length - 1] == 0 && --length == 0)
      return;

    _array = (uint32_t*)malloc(length * sizeof(*_array));
    if (_array == nullptr)
      throw std::bad_alloc();

    size_t codeSize;
    for (uint32_t* it = _array; length; ++it, ++_length) {
      *it = pandora::io::Encoder::Utf16::decode(utf16, length, codeSize);
      utf16 += (intptr_t)codeSize;
      length -= codeSize;
    }
    if (_length < length) {
      uint32_t* resized = (uint32_t*)realloc(_array, _length * sizeof(*_array));
      if (resized)
        _array = resized;
    }
  }
}

// ---

CharCodeArray::CharCodeArray(const CharCodeArray& rhs) {
  if (rhs._array != nullptr) {
    _array = (uint32_t*)malloc(rhs._length*sizeof(*_array));
    if (_array == nullptr)
      throw std::bad_alloc();
    
    memcpy(_array, rhs._array, rhs._length*sizeof(*_array));
    _length = rhs._length;
  }
}

CharCodeArray& CharCodeArray::operator=(const CharCodeArray& rhs) {
  if (_array != nullptr) {
    free(_array);
    _array = nullptr;
    _length = 0;
  }
  if (rhs._array != nullptr) {
    _array = (uint32_t*)malloc(rhs._length * sizeof(*_array));
    if (_array == nullptr)
      throw std::bad_alloc();

    memcpy(_array, rhs._array, rhs._length*sizeof(*_array));
    _length = rhs._length;
  }
  return *this;
}

CharCodeArray::~CharCodeArray() noexcept {
  if (_array != nullptr)
    free(_array);
}
