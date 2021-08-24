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
#include <display/status_register.h>

using namespace display;

class StatusRegisterTest : public testing::Test {
public:
protected:
  //static void SetUpTestCase() {}
  //static void TearDownTestCase() {}

  void SetUp() override {}
  void TearDown() override {}
};


TEST_F(StatusRegisterTest, getSetHardwareInfo) {
  StatusRegister reg;
  reg.setGpuReadBuffer(12345u);
  EXPECT_EQ((unsigned long)12345u, reg.getGpuReadBuffer());
  EXPECT_EQ((unsigned long)0x14802000u, reg.getStatusControlRegister());
  reg.setStatusControlRegister(0);
  EXPECT_EQ((unsigned long)0, reg.getStatusControlRegister());
  EXPECT_EQ((unsigned long)0, reg.readStatus((StatusBits)0xFFFFFFFFu));

  // GPU type
  reg.setGpuType(GpuVersion::psxGpu208pin, psxVramHeight());
  EXPECT_EQ(GpuVersion::psxGpu208pin, reg.getGpuVersion());
  EXPECT_EQ(psxVramHeight(), reg.getGpuVramHeight());
  reg.requestGpuInfo((unsigned long)GpuInfoType::gpuVersion);
  EXPECT_EQ((unsigned long)2, reg.getGpuReadBuffer());

  reg.setGpuType(GpuVersion::arcadeGpu1, znArcadeVramHeight());
  EXPECT_EQ(GpuVersion::arcadeGpu1, reg.getGpuVersion());
  EXPECT_EQ(znArcadeVramHeight(), reg.getGpuVramHeight());
  reg.requestGpuInfo((unsigned long)GpuInfoType::gpuVersion);
  EXPECT_EQ((unsigned long)2, reg.getGpuReadBuffer());

  reg.setGpuType(GpuVersion::arcadeGpu2, znArcadeVramHeight());
  EXPECT_EQ(GpuVersion::arcadeGpu2, reg.getGpuVersion());
  EXPECT_EQ(znArcadeVramHeight(), reg.getGpuVramHeight());
  reg.requestGpuInfo((unsigned long)GpuInfoType::gpuVersion);
  EXPECT_EQ((unsigned long)1, reg.getGpuReadBuffer());

  // DMA
  EXPECT_EQ(DmaMode::off, reg.readStatus<DmaMode>(StatusBits::dmaMode));
  reg.setDmaMode(1);
  EXPECT_EQ(DmaMode::fifoStatus, reg.readStatus<DmaMode>(StatusBits::dmaMode));
  reg.setDmaMode(2);
  EXPECT_EQ(DmaMode::cpuToGpu, reg.readStatus<DmaMode>(StatusBits::dmaMode));
  reg.setDmaMode(3);
  EXPECT_EQ(DmaMode::gpuToCpu, reg.readStatus<DmaMode>(StatusBits::dmaMode));
  reg.setDmaMode(0);
  EXPECT_EQ(DmaMode::off, reg.readStatus<DmaMode>(StatusBits::dmaMode));

  EXPECT_EQ(DataTransfer::primitives, reg.getDataTransferMode());
  reg.setDataTransferMode(DataTransfer::vramWrite);
  EXPECT_EQ(DataTransfer::vramWrite, reg.getDataTransferMode());
  reg.setDataTransferMode(DataTransfer::vramRead);
  EXPECT_EQ(DataTransfer::vramRead, reg.getDataTransferMode());
  reg.setDataTransferMode(DataTransfer::primitives);
  EXPECT_EQ(DataTransfer::primitives, reg.getDataTransferMode());

  // lightgun
  reg.requestGpuInfo((unsigned long)GpuInfoType::biosAddress);
  EXPECT_EQ(biosModuleAddress(), reg.getGpuReadBuffer());
  EXPECT_EQ((unsigned long)0, reg.getActiveLightgunsMap());
  reg.requestGpuInfo((unsigned long)GpuInfoType::lightgunStatus);
  EXPECT_EQ((unsigned long)0, reg.getGpuReadBuffer());

  reg.setLightgunCursor(0, 1, 2);
  EXPECT_EQ((unsigned long)1, reg.getActiveLightgunsMap());
  EXPECT_EQ((long)1, reg.lightgunCursorPositions()[0].x);
  EXPECT_EQ((long)2, reg.lightgunCursorPositions()[0].y);
  reg.requestGpuInfo((unsigned long)GpuInfoType::lightgunStatus);
  EXPECT_EQ(biosModuleAddress(), reg.getGpuReadBuffer());

  reg.clearLightgunCursors();
  EXPECT_EQ((unsigned long)0, reg.getActiveLightgunsMap());
  reg.requestGpuInfo((unsigned long)GpuInfoType::lightgunStatus);
  EXPECT_EQ((unsigned long)0, reg.getGpuReadBuffer());
}

