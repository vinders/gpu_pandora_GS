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
#pragma once

#include "display/status_register.h"

namespace display {
  /// @brief 'GPU busy' lock guard for status register
  /// @remarks RAII lock: automatic unlock when reaching end of scope or if an exception occurs
  class GpuBusyStatusLock final {
  public:
    /// @brief Lock 'GPU busy' status flag (on object creation)
    inline GpuBusyStatusLock(StatusRegister& statusRegister) noexcept
      : _statusRegister(statusRegister) {
      this->_statusRegister.setGpuBusy();
    }
    /// @brief Unlock 'GPU busy' status flag (on object destruction)
    inline ~GpuBusyStatusLock() noexcept {
      this->_statusRegister.setGpuIdle();
    }

    GpuBusyStatusLock(const GpuBusyStatusLock&) = delete;
    GpuBusyStatusLock(GpuBusyStatusLock&&) = delete;
    GpuBusyStatusLock& operator=(const GpuBusyStatusLock&) = delete;
    GpuBusyStatusLock& operator=(GpuBusyStatusLock&&) = delete;

  private:
    StatusRegister& _statusRegister;
  };
  
  // ---
  
  /// @brief 'GP0 command processing' lock guard for status register
  /// @remarks RAII lock: automatic unlock when reaching end of scope or if an exception occurs
  class Gp0CommandStatusLock final {
  public:
    /// @brief Lock 'GP0 command processing' status flag (on object creation)
    inline Gp0CommandStatusLock(StatusRegister& statusRegister) noexcept
      : _statusRegister(statusRegister) {
      this->_statusRegister.setGp0CommandReceived();
    }
    /// @brief Unlock 'GP0 command processing' status flag (on object destruction)
    inline ~Gp0CommandStatusLock() noexcept {
      this->_statusRegister.setGp0CommandFinished();
    }

    Gp0CommandStatusLock(const Gp0CommandStatusLock&) = delete;
    Gp0CommandStatusLock(Gp0CommandStatusLock&&) = delete;
    Gp0CommandStatusLock& operator=(const Gp0CommandStatusLock&) = delete;
    Gp0CommandStatusLock& operator=(Gp0CommandStatusLock&&) = delete;

  private:
    StatusRegister& _statusRegister;
  };
}
