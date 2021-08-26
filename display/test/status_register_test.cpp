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

TEST_F(StatusRegisterTest, statusResetTest) {
  StatusRegister empty;
  StatusRegister reg;
  reg.setStatusControlRegister(0xFFFFFFFFu);
  reg.setDataTransferMode(DataTransfer::vramWrite);
  reg.setTexturePageMode(0xFFFFFFFFu);
  reg.setTextureWindow(0xFFFFFFFFu);

  reg.resetGpu();
  EXPECT_EQ(empty.getStatusControlRegister(), reg.getStatusControlRegister());
  EXPECT_EQ((unsigned long)0x14802000u, reg.getStatusControlRegister());
  EXPECT_EQ(empty.getDataTransferMode(), reg.getDataTransferMode());
  EXPECT_EQ(DataTransfer::primitives, reg.getDataTransferMode());
  EXPECT_EQ((long)0, reg.getDisplayState().displayOrigin.x);
  EXPECT_EQ((long)0, reg.getDisplayState().displayOrigin.y);
  EXPECT_EQ((long)256, reg.getDisplayState().displayAreaSize.x);
  EXPECT_EQ((long)240, reg.getDisplayState().displayAreaSize.y);
  EXPECT_EQ((long)0, reg.getDisplayState().drawOffset.x);
  EXPECT_EQ((long)0, reg.getDisplayState().drawOffset.y);
  EXPECT_EQ((long)__DEFAULT_RANGE_X0, reg.getDisplayState().displayRange.leftX);
  EXPECT_EQ((long)__DEFAULT_RANGE_X1, reg.getDisplayState().displayRange.rightX);
  EXPECT_EQ((long)__DEFAULT_RANGE_Y0, reg.getDisplayState().displayRange.topY);
  EXPECT_EQ((long)__DEFAULT_RANGE_Y1, reg.getDisplayState().displayRange.bottomY);
  EXPECT_FALSE(reg.getTextureWindow().isEnabled);
  EXPECT_EQ((long)0, reg.getTextureWindow().offsetX);
  EXPECT_EQ((long)0, reg.getTextureWindow().offsetY);
  EXPECT_EQ((long)256, reg.getTextureWindow().maskWidth);
  EXPECT_EQ((long)256, reg.getTextureWindow().maskHeight);
  EXPECT_EQ(empty.getTexpageBaseX(), reg.getTexpageBaseX());
  EXPECT_EQ((long)0, reg.getTexpageBaseX());
  EXPECT_EQ(empty.getTexpageBaseY(), reg.getTexpageBaseY());
  EXPECT_EQ((long)0, reg.getTexpageBaseY());
  EXPECT_EQ(empty.isTextureFlipX(), reg.isTextureFlipX());
  EXPECT_FALSE(reg.isTextureFlipX());
  EXPECT_EQ(empty.isTextureFlipY(), reg.isTextureFlipY());
  EXPECT_FALSE(reg.isTextureFlipY());

  unsigned long history[controlCommandNumber()];
  StatusRegister::resetControlCommandHistory(history);
  for (int i = 0; i <= (int)ControlCommandId::displayMode; ++i) {
    EXPECT_EQ(i, (int)(history[i] >> 24));
  }
  EXPECT_EQ((unsigned long)1, (history[3] & 0xFFFFFF));
  EXPECT_EQ((unsigned long)0, (history[4] & 0xFFFFFF));
  EXPECT_EQ((unsigned long)0, (history[5] & 0xFFFFFF));
  EXPECT_EQ((unsigned long)0xC60260, (history[6] & 0xFFFFFF));
  EXPECT_EQ((unsigned long)0x040010, (history[7] & 0xFFFFFF));
  EXPECT_EQ((unsigned long)0, (history[8] & 0xFFFFFF));
}

