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
#include <gtest/gtest.h>
#include <display/status_lock.h>

using namespace display;

class StatusLockTest : public testing::Test {
public:
protected:
  //static void SetUpTestCase() {}
  //static void TearDownTestCase() {}

  void SetUp() override {}
  void TearDown() override {}
};


TEST_F(StatusLockTest, gpuBusyLockTest) {
  StatusRegister reg;
  EXPECT_TRUE(reg.readStatus<bool>(StatusBits::readyForCommands));
  
  try { // scope
    GpuBusyStatusLock guard(reg);
    EXPECT_FALSE(reg.readStatus<bool>(StatusBits::readyForCommands));
  }
  catch (...) {}
  EXPECT_TRUE(reg.readStatus<bool>(StatusBits::readyForCommands));
  
  try { // exception
    GpuBusyStatusLock guard(reg);
    EXPECT_FALSE(reg.readStatus<bool>(StatusBits::readyForCommands));
    throw std::runtime_error("");
  }
  catch (...) {}
  EXPECT_TRUE(reg.readStatus<bool>(StatusBits::readyForCommands));
}

TEST_F(StatusLockTest, gp0CommandLockTest) {
  StatusRegister reg;
  EXPECT_TRUE(reg.readStatus<bool>(StatusBits::readyForDmaBlock));
  
  try { // scope
    Gp0CommandStatusLock guard(reg);
    EXPECT_FALSE(reg.readStatus<bool>(StatusBits::readyForDmaBlock));
  }
  catch (...) {}
  EXPECT_TRUE(reg.readStatus<bool>(StatusBits::readyForDmaBlock));
  
  try { // exception
    Gp0CommandStatusLock guard(reg);
    EXPECT_FALSE(reg.readStatus<bool>(StatusBits::readyForDmaBlock));
    throw std::runtime_error("");
  }
  catch (...) {}
  EXPECT_TRUE(reg.readStatus<bool>(StatusBits::readyForDmaBlock));
}
