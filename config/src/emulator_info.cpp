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
#include <cstdint>
#include <cstring>
#include "config/emulator_info.h"

#if defined(_WINDOWS)
# include <system/api/windows_api.h>
# define __MAX_PATH_LENGTH  MAX_PATH

#elif !defined(__APPLE__) && (defined(PLATFORM_POSIX) || defined(__linux__) || defined(__linux))
# include <unistd.h>
# include <limits.h>
# define __MAX_PATH_LENGTH  (PATH_MAX + 1)

#else
# include <cstdlib>
# include <unistd.h>
# include <libproc.h>
# define __MAX_PATH_LENGTH  PROC_PIDPATHINFO_MAXSIZE
#endif


__PATH_CHAR* config::getProcessName(__PATH_CHAR* buffer, size_t bufferLength, size_t& outUsedBufferLength) noexcept {
# if defined(_WINDOWS)
    DWORD length = GetModuleFileNameW(nullptr, buffer, bufferLength);
    if (length <= 0)
      return nullptr;
    
    wchar_t* fileName = &buffer[length - 1];
    while (fileName > buffer && *fileName != L'\\' && *fileName != L'/')
      --fileName;
    if (*fileName == L'\\' || *fileName == L'/')
      ++fileName;
    outUsedBufferLength = (size_t)length;
    return fileName;
    
# else
#   if !defined(__APPLE__) && (defined(PLATFORM_POSIX) || defined(__linux__) || defined(__linux))
      ssize_t length = readlink("/proc/self/exe", buffer, bufferLength - 1);
      if (length <= 0)
        return nullptr;
      buffer[length] = '\0';
      
      char* fileName = &buffer[length - 1];
      while (fileName > buffer && *fileName != '/')
        --fileName;
      if (*fileName == '/')
        ++fileName;
      outUsedBufferLength = (size_t)length;
      return fileName;
      
#   else
      pid_t pid = getpid();
      proc_name(pid, buffer, bufferLength);
      outUsedBufferLength = strlen(buffer);
      return &buffer[0];
#   endif
# endif
}

// ---

config::EmulatorType config::getEmulatorType() noexcept {
  __PATH_CHAR processFilePath[__MAX_PATH_LENGTH];
  size_t filePathLength = 0;
  __PATH_CHAR* fileName = config::getProcessName(processFilePath, __MAX_PATH_LENGTH, filePathLength);
  
  if (fileName != nullptr) {
    switch (*fileName) {
      case (__PATH_CHAR)'e':
      case (__PATH_CHAR)'E':
        return EmulatorType::epsxe;
      case (__PATH_CHAR)'p':
      case (__PATH_CHAR)'P':
        if (filePathLength + 7 < __MAX_PATH_LENGTH && memcmp(fileName, __PATH_STR("psxtest"), 7*sizeof(__PATH_CHAR)) == 0)
          return EmulatorType::pluginTest;
        else
          return EmulatorType::pcsxr;
      case (__PATH_CHAR)'z':
      case (__PATH_CHAR)'Z':
        return EmulatorType::zinc;
      default: break;
    }
  }
  return EmulatorType::unknown;
}