TEST_F(StatusRegisterTest, statusGp1ModeAttrTest) {
  StatusRegister reg;
  
  // enable/disable display
  unsigned long prevStatus = reg.getStatusControlRegister();
  reg.toggleDisplay(0x1u);
  EXPECT_TRUE(reg.readStatus<bool>(StatusBits::disableDisplay));
  EXPECT_EQ((prevStatus & ~((unsigned long)StatusBits::disableDisplay)),
            (reg.getStatusControlRegister() & ~((unsigned long)StatusBits::disableDisplay)));
  reg.toggleDisplay(0x0u);
  EXPECT_FALSE(reg.readStatus<bool>(StatusBits::disableDisplay));
  EXPECT_EQ((prevStatus & ~((unsigned long)StatusBits::disableDisplay)),
            (reg.getStatusControlRegister() & ~((unsigned long)StatusBits::disableDisplay)));
  reg.toggleDisplay(0x03000001u);
  EXPECT_TRUE(reg.readStatus<bool>(StatusBits::disableDisplay));
  EXPECT_EQ((prevStatus & ~((unsigned long)StatusBits::disableDisplay)),
            (reg.getStatusControlRegister() & ~((unsigned long)StatusBits::disableDisplay)));
  reg.toggleDisplay(0x03000000u);
  EXPECT_FALSE(reg.readStatus<bool>(StatusBits::disableDisplay));
  EXPECT_EQ((prevStatus & ~((unsigned long)StatusBits::disableDisplay)),
            (reg.getStatusControlRegister() & ~((unsigned long)StatusBits::disableDisplay)));

  // empty/full
  prevStatus = reg.getStatusControlRegister();
  reg.setDisplayMode(0);
  EXPECT_EQ((unsigned long)SmpteStandard::ntsc,
            (reg.getStatusControlRegister() & displayModeBits()));
  EXPECT_EQ((long)256, reg.getDisplayState().displayAreaSize.x);
  EXPECT_EQ((long)240, reg.getDisplayState().displayAreaSize.y);
  EXPECT_EQ((prevStatus & ~(displayModeBits())),
            (reg.getStatusControlRegister() & ~(displayModeBits())));
  reg.setDisplayMode(0x8u);
  EXPECT_EQ((unsigned long)SmpteStandard::pal,
            (reg.getStatusControlRegister() & displayModeBits()));
  EXPECT_EQ((long)256, reg.getDisplayState().displayAreaSize.x);
  EXPECT_EQ((long)256, reg.getDisplayState().displayAreaSize.y);
  EXPECT_EQ((prevStatus & ~(displayModeBits())),
            (reg.getStatusControlRegister() & ~(displayModeBits())));
  reg.setDisplayMode(0xFFFFFFFFu);
  EXPECT_EQ((unsigned long)StatusBits::reverseFlag | (unsigned long)StatusBits::displayAreaWidth1
          | (unsigned long)StatusBits::displayAreaWidth2 | (unsigned long)StatusBits::displayAreaHeight
          | (unsigned long)SmpteStandard::pal|(unsigned long)StatusBits::colorDepth|(unsigned long)StatusBits::verticalInterlacing,
            (reg.getStatusControlRegister() & displayModeBits()));
  EXPECT_EQ((long)384, reg.getDisplayState().displayAreaSize.x);
  EXPECT_EQ((long)512, reg.getDisplayState().displayAreaSize.y);
  EXPECT_EQ((prevStatus & ~(displayModeBits())),
            (reg.getStatusControlRegister() & ~(displayModeBits())));

  // widths
  reg.setDisplayMode(0x1u);
  EXPECT_EQ((unsigned long)1 << bitOffset_displayAreaWidth2() | (unsigned long)SmpteStandard::ntsc,
            (reg.getStatusControlRegister() & displayModeBits()));
  EXPECT_EQ((long)320, reg.getDisplayState().displayAreaSize.x);
  EXPECT_EQ((long)240, reg.getDisplayState().displayAreaSize.y);
  reg.setDisplayMode(0x2u);
  EXPECT_EQ((unsigned long)2 << bitOffset_displayAreaWidth2() | (unsigned long)SmpteStandard::ntsc,
            (reg.getStatusControlRegister() & displayModeBits()));
  EXPECT_EQ((long)512, reg.getDisplayState().displayAreaSize.x);
  EXPECT_EQ((long)240, reg.getDisplayState().displayAreaSize.y);
  reg.setDisplayMode(0x3u);
  EXPECT_EQ((unsigned long)3 << bitOffset_displayAreaWidth2() | (unsigned long)SmpteStandard::ntsc,
            (reg.getStatusControlRegister() & displayModeBits()));
  EXPECT_EQ((long)640, reg.getDisplayState().displayAreaSize.x);
  EXPECT_EQ((long)240, reg.getDisplayState().displayAreaSize.y);
  reg.setDisplayMode(0x40u);
  EXPECT_EQ((unsigned long)1 << bitOffset_displayAreaWidth1() | (unsigned long)SmpteStandard::ntsc,
            (reg.getStatusControlRegister() & displayModeBits()));
  EXPECT_EQ((long)384, reg.getDisplayState().displayAreaSize.x);
  EXPECT_EQ((long)240, reg.getDisplayState().displayAreaSize.y);

  // heights
  reg.setDisplayMode(0x4u);
  EXPECT_EQ((unsigned long)StatusBits::displayAreaHeight | (unsigned long)SmpteStandard::ntsc,
            (reg.getStatusControlRegister() & displayModeBits()));
  EXPECT_EQ((long)256, reg.getDisplayState().displayAreaSize.x);
  EXPECT_EQ((long)480, reg.getDisplayState().displayAreaSize.y);
  reg.setDisplayMode(0x24u);
  EXPECT_EQ((unsigned long)StatusBits::displayAreaHeight | (unsigned long)StatusBits::verticalInterlacing | (unsigned long)SmpteStandard::ntsc,
            (reg.getStatusControlRegister() & displayModeBits()));
  EXPECT_EQ((long)256, reg.getDisplayState().displayAreaSize.x);
  EXPECT_EQ((long)480, reg.getDisplayState().displayAreaSize.y);
  reg.setDisplayMode(0x2Cu);
  EXPECT_EQ((unsigned long)StatusBits::displayAreaHeight | (unsigned long)StatusBits::verticalInterlacing | (unsigned long)SmpteStandard::pal,
            (reg.getStatusControlRegister() & displayModeBits()));
  EXPECT_EQ((long)256, reg.getDisplayState().displayAreaSize.x);
  EXPECT_EQ((long)512, reg.getDisplayState().displayAreaSize.y);

  // color / reverse
  reg.setDisplayMode(0x10u);
  EXPECT_EQ((unsigned long)StatusBits::colorDepth | (unsigned long)SmpteStandard::ntsc,
            (reg.getStatusControlRegister() & displayModeBits()));
  EXPECT_TRUE(reg.readStatus<bool>(StatusBits::colorDepth));
  reg.setDisplayMode(0x80u);
  EXPECT_EQ((unsigned long)StatusBits::reverseFlag | (unsigned long)SmpteStandard::ntsc,
    (reg.getStatusControlRegister() & displayModeBits()));
  EXPECT_TRUE(reg.readStatus<bool>(StatusBits::reverseFlag));
}

