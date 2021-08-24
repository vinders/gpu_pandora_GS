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

#include <time/timer.h>
#include "display/types.h"

namespace psemu {
  /// @brief Special speed mode
  enum class SpeedMode : int {
    none       = 0, ///< No framerate limit
    normal     = 1, ///< Normal game speed limit
    turbo      = 2, ///< Turbo mode (fast-forward)
    slowMotion = 3  ///< Slow-motion mode
  };

  // ---

  /// @brief Emulator synchronization timer (framerate limit + skipping management)
  class Timer final {
  public:
    using Clock = pandora::time::Timer<pandora::time::HighResolutionClock,pandora::time::HighResolutionAuxClock,
                                       pandora::time::DelayHandling::compensate,true>;
    Timer() = default;
    Timer(const Timer&) = delete;
    Timer(Timer&&) = default;
    Timer& operator=(const Timer&) = delete;
    Timer& operator=(Timer&&) = default;
    ~Timer() noexcept = default;


    // -- settings --

    /// @brief Set frequency based on SMPTE standard and CPU clock (auto-detection)
    inline void setFrequency(display::SmpteStandard standard, bool isInterlaced) noexcept {
      double targetRate = cpuFieldRate(standard, isInterlaced);
      if (targetRate != this->_rate) {
        this->_rate = targetRate;
        this->_clock.reset<true, false>(targetRate);
      }
    }
    /// @brief Set custom frequency value
    inline void setFrequency(float framerateLimit) noexcept {
      this->_rate = (double)framerateLimit;
      this->_clock.reset<true,false>(this->_rate);
    }
    /// @brief Enable/disable adaptative frame skipping
    inline void setFrameSkipping(bool isEnabled) noexcept {
      this->_useFrameSkipping = isEnabled;
    }

    /// @brief Set special speed mode (normal / turbo / slow-motion)
    inline void setSpeedMode(SpeedMode mode) noexcept {
      this->_speed = mode;
      this->_clock.reset<true, true>();
    }
    inline SpeedMode getSpeedMode() noexcept { return this->_speed; } ///< Get current speed mode


    // -- operations --

    /// @brief Reset clock reference
    inline void reset() noexcept {
      this->_clock.reset<true, true>();
    }

    /// @brief Wait for remaining time of current frame period
    /// @returns True if next frame should be skipped
    bool waitPeriod() noexcept;


  private:
    static constexpr inline double cpuFieldRate(display::SmpteStandard standard, bool isInterlaced) noexcept { 
      return (standard == display::SmpteStandard::ntsc)
             ? (isInterlaced ? r3000aCpuFrequency()/565031.25 : r3000aCpuFrequency()/566107.50)  // NTSC: 59.94146i or 59.82751p
             : (isInterlaced ? r3000aCpuFrequency()/677343.75 : r3000aCpuFrequency()/680595.00); // PAL : 50.00238i or 49.76352p
    }
    static constexpr inline double r3000aCpuFrequency() noexcept { return 33868800.0; } // Clock frequency of PlayStation CPU R3000a

  private:
    Clock _clock{ cpuFieldRate(display::SmpteStandard::ntsc, false) };
    double _rate = cpuFieldRate(display::SmpteStandard::ntsc, false);
    SpeedMode _speed = SpeedMode::normal;
    bool _useFrameSkipping = false;
  };
}
