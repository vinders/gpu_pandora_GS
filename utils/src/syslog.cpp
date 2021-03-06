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
#include <system/logger.h>
#include <memory>
#include "utils/syslog.h"

#ifdef _WINDOWS
# define __UNICODE_STR(str) L"" str
#else
# define __UNICODE_STR(str) u8"" str
#endif

using namespace utils;
using Logger = pandora::system::FileLogger<128>;
using Formatter = pandora::system::LogFileFormatter<128>;


// globals
std::unique_ptr<Logger> g_logger = nullptr;
LoggerPath g_logFilePath;
const char* g_logTitle = "-----";
SysLog::Level g_logLevel = SysLog::Level::debug;


// -- initialization -- --------------------------------------------------------

void SysLog::init(const LoggerPath& logDir, const char* title, SysLog::Level level) {
  g_logTitle = title;
  g_logLevel = level;
  if (!logDir.empty())
    g_logFilePath = logDir + __UNICODE_STR("gpuPandoraGS.log");
}

static bool __createLogger() noexcept {
  try {
    if (g_logFilePath.empty())
      g_logFilePath = __UNICODE_STR("./plugins/gpuPandoraGS.log"); // default path

    g_logger.reset(new Logger(Formatter(std::ofstream(g_logFilePath.c_str(), std::ios::out|std::ios::app), g_logTitle),
                              (pandora::system::LogLevel)g_logLevel));
    g_logFilePath.clear(); // no need to keep string alloc anymore
    return true;
  }
  catch (...) { return false; }
}

void SysLog::close() {
  g_logger.reset();
}


// -- messages -- --------------------------------------------------------------

#if defined(_DEBUG) || !defined(NDEBUG)
  void SysLog::logDebug(const char* origin, uint32_t line, const char* format, ...) {
    if (g_logLevel > SysLog::Level::debug || (!g_logger && !__createLogger())) // try to create if not already existing
      return;

    va_list args;
    va_start(args, format);
    g_logger->logArgs(pandora::system::LogLevel::debug, pandora::system::LogCategory::none, origin, line, format, args);
    va_end(args);
  }
#endif

void SysLog::logInfo(const char* origin, uint32_t line, const char* format, ...) {
  if (g_logLevel > SysLog::Level::info || (!g_logger && !__createLogger())) // try to create if not already existing
    return;
  va_list args;
  va_start(args, format);
  g_logger->logArgs(pandora::system::LogLevel::informative, pandora::system::LogCategory::INFO, origin, line, format, args);
  va_end(args);
}

void SysLog::logWarning(const char* origin, uint32_t line, const char* message) {
  if (g_logLevel > SysLog::Level::warning || (!g_logger && !__createLogger())) // try to create if not already existing
    return;
  g_logger->log(pandora::system::LogLevel::standard, pandora::system::LogCategory::WARNING, origin, line, message);
}
void SysLog::logError(const char* origin, uint32_t line, const char* message) {
  if (!g_logger && !__createLogger()) // try to create if not already existing
    return;
  g_logger->log(pandora::system::LogLevel::critical, pandora::system::LogCategory::ERROR, origin, line, message);
}
