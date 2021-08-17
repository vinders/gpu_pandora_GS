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
#include <cstdio>
#include <cstring>
#include <io/file_system_locations.h>
#include "config/file_path_utils.h"

#if defined(_WINDOWS)
# include <system/api/windows_api.h>
# include <system/api/windows_app.h>
# define __MAX_PATH_LENGTH  MAX_PATH + 1
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

using namespace config;


// -- file / path utilities -- -------------------------------------------------

pandora::io::FileHandle config::openFile(const __UNICODE_CHAR* path, const __UNICODE_CHAR* mode) noexcept {
# ifdef _WINDOWS
    FILE* fileAccess = nullptr;
    if (_wfopen_s(&fileAccess, path, mode) == 0)
      return pandora::io::FileHandle(fileAccess);
    errno = 0;
    return pandora::io::FileHandle{};
# else
    return pandora::io::FileHandle(fopen(path, mode));
# endif
}

bool config::removeFile(const __UNICODE_CHAR* path) noexcept {
# ifdef _WINDOWS
    if (_wremove(path) == 0)
      return true;
# else
    if (remove(path) == 0)
      return true;
# endif
  errno = 0;
  return false;
}

bool config::createDirectory(const __UNICODE_CHAR* path) noexcept {
# ifdef _WINDOWS
    if (_wmkdir(path) == 0)
      return true;
# else
    if (mkdir(path, (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH)) == 0)
      return true;
# endif
  errno = 0;
  return false;
}

// ---

bool config::isPathReadable(const __UNICODE_CHAR* path) noexcept {
# ifdef _WINDOWS
    return (path != nullptr && _waccess(path, 2) == 0);
# else
    return (path != nullptr && access(path, 2) == 0);
# endif
}

bool config::isPathWritable(const __UNICODE_CHAR* path) noexcept {
# ifdef _WINDOWS
    return (path != nullptr && _waccess(path, 6) == 0);
# else
    return (path != nullptr && access(path, 6) == 0);
# endif
}

// ---

// Read current execution directory path (no trailing separator)
UnicodeString config::getCurrentDir() {
  __UNICODE_CHAR buffer[__MAX_PATH_LENGTH];
# ifdef _WINDOWS
    DWORD length = GetCurrentDirectoryW(__MAX_PATH_LENGTH, buffer);
# else
    size_t length = 0;
    char* envPwd = getenv("PWD");
    if (envPwd != nullptr) {
      length = strlen(envPwd);
      if (length >= __MAX_PATH_LENGTH)
        length = __MAX_PATH_LENGTH - 1;
      memcpy(buffer, envPwd, length*sizeof(__UNICODE_CHAR));
      buffer[length] = '\0';
    }
    else if (getcwd(buffer, __MAX_PATH_LENGTH - 1) != nullptr)
      length = strlen(buffer);
# endif

  if (length > 1) {
    if (buffer[length - 1] == (__UNICODE_CHAR)'\\' || buffer[length - 1] == (__UNICODE_CHAR)'/')
      --length;
    return UnicodeString(buffer, (size_t)length);
  }
  return UnicodeString(__UNICODE_STR("."));
}


// Read emulator process file path and name (if available)
UnicodeString config::getProcessPath() {
  __UNICODE_CHAR buffer[__MAX_PATH_LENGTH];
# if defined(_WINDOWS)
    DWORD length = GetModuleFileNameW(nullptr, buffer, __MAX_PATH_LENGTH);
    return (length > 0) ? UnicodeString(buffer, (size_t)length) : UnicodeString{};
# elif !defined(__APPLE__) && (defined(PLATFORM_POSIX) || defined(__linux__) || defined(__linux))
    ssize_t length = readlink("/proc/self/exe", buffer, __MAX_PATH_LENGTH - 1);
    return (length > 0) ? UnicodeString(buffer, (size_t)length) : UnicodeString{};
# else
    pid_t pid = getpid();
    proc_pidpath(pid, buffer, __MAX_PATH_LENGTH);
    return UnicodeString(buffer);
# endif
}


// -- emulator detection -- ----------------------------------------------------