TEST_F(StatusRegisterTest, statusGp1DisplayAreaTest) {
  StatusRegister reg;
  // texture disable
  reg.allowTextureDisable(0x1);
  EXPECT_FALSE(reg.readStatus<bool>(StatusBits::disableTextures));
  EXPECT_FALSE(reg.areTexturesDisabled());
  reg.setTexturePageMode(0x800);
  EXPECT_TRUE(reg.readStatus<bool>(StatusBits::disableTextures));
  EXPECT_TRUE(reg.areTexturesDisabled());
  reg.allowTextureDisable(0);
  EXPECT_TRUE(reg.readStatus<bool>(StatusBits::disableTextures));
  EXPECT_FALSE(reg.areTexturesDisabled());
  reg.arcadeTextureDisable(0x504);
  EXPECT_TRUE(reg.readStatus<bool>(StatusBits::disableTextures));
  EXPECT_TRUE(reg.areTexturesDisabled());
  reg.arcadeTextureDisable(0x501);
  EXPECT_TRUE(reg.readStatus<bool>(StatusBits::disableTextures));
  EXPECT_FALSE(reg.areTexturesDisabled());
  reg.setTexturePageMode(0);
  EXPECT_FALSE(reg.readStatus<bool>(StatusBits::disableTextures));
  EXPECT_FALSE(reg.areTexturesDisabled());

  // display origin
  reg.setDisplayAreaOrigin(0x0);
  EXPECT_EQ((long)0, reg.getDisplayState().displayOrigin.x);
  EXPECT_EQ((long)0, reg.getDisplayState().displayOrigin.y);
  reg.setDisplayAreaOrigin(0xFFFFFFFFu);
  EXPECT_EQ((long)0x3FF, reg.getDisplayState().displayOrigin.x);
  EXPECT_EQ((long)0x1FF, reg.getDisplayState().displayOrigin.y);
  reg.setDisplayAreaOrigin(0x3FFu);
  EXPECT_EQ((long)0x3FF, reg.getDisplayState().displayOrigin.x);
  EXPECT_EQ((long)0, reg.getDisplayState().displayOrigin.y);
  reg.setDisplayAreaOrigin(0x0401u);
  EXPECT_EQ((long)0x1, reg.getDisplayState().displayOrigin.x);
  EXPECT_EQ((long)0x1, reg.getDisplayState().displayOrigin.y);

  // display range
  reg.setHorizontalDisplayRange(0x0);
  EXPECT_EQ((long)0, reg.getDisplayState().displayRange.leftX);
  EXPECT_EQ((long)0, reg.getDisplayState().displayRange.rightX);
  reg.setVerticalDisplayRange(0x0);
  EXPECT_EQ((long)0, reg.getDisplayState().displayRange.topY);
  EXPECT_EQ((long)0, reg.getDisplayState().displayRange.bottomY);
  reg.setHorizontalDisplayRange(0xFFFFFFFFu);
  EXPECT_EQ((long)0xFFF, reg.getDisplayState().displayRange.leftX);
  EXPECT_EQ((long)0xFFF, reg.getDisplayState().displayRange.rightX);
  reg.setVerticalDisplayRange(0xFFFFFFFFu);
  EXPECT_EQ((long)0x3FF, reg.getDisplayState().displayRange.topY);
  EXPECT_EQ((long)0x3FF, reg.getDisplayState().displayRange.bottomY);
  reg.setHorizontalDisplayRange(0x900100u);
  EXPECT_EQ((long)0x100, reg.getDisplayState().displayRange.leftX);
  EXPECT_EQ((long)0x900, reg.getDisplayState().displayRange.rightX);
  reg.setVerticalDisplayRange(0x040010u);
  EXPECT_EQ((long)0x10, reg.getDisplayState().displayRange.topY);
  EXPECT_EQ((long)0x100, reg.getDisplayState().displayRange.bottomY);
}

