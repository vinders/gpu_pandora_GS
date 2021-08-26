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

#include <cstdint>

namespace display {
  static constexpr inline unsigned long psxBiosSize() noexcept { return 0x10000u; }      ///< PS1 BIOS size (reserved in RAM)
  static constexpr inline unsigned long psxRamSize() noexcept { return 0x200000u; }      ///< RAM memory of standard PS1
  static constexpr inline unsigned long znArcadeRamSize() noexcept { return 0x800000u; } ///< Maximum RAM memory of ZN-2 arcades

  /// @brief Direct-memory-access linked list iterator
  /// @remarks Iterates through DMA linked list (protected against endless loops)
  template <unsigned long _MaxRamSize>
  class DmaChainIterator final {
  public:
    DmaChainIterator(unsigned long* baseAddress, unsigned long index) noexcept
      : _baseAddress((uint32_t*)baseAddress),
        _index((baseAddress != nullptr) ? index : endIndexBits()) {}

    DmaChainIterator() = default;
    DmaChainIterator(const DmaChainIterator<_MaxRamSize>&) = default;
    DmaChainIterator& operator=(const DmaChainIterator<_MaxRamSize>&) = default;
    ~DmaChainIterator() noexcept = default;

    /// @brief Lowest bits of termination block index (false address to indicate end of chain)
    static constexpr inline unsigned long endIndexBits() noexcept { return 0xFFFFFFu; } // xxFFFFFF
    /// @brief Mask to limit addresses below max and with 4-byte alignment (ex: if max RAM size is 0x200000: mask is 0x1FFFFC)
    static constexpr inline unsigned long addressMask() noexcept { return _MaxRamSize - 4u; }
    /// @brief Max number of indexes to iterate = max 4-byte blocks = (max memory / 4) + ending block
    static constexpr inline unsigned long maxCounter() noexcept { return ((_MaxRamSize - psxBiosSize()) >> 2) + 1u; }

    // -- operations --

    /// @brief Get address and size of next data block (if available)
    /// @returns Success (true) or end of chain (false)
    /// @warning Some blocks may have a size of 0: always verify 'if(blockSize>0)' before using memory block.
    inline bool readNext(unsigned long** outMemBlock, int& outBlockSize) noexcept {
      if ((this->_index & endIndexBits()) == endIndexBits()) // xxFFFFFF == end symbol
        return false;
      this->_index &= addressMask(); // ignore bits out of range

      // prevent endless loops
      if (++(this->_counter) > maxCounter()
      ||  this->_index == this->_prevIndexes.slow
      ||  this->_index == this->_prevIndexes.lower
      ||  this->_index == this->_prevIndexes.greater) {
        this->_index = endIndexBits();
        return false;
      }
      // previous addresses, to detect loops (great for small loops and ordered chains) // inspired by Peops sources
      if (this->_index < this->_prevIndexes.latest)
        this->_prevIndexes.lower = this->_index;
      else
        this->_prevIndexes.greater = this->_index;
      this->_prevIndexes.latest = this->_index;

      // slower moving index, to detect loops (much better for large loops and unordered chains)
      if (this->_counter & 0x1) {
        if (this->_prevIndexes.slow != endIndexBits()) // not default value (already initialized)
          this->_prevIndexes.slow = (this->_baseAddress[this->_prevIndexes.slow >> 2] & addressMask());
        else
          this->_prevIndexes.slow = this->_index;
      }

      // read current value + size
      uint32_t* currentBlock = &_baseAddress[this->_index >> 2];
      *outMemBlock = (unsigned long*)(currentBlock + 1);
      outBlockSize = static_cast<int>((*currentBlock >> 24) & 0xFF);

      this->_index = *currentBlock; // move to next index (for next call)
      return true;
    }

  private:
    struct {
      unsigned long latest = endIndexBits();
      unsigned long lower  = endIndexBits();
      unsigned long greater = endIndexBits();
      unsigned long slow = endIndexBits();
    } _prevIndexes;

    uint32_t* _baseAddress = nullptr;
    unsigned long _index = endIndexBits();
    uint32_t _counter = 0;
  };
}
