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
#pragma once

#include <cstdint>
#include <cstddef>
#include <cstring>
#include <memory/light_string.h>

#ifdef _WINDOWS
  using LoggerPath = pandora::memory::LightWString;
# ifndef __FILE_NAME__
#   define __FILE_NAME__ strrchr("\\" __FILE__, '\\') + 1
# endif
#else
  using LoggerPath = pandora::memory::LightString;
# ifndef __FILE_NAME__
#   define __FILE_NAME__ strrchr("/" __FILE__, '/') + 1
# endif
#endif

namespace psemu {
  /// @brief System logger (for warnings, errors, debugging...)
  class SysLog final {
  public:
    // -- initialization --

    /// @brief Initialize log file directory path (with trailing slash/backslash)
    /// @remarks Has no effect if some messages have already been logged (logger is created with first message log)
    static void init(const LoggerPath& logDir);
    /// @brief Flush and shutdown logger
    static void close();

    // -- messages --

#   if defined(_DEBUG) || !defined(NDEBUG)
      static void logDebug(const char* origin, uint32_t line, const char* format, ...); ///< Verbose log (debug mode only)
      static void logInfo(const char* origin, uint32_t line, const char* message);  ///< Informative log (debug mode only)
#   else
      static inline void logDebug(...) {}
      static inline void logInfo(...) {}
#   endif
    static void logWarning(const char* origin, uint32_t line, const char* message); ///< Log warning message
    static void logError(const char* origin, uint32_t line, const char* message);   ///< Log error message
  };
}
