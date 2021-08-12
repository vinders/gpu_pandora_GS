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
#include "psemu/timer.h"
// this implementation is included in psemu_gpu.cpp -> single object file (less overhead + smaller lib size)

using namespace psemu;


bool Timer::waitPeriod() noexcept {
  // normal speed -> use timer
  if (this->_speed == SpeedMode::normal) {
    auto lateness = this->_clock.waitPeriod();
    return (this->_useFrameSkipping && lateness.count() > this->_clock.periodDuration().count() / 2);
  }
  else {
    // slow-motion -> wait longer
    if (this->_speed == SpeedMode::slowMotion)
      std::this_thread::sleep_for(this->_clock.periodDuration() * 4);
    //else: SpeedMode::turbo (no wait) / SpeedMode::none (no wait)
      // -> nothing to do
    return false;
  }
}