TEST_F(StatusRegisterTest, commandHelpersTest) {
  EXPECT_EQ((unsigned long)0, StatusRegister::getGp0CommandId(0));
  EXPECT_EQ((unsigned long)0x12, StatusRegister::getGp0CommandId(0x12345678));
  EXPECT_EQ((unsigned long)0, StatusRegister::getGp0CommandId(0x123456));
  EXPECT_EQ((unsigned long)0x7, StatusRegister::getGp0CommandId(0x7654321));
  EXPECT_EQ((unsigned long)0x22, StatusRegister::getGp0CommandId(0x22000000));
  EXPECT_EQ((unsigned long)0xFF, StatusRegister::getGp0CommandId(0xFF000000));
  EXPECT_EQ((ControlCommandId)0, StatusRegister::getGp1CommandId(0));
  EXPECT_EQ((ControlCommandId)0x12, StatusRegister::getGp1CommandId(0x12345678));
  EXPECT_EQ((ControlCommandId)0, StatusRegister::getGp1CommandId(0x123456));
  EXPECT_EQ((ControlCommandId)0x7, StatusRegister::getGp1CommandId(0x7654321));
  EXPECT_EQ((ControlCommandId)0x22, StatusRegister::getGp1CommandId(0x22000000));
  EXPECT_EQ((ControlCommandId)0x3F, StatusRegister::getGp1CommandId(0x3F000000));
  EXPECT_EQ((ControlCommandId)0x3F, StatusRegister::getGp1CommandId(0xFF000000));
  EXPECT_EQ((ControlCommandId)0x7, StatusRegister::getGp1CommandId(0xC7000000));

  for (unsigned long i = 0x10; i < 0x20; ++i) {
    EXPECT_TRUE(StatusRegister::isGpuInfoRequestMirror((ControlCommandId)i));
  }
  EXPECT_FALSE(StatusRegister::isGpuInfoRequestMirror((ControlCommandId)0x00));
  EXPECT_FALSE(StatusRegister::isGpuInfoRequestMirror((ControlCommandId)0x09));
  EXPECT_FALSE(StatusRegister::isGpuInfoRequestMirror((ControlCommandId)0x0F));
  EXPECT_FALSE(StatusRegister::isGpuInfoRequestMirror((ControlCommandId)0x20));
}