TEST_F(StatusRegisterTest, statusGp0DrawAreaTest) {
  StatusRegister reg;

  // draw area
  reg.setDrawAreaOrigin(0x0);
  EXPECT_EQ((long)0, reg.getDisplayState().drawArea.leftX);
  EXPECT_EQ((long)0, reg.getDisplayState().drawArea.topY);
  reg.setDrawAreaEnd(0x0);
  EXPECT_EQ((long)0, reg.getDisplayState().drawArea.rightX);
  EXPECT_EQ((long)0, reg.getDisplayState().drawArea.bottomY);
  reg.setDrawAreaOrigin(0xFFFFFFFFu);
  EXPECT_EQ((long)0x3FF, reg.getDisplayState().drawArea.leftX);
  EXPECT_EQ((long)0x1FF, reg.getDisplayState().drawArea.topY);
  reg.setDrawAreaEnd(0xFFFFFFFFu);
  EXPECT_EQ((long)0x3FF, reg.getDisplayState().drawArea.rightX);
  EXPECT_EQ((long)0x1FF, reg.getDisplayState().drawArea.bottomY);
  reg.setDrawAreaOrigin(0x03FFu);
  EXPECT_EQ((long)0x3FF, reg.getDisplayState().drawArea.leftX);
  EXPECT_EQ((long)0, reg.getDisplayState().drawArea.topY);
  reg.setDrawAreaEnd(0x7FC00u);
  EXPECT_EQ((long)0, reg.getDisplayState().drawArea.rightX);
  EXPECT_EQ((long)0x1FF, reg.getDisplayState().drawArea.bottomY);
  reg.setDrawAreaOrigin(0x040010u);
  EXPECT_EQ((long)0x10, reg.getDisplayState().drawArea.leftX);
  EXPECT_EQ((long)0x100, reg.getDisplayState().drawArea.topY);
  reg.setDrawAreaEnd(0x060020u);
  EXPECT_EQ((long)0x20, reg.getDisplayState().drawArea.rightX);
  EXPECT_EQ((long)0x180, reg.getDisplayState().drawArea.bottomY);

  // draw offset
  reg.setDrawOffset(0x0);
  EXPECT_EQ((long)0, reg.getDisplayState().drawOffset.x);
  EXPECT_EQ((long)0, reg.getDisplayState().drawOffset.y);
  reg.setDrawOffset(0xFFFFFFFFu);
  EXPECT_EQ((long)-1, reg.getDisplayState().drawOffset.x);
  EXPECT_EQ((long)-1, reg.getDisplayState().drawOffset.y);
  reg.setDrawOffset(0x010040u);
  EXPECT_EQ((long)0x40, reg.getDisplayState().drawOffset.x);
  EXPECT_EQ((long)0x20, reg.getDisplayState().drawOffset.y);
  reg.setDrawOffset(0x0003FFu);
  EXPECT_EQ((long)1023, reg.getDisplayState().drawOffset.x);
  EXPECT_EQ((long)0, reg.getDisplayState().drawOffset.y);
  reg.setDrawOffset(0x1FF800u);
  EXPECT_EQ((long)0, reg.getDisplayState().drawOffset.x);
  EXPECT_EQ((long)1023, reg.getDisplayState().drawOffset.y);
  reg.setDrawOffset(0x200400u);
  EXPECT_EQ((long)-1024, reg.getDisplayState().drawOffset.x);
  EXPECT_EQ((long)-1024, reg.getDisplayState().drawOffset.y);
}

