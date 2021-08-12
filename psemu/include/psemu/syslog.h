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
#include <memory/light_string.h>
#include <system/trace.h>

# ifdef _WINDOWS
    using LoggerPath = pandora::memory::LightWString;
# else
    using LoggerPath = pandora::memory::LightString;
# endif

namespace psemu {
  /// @brief System logger (for warnings, errors, debugging...)
  class SysLog final {
  public:
    // -- initialization --

    /// @brief Initialize log file directory path (with trailing slash/backslash)
    /// @remarks Has no effect if some messages have already been logged (logger is created with first message log)
    static void init(const LoggerPath& logDir);

    // -- messages --

#   if defined(_DEBUG) || !defined(NDEBUG)
      static void logDebug(const char* origin, uint32_t line, const char* format, ...); ///< Verbose log (debug mode only)
      static void logInfo(const char* origin, uint32_t line, const char* message);  ///< Informative log (debug mode only)
#   else
#     define logDebug(origin,line,format,...) ;
#     define logInfo(origin,line,format,...)  ;
#   endif
    static void logWarning(const char* origin, uint32_t line, const char* message); ///< Log warning message
    static void logError(const char* origin, uint32_t line, const char* message);   ///< Log error message
  };
}