TEST_F(StatusRegisterTest, gpuReadinessTest) {
  StatusRegister reg;
  EXPECT_TRUE(reg.readStatus<bool>(StatusBits::readyForCommands));
  EXPECT_TRUE(reg.readStatus<bool>(StatusBits::readyForDmaBlock));
  EXPECT_FALSE(reg.readStatus<bool>(StatusBits::readyForDmaRead));
  EXPECT_EQ(((unsigned long)StatusBits::readyForCommands | (unsigned long)StatusBits::readyForDmaBlock),
            reg.getStatusControlRegister() & ((unsigned long)StatusBits::readyForCommands | (unsigned long)StatusBits::readyForDmaBlock));

  reg.enableBusyGpuHack(true);
  reg.setGp0CommandFinished();
  EXPECT_FALSE(reg.getStatusControlRegister() & ((unsigned long)StatusBits::readyForCommands | (unsigned long)StatusBits::readyForDmaBlock));
  EXPECT_TRUE(reg.getStatusControlRegister() & ((unsigned long)StatusBits::readyForCommands | (unsigned long)StatusBits::readyForDmaBlock));
  EXPECT_FALSE(reg.getStatusControlRegister() & ((unsigned long)StatusBits::readyForCommands | (unsigned long)StatusBits::readyForDmaBlock));
  EXPECT_TRUE(reg.getStatusControlRegister() & ((unsigned long)StatusBits::readyForCommands | (unsigned long)StatusBits::readyForDmaBlock));
  reg.setGp0CommandFinished();
  EXPECT_FALSE(reg.getStatusControlRegister() & ((unsigned long)StatusBits::readyForCommands | (unsigned long)StatusBits::readyForDmaBlock));
  EXPECT_TRUE(reg.getStatusControlRegister() & ((unsigned long)StatusBits::readyForCommands | (unsigned long)StatusBits::readyForDmaBlock));
  reg.clearPendingCommands();
  EXPECT_TRUE(reg.getStatusControlRegister() & ((unsigned long)StatusBits::readyForCommands | (unsigned long)StatusBits::readyForDmaBlock));
  reg.enableBusyGpuHack(false);
  EXPECT_TRUE(reg.getStatusControlRegister() & ((unsigned long)StatusBits::readyForCommands | (unsigned long)StatusBits::readyForDmaBlock));

  reg.setGpuIdle();
  EXPECT_TRUE(reg.readStatus<bool>(StatusBits::readyForCommands));
  reg.setGpuBusy();
  EXPECT_FALSE(reg.readStatus<bool>(StatusBits::readyForCommands));
  reg.setGpuIdle();
  EXPECT_TRUE(reg.readStatus<bool>(StatusBits::readyForCommands));

  reg.setGp0CommandFinished();
  EXPECT_TRUE(reg.readStatus<bool>(StatusBits::readyForDmaBlock));
  EXPECT_FALSE(reg.readStatus<bool>(StatusBits::dmaRequestState));
  reg.setGp0CommandReceived();
  EXPECT_FALSE(reg.readStatus<bool>(StatusBits::readyForDmaBlock));
  EXPECT_FALSE(reg.readStatus<bool>(StatusBits::dmaRequestState));
  reg.setGp0CommandFinished();
  EXPECT_TRUE(reg.readStatus<bool>(StatusBits::readyForDmaBlock));
  EXPECT_FALSE(reg.readStatus<bool>(StatusBits::dmaRequestState));
  reg.setVramReadFinished();
  EXPECT_FALSE(reg.readStatus<bool>(StatusBits::readyForDmaRead));
  EXPECT_FALSE(reg.readStatus<bool>(StatusBits::dmaRequestState));
  reg.setVramReadPending();
  EXPECT_TRUE(reg.readStatus<bool>(StatusBits::readyForDmaRead));
  EXPECT_FALSE(reg.readStatus<bool>(StatusBits::dmaRequestState));
  reg.setVramReadFinished();
  EXPECT_FALSE(reg.readStatus<bool>(StatusBits::readyForDmaRead));
  EXPECT_FALSE(reg.readStatus<bool>(StatusBits::dmaRequestState));

  reg.setDmaMode(2);
  reg.setGp0CommandFinished();
  EXPECT_TRUE(reg.readStatus<bool>(StatusBits::readyForDmaBlock));
  EXPECT_TRUE(reg.readStatus<bool>(StatusBits::dmaRequestState));
  reg.setGp0CommandReceived();
  EXPECT_FALSE(reg.readStatus<bool>(StatusBits::readyForDmaBlock));
  EXPECT_FALSE(reg.readStatus<bool>(StatusBits::dmaRequestState));
  reg.setGp0CommandFinished();
  EXPECT_TRUE(reg.readStatus<bool>(StatusBits::readyForDmaBlock));
  EXPECT_TRUE(reg.readStatus<bool>(StatusBits::dmaRequestState));
  reg.setDmaMode(0);
  EXPECT_TRUE(reg.readStatus<bool>(StatusBits::readyForDmaBlock));
  EXPECT_FALSE(reg.readStatus<bool>(StatusBits::dmaRequestState));
  reg.setDmaMode(3);
  reg.setVramReadFinished();
  EXPECT_FALSE(reg.readStatus<bool>(StatusBits::readyForDmaRead));
  EXPECT_FALSE(reg.readStatus<bool>(StatusBits::dmaRequestState));
  reg.setVramReadPending();
  EXPECT_TRUE(reg.readStatus<bool>(StatusBits::readyForDmaRead));
  EXPECT_TRUE(reg.readStatus<bool>(StatusBits::dmaRequestState));
  reg.setVramReadFinished();
  EXPECT_FALSE(reg.readStatus<bool>(StatusBits::readyForDmaRead));
  EXPECT_FALSE(reg.readStatus<bool>(StatusBits::dmaRequestState));

  EXPECT_FALSE(reg.readStatus<bool>(StatusBits::interruptReq1));
  reg.setIrq1();
  EXPECT_TRUE(reg.readStatus<bool>(StatusBits::interruptReq1));
  reg.setIrq1();
  EXPECT_TRUE(reg.readStatus<bool>(StatusBits::interruptReq1));
  reg.ackIrq1();
  EXPECT_FALSE(reg.readStatus<bool>(StatusBits::interruptReq1));
  reg.ackIrq1();
  EXPECT_FALSE(reg.readStatus<bool>(StatusBits::interruptReq1));
}