TEST_F(StatusRegisterTest, statusGp0TexturePageTest) {
  StatusRegister reg;
  unsigned long prevStatus = reg.getStatusControlRegister();

  // empty / full
  reg.setTexturePageMode(0);
  EXPECT_EQ(((unsigned long)TextureColorMode::lookupTable4bit | (unsigned long)BlendingMode::mean),
            (reg.getStatusControlRegister() & texturePageBits()));
  EXPECT_EQ((long)0, reg.getTexpageBaseX());
  EXPECT_EQ((long)0, reg.getTexpageBaseY());
  EXPECT_FALSE(reg.isTextureFlipX());
  EXPECT_FALSE(reg.isTextureFlipY());
  EXPECT_EQ((prevStatus & ~(texturePageBits())),
            (reg.getStatusControlRegister() & ~(texturePageBits())));
  reg.setTexturePageMode(0xFFFFFFFFu);
  EXPECT_EQ(((unsigned long)TextureColorMode::directColor15bit | (unsigned long)BlendingMode::addQuarter
          | (unsigned long)StatusBits::drawToDisplay | (unsigned long)StatusBits::dithering | (unsigned long)StatusBits::disableTextures
          | (unsigned long)StatusBits::texturePageBaseX | (unsigned long)StatusBits::texturePageBaseY),
            (reg.getStatusControlRegister() & texturePageBits()));
  EXPECT_EQ((long)0x3C0, reg.getTexpageBaseX());
  EXPECT_EQ((long)0x100, reg.getTexpageBaseY());
  EXPECT_TRUE(reg.isTextureFlipX());
  EXPECT_TRUE(reg.isTextureFlipY());
  EXPECT_EQ((prevStatus & ~(texturePageBits())),
            (reg.getStatusControlRegister() & ~(texturePageBits())));

  // texture page base
  reg.setTexturePageMode(0x02u);
  EXPECT_EQ(((unsigned long)TextureColorMode::lookupTable4bit | (unsigned long)BlendingMode::mean | (unsigned long)0x2),
            (reg.getStatusControlRegister() & texturePageBits()));
  EXPECT_EQ((long)0x80, reg.getTexpageBaseX());
  EXPECT_EQ((long)0, reg.getTexpageBaseY());
  EXPECT_FALSE(reg.isTextureFlipX());
  EXPECT_FALSE(reg.isTextureFlipY());
  reg.setTexturePageMode(0x14u);
  EXPECT_EQ(((unsigned long)TextureColorMode::lookupTable4bit | (unsigned long)BlendingMode::mean
           | (unsigned long)0x4 | (unsigned long)StatusBits::texturePageBaseY),
             (reg.getStatusControlRegister() & texturePageBits()));
  EXPECT_EQ((long)0x100, reg.getTexpageBaseX());
  EXPECT_EQ((long)0x100, reg.getTexpageBaseY());
  EXPECT_FALSE(reg.isTextureFlipX());
  EXPECT_FALSE(reg.isTextureFlipY());

  // drawing / color / dithering / blending
  reg.setTexturePageMode(0x480u);
  EXPECT_EQ(((unsigned long)TextureColorMode::lookupTable8bit | (unsigned long)BlendingMode::mean
          | (unsigned long)StatusBits::drawToDisplay),
            (reg.getStatusControlRegister() & texturePageBits()));
  EXPECT_EQ((long)0, reg.getTexpageBaseX());
  EXPECT_EQ((long)0, reg.getTexpageBaseY());
  EXPECT_FALSE(reg.isTextureFlipX());
  EXPECT_FALSE(reg.isTextureFlipY());
  reg.setTexturePageMode(0x320u);
  EXPECT_EQ(((unsigned long)TextureColorMode::directColor15bit | (unsigned long)BlendingMode::add
          | (unsigned long)StatusBits::dithering),
            (reg.getStatusControlRegister() & texturePageBits()));
  EXPECT_EQ((long)0, reg.getTexpageBaseX());
  EXPECT_EQ((long)0, reg.getTexpageBaseY());
  EXPECT_FALSE(reg.isTextureFlipX());
  EXPECT_FALSE(reg.isTextureFlipY());
  reg.setTexturePageMode(0x640u);
  EXPECT_EQ(((unsigned long)TextureColorMode::lookupTable4bit | (unsigned long)BlendingMode::subtract
          | (unsigned long)StatusBits::drawToDisplay | (unsigned long)StatusBits::dithering),
            (reg.getStatusControlRegister() & texturePageBits()));
  EXPECT_EQ((long)0, reg.getTexpageBaseX());
  EXPECT_EQ((long)0, reg.getTexpageBaseY());
  EXPECT_FALSE(reg.isTextureFlipX());
  EXPECT_FALSE(reg.isTextureFlipY());
  reg.setTexturePageMode(0x1E0u);
  EXPECT_EQ(((unsigned long)TextureColorMode::directColor15bit | (unsigned long)BlendingMode::addQuarter),
            (reg.getStatusControlRegister() & texturePageBits()));
  EXPECT_EQ((long)0, reg.getTexpageBaseX());
  EXPECT_EQ((long)0, reg.getTexpageBaseY());
  EXPECT_FALSE(reg.isTextureFlipX());
  EXPECT_FALSE(reg.isTextureFlipY());

  // sprite texture flipping
  reg.setTexturePageMode(0x1000u);
  EXPECT_EQ(((unsigned long)TextureColorMode::lookupTable4bit | (unsigned long)BlendingMode::mean),
            (reg.getStatusControlRegister() & texturePageBits()));
  EXPECT_EQ((long)0, reg.getTexpageBaseX());
  EXPECT_EQ((long)0, reg.getTexpageBaseY());
  EXPECT_TRUE(reg.isTextureFlipX());
  EXPECT_FALSE(reg.isTextureFlipY());
  reg.setTexturePageMode(0x2000u);
  EXPECT_EQ(((unsigned long)TextureColorMode::lookupTable4bit | (unsigned long)BlendingMode::mean),
            (reg.getStatusControlRegister() & texturePageBits()));
  EXPECT_EQ((long)0, reg.getTexpageBaseX());
  EXPECT_EQ((long)0, reg.getTexpageBaseY());
  EXPECT_FALSE(reg.isTextureFlipX());
  EXPECT_TRUE(reg.isTextureFlipY());
  reg.setTexturePageMode(0x3000u);
  EXPECT_EQ(((unsigned long)TextureColorMode::lookupTable4bit | (unsigned long)BlendingMode::mean),
            (reg.getStatusControlRegister() & texturePageBits()));
  EXPECT_EQ((long)0, reg.getTexpageBaseX());
  EXPECT_EQ((long)0, reg.getTexpageBaseY());
  EXPECT_TRUE(reg.isTextureFlipX());
  EXPECT_TRUE(reg.isTextureFlipY());
}