void config::readEmulatorInfo(EmulatorInfo& outInfo) {
  // get process path/name (if available) or current directory path
  bool isProcessDirPath = false;
  auto processPath = config::getProcessPath();
  if (processPath.empty()) {
    processPath = config::getCurrentDir();
    isProcessDirPath = true;
  }

  // get process file name (or process directory name, if getProcessPath() failed)
  size_t nameLength = 0;
  const __UNICODE_CHAR* fileName = &(processPath.c_str()[processPath.size() - 1]);
  while (fileName >= processPath.c_str() && *fileName != (__UNICODE_CHAR)'\\' && *fileName != (__UNICODE_CHAR)'/') {
    --fileName;
    ++nameLength;
  }
  ++fileName; // move after separator (or at first index, if no separator)

  // detect emulator type based on name
  switch (*fileName) {
    case (__UNICODE_CHAR)'p':
    case (__UNICODE_CHAR)'P': outInfo.type = EmulatorType::pcsxr; break;
    case (__UNICODE_CHAR)'e':
    case (__UNICODE_CHAR)'E': outInfo.type = EmulatorType::epsxe; break;
    case (__UNICODE_CHAR)'z':
    case (__UNICODE_CHAR)'Z': outInfo.type = EmulatorType::zinc; break;
    default: outInfo.type = EmulatorType::unknown; break;
  }

  // find plugin directory path
# ifdef _WINDOWS
    if (pandora::system::WindowsApp::instance().isInitialized()) {
      wchar_t pluginPath[MAX_PATH + 1];
      DWORD length = GetModuleFileNameW((HMODULE)pandora::system::WindowsApp::instance().handle(), pluginPath, MAX_PATH + 1);
      if (length > 0) {
        for (wchar_t* it = &pluginPath[length - 1]; length && *it != L'\\' && *it != L'/'; --it)
          --length;
        if (length > 0) {
          outInfo.pluginDir.assign(pluginPath, length);
          return; // found -> return here
        }
      }
    }
# endif

  outInfo.pluginDir = isProcessDirPath
                    ? processPath + __UNICODE_STR(__ABS_PATH_SEP) // current dir -> append separator
                    : UnicodeString(processPath.c_str(), processPath.size() - nameLength); // full process path -> remove name
  if (outInfo.type != EmulatorType::zinc)
    outInfo.pluginDir.append(__UNICODE_STR("plugins" __ABS_PATH_SEP));
}


// -- config file paths -- -----------------------------------------------------

UnicodeString config::getLocalUserConfigDir() {
  auto appDataDirs = pandora::io::FileSystemLocationFinder::standardLocation(pandora::io::FileSystemLocation::appData,
                                                            __UNICODE_STR("Games" __ABS_PATH_SEP "gpuPandoraGS" __ABS_PATH_SEP));
  return UnicodeString(appDataDirs.front().c_str(), appDataDirs.front().size());
}
UnicodeString config::getLocalUserParentDir() {
  auto appDataDirs = pandora::io::FileSystemLocationFinder::standardLocation(pandora::io::FileSystemLocation::appData,
    __UNICODE_STR("Games" __ABS_PATH_SEP));
  return UnicodeString(appDataDirs.front().c_str(), appDataDirs.front().size());
}
UnicodeString config::toLocalUserConfigDir(const UnicodeString& localUserParentDir) {
  return localUserParentDir + __UNICODE_STR("gpuPandoraGS" __ABS_PATH_SEP);
}

UnicodeString config::findConfigDir(const UnicodeString& pluginDir) {
  auto configDir = getPortableConfigDir(pluginDir);
  if (!isPathWritable(configDir.c_str())) {
    configDir = getLocalUserConfigDir();
    if (!isPathWritable(configDir.c_str()))
      configDir.clear();
  }
  return configDir;
}


// -- game/profile binding paths -- --------------------------------------------

#define __GAME_ID_BUFFER_SIZE 48
#define __GAME_ID_SUFFIX      __UNICODE_STR(".bind")
#define __GAME_ID_SUFFIX_SIZE 5

UnicodeString config::getGameBindingPath(const UnicodeString& configDir, const char* gameId) {
  __UNICODE_CHAR buffer[__GAME_ID_BUFFER_SIZE];

  *buffer = (__UNICODE_CHAR)__ABS_PATH_SEP[0]; // add separator before file name (will be appended after dir path)
  __UNICODE_CHAR* cur = &buffer[1]; // skip separator
  __UNICODE_CHAR* lastNameIndex = &buffer[__GAME_ID_BUFFER_SIZE - __GAME_ID_SUFFIX_SIZE - 1];

  while (*gameId && cur < lastNameIndex) {
    *cur = ((*gameId >= '@' && *gameId != '\\' && *gameId != '|')
         || (*gameId <= '9' && *gameId >= '#' && *gameId != '*' && *gameId != '/'))
      ? static_cast<__UNICODE_CHAR>(*gameId)
      : static_cast<__UNICODE_CHAR>('_'); // replace reserved characters with '_'

    ++cur;
    ++gameId;
  }
  memcpy(cur, __GAME_ID_SUFFIX, (__GAME_ID_SUFFIX_SIZE + 1)*sizeof(__UNICODE_CHAR)); // add suffix (include end zero -> size + 1)

  return getGameBindingsDir(configDir) + buffer;
}