TEST_F(StatusRegisterTest, statusGp0TextureWindowTest) {
  StatusRegister reg;

  reg.setTextureWindow(0);
  EXPECT_EQ((long)256, reg.getTextureWindow().maskWidth);
  EXPECT_EQ((long)256, reg.getTextureWindow().maskHeight);
  EXPECT_EQ((long)0, reg.getTextureWindow().offsetX);
  EXPECT_EQ((long)0, reg.getTextureWindow().offsetY);
  EXPECT_FALSE(reg.getTextureWindow().isEnabled);
  reg.setTextureWindow(0xFFC00u);
  EXPECT_EQ((long)256, reg.getTextureWindow().maskWidth);
  EXPECT_EQ((long)256, reg.getTextureWindow().maskHeight);
  EXPECT_EQ((long)0, reg.getTextureWindow().offsetX);
  EXPECT_EQ((long)0, reg.getTextureWindow().offsetY);
  EXPECT_FALSE(reg.getTextureWindow().isEnabled);
  reg.setTextureWindow(0x3FFu);
  EXPECT_EQ((long)8, reg.getTextureWindow().maskWidth);
  EXPECT_EQ((long)8, reg.getTextureWindow().maskHeight);
  EXPECT_EQ((long)0, reg.getTextureWindow().offsetX);
  EXPECT_EQ((long)0, reg.getTextureWindow().offsetY);
  EXPECT_TRUE(reg.getTextureWindow().isEnabled);
  reg.setTextureWindow(0xFFFFFFFFu);
  EXPECT_EQ((long)8, reg.getTextureWindow().maskWidth);
  EXPECT_EQ((long)8, reg.getTextureWindow().maskHeight);
  EXPECT_EQ((long)248, reg.getTextureWindow().offsetX);
  EXPECT_EQ((long)248, reg.getTextureWindow().offsetY);
  EXPECT_TRUE(reg.getTextureWindow().isEnabled);
  
  reg.setTextureWindow(0xFFC21u);
  EXPECT_EQ((long)8, reg.getTextureWindow().maskWidth);
  EXPECT_EQ((long)8, reg.getTextureWindow().maskHeight);
  EXPECT_EQ((long)248, reg.getTextureWindow().offsetX);
  EXPECT_EQ((long)248, reg.getTextureWindow().offsetY);
  EXPECT_TRUE(reg.getTextureWindow().isEnabled);
  reg.setTextureWindow(0xFFC02u);
  EXPECT_EQ((long)16, reg.getTextureWindow().maskWidth);
  EXPECT_EQ((long)256, reg.getTextureWindow().maskHeight);
  EXPECT_EQ((long)240, reg.getTextureWindow().offsetX);
  EXPECT_EQ((long)0, reg.getTextureWindow().offsetY);
  EXPECT_TRUE(reg.getTextureWindow().isEnabled);
  reg.setTextureWindow(0xFFC44u);
  EXPECT_EQ((long)32, reg.getTextureWindow().maskWidth);
  EXPECT_EQ((long)16, reg.getTextureWindow().maskHeight);
  EXPECT_EQ((long)224, reg.getTextureWindow().offsetX);
  EXPECT_EQ((long)240, reg.getTextureWindow().offsetY);
  EXPECT_TRUE(reg.getTextureWindow().isEnabled);
  reg.setTextureWindow(0xFFC30u);
  EXPECT_EQ((long)128, reg.getTextureWindow().maskWidth);
  EXPECT_EQ((long)8, reg.getTextureWindow().maskHeight);
  EXPECT_EQ((long)128, reg.getTextureWindow().offsetX);
  EXPECT_EQ((long)248, reg.getTextureWindow().offsetY);
  EXPECT_TRUE(reg.getTextureWindow().isEnabled);
  reg.setTextureWindow(0xFFD08u);
  EXPECT_EQ((long)64, reg.getTextureWindow().maskWidth);
  EXPECT_EQ((long)64, reg.getTextureWindow().maskHeight);
  EXPECT_EQ((long)192, reg.getTextureWindow().offsetX);
  EXPECT_EQ((long)192, reg.getTextureWindow().offsetY);
  EXPECT_TRUE(reg.getTextureWindow().isEnabled);

  reg.setTextureWindow(0x8421u);
  EXPECT_EQ((long)8, reg.getTextureWindow().maskWidth);
  EXPECT_EQ((long)8, reg.getTextureWindow().maskHeight);
  EXPECT_EQ((long)8, reg.getTextureWindow().offsetX);
  EXPECT_EQ((long)8, reg.getTextureWindow().offsetY);
  EXPECT_TRUE(reg.getTextureWindow().isEnabled);
  reg.setTextureWindow(0x11021u);
  EXPECT_EQ((long)8, reg.getTextureWindow().maskWidth);
  EXPECT_EQ((long)8, reg.getTextureWindow().maskHeight);
  EXPECT_EQ((long)32, reg.getTextureWindow().offsetX);
  EXPECT_EQ((long)16, reg.getTextureWindow().offsetY);
  EXPECT_TRUE(reg.getTextureWindow().isEnabled);
  reg.setTextureWindow(0x40821u);
  EXPECT_EQ((long)8, reg.getTextureWindow().maskWidth);
  EXPECT_EQ((long)8, reg.getTextureWindow().maskHeight);
  EXPECT_EQ((long)16, reg.getTextureWindow().offsetX);
  EXPECT_EQ((long)64, reg.getTextureWindow().offsetY);
  EXPECT_TRUE(reg.getTextureWindow().isEnabled);
  reg.setTextureWindow(0x84021u);
  EXPECT_EQ((long)8, reg.getTextureWindow().maskWidth);
  EXPECT_EQ((long)8, reg.getTextureWindow().maskHeight);
  EXPECT_EQ((long)128, reg.getTextureWindow().offsetX);
  EXPECT_EQ((long)128, reg.getTextureWindow().offsetY);
  EXPECT_TRUE(reg.getTextureWindow().isEnabled);
  reg.setTextureWindow(0xE4821u);
  EXPECT_EQ((long)8, reg.getTextureWindow().maskWidth);
  EXPECT_EQ((long)8, reg.getTextureWindow().maskHeight);
  EXPECT_EQ((long)144, reg.getTextureWindow().offsetX);
  EXPECT_EQ((long)224, reg.getTextureWindow().offsetY);
  EXPECT_TRUE(reg.getTextureWindow().isEnabled);
  reg.setTextureWindow(0xF7821u);
  EXPECT_EQ((long)8, reg.getTextureWindow().maskWidth);
  EXPECT_EQ((long)8, reg.getTextureWindow().maskHeight);
  EXPECT_EQ((long)240, reg.getTextureWindow().offsetX);
  EXPECT_EQ((long)240, reg.getTextureWindow().offsetY);
  EXPECT_TRUE(reg.getTextureWindow().isEnabled);

  reg.setTextureWindow(0x11082u);
  EXPECT_EQ((long)16, reg.getTextureWindow().maskWidth);
  EXPECT_EQ((long)32, reg.getTextureWindow().maskHeight);
  EXPECT_EQ((long)32, reg.getTextureWindow().offsetX);
  EXPECT_EQ((long)0, reg.getTextureWindow().offsetY);
  EXPECT_TRUE(reg.getTextureWindow().isEnabled);
  reg.setTextureWindow(0xF7882u);
  EXPECT_EQ((long)16, reg.getTextureWindow().maskWidth);
  EXPECT_EQ((long)32, reg.getTextureWindow().maskHeight);
  EXPECT_EQ((long)240, reg.getTextureWindow().offsetX);
  EXPECT_EQ((long)224, reg.getTextureWindow().offsetY);
  EXPECT_TRUE(reg.getTextureWindow().isEnabled);
}

TEST_F(StatusRegisterTest, statusGp0MaskBitTest) {
  StatusRegister reg;

  reg.setMaskBit(0);
  EXPECT_FALSE(reg.readStatus<bool>(StatusBits::forceSetMaskBit));
  EXPECT_FALSE(reg.readStatus<bool>(StatusBits::enableMask));
  reg.setMaskBit(0xFFFFFFFFu);
  EXPECT_TRUE(reg.readStatus<bool>(StatusBits::forceSetMaskBit));
  EXPECT_TRUE(reg.readStatus<bool>(StatusBits::enableMask));
  reg.setMaskBit(1u);
  EXPECT_TRUE(reg.readStatus<bool>(StatusBits::forceSetMaskBit));
  EXPECT_FALSE(reg.readStatus<bool>(StatusBits::enableMask));
  reg.setMaskBit(2u);
  EXPECT_FALSE(reg.readStatus<bool>(StatusBits::forceSetMaskBit));
  EXPECT_TRUE(reg.readStatus<bool>(StatusBits::enableMask));
  reg.setMaskBit(3u);
  EXPECT_TRUE(reg.readStatus<bool>(StatusBits::forceSetMaskBit));
  EXPECT_TRUE(reg.readStatus<bool>(StatusBits::enableMask));
}
