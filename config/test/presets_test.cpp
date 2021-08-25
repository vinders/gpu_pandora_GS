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
#include <config/presets.h>

using namespace config;

class PresetsTest : public testing::Test {
public:
protected:
  //static void SetUpTestCase() {}
  //static void TearDownTestCase() {}

  void SetUp() override {}
  void TearDown() override {}
};


static bool __isProfileConfigEqual(const RendererProfile& r1, const RendererProfile& r2) {
  return  (r1.screenStretching == r2.screenStretching
        && r1.screenCropping == r2.screenCropping
        && r1.isCenterX == r2.isCenterX
        && r1.isCenterY == r2.isCenterY
        && r1.isOverscanVisible == r2.isOverscanVisible
        && r1.isMirrored == r2.isMirrored
        && r1.screenCurvature == r2.screenCurvature
        
        && r1.internalResFactorX == r2.internalResFactorX
        && r1.internalResFactorY == r2.internalResFactorY
        && r1.colorMode == r2.colorMode
        && r1.fillMode == r2.fillMode
        && r1.antiAliasing == r2.antiAliasing
        && r1.textureUpscaling == r2.textureUpscaling
        && r1.textureUpscalingFactor == r2.textureUpscalingFactor
        && r1.useTextureBilinear == r2.useTextureBilinear
        && r1.spriteUpscaling == r2.spriteUpscaling
        && r1.spriteUpscalingFactor == r2.spriteUpscalingFactor
        && r1.useSpriteBilinear == r2.useSpriteBilinear
        && r1.screenUpscaling == r2.screenUpscaling
        && r1.screenUpscalingFactor == r2.screenUpscalingFactor
        && r1.mdecUpscaling == r2.mdecUpscaling
        
        && r1.screenGrain == r2.screenGrain
        && r1.textureGrain == r2.textureGrain
        && r1.dithering == r2.dithering
        && r1.useTextureDithering == r2.useTextureDithering
        && r1.useSpriteDithering == r2.useSpriteDithering);
}

// ---

TEST_F(PresetsTest, applyPresets) {
  RendererProfile defRendererCfg, rendererCfg;
  loadPreset(PresetId::defaultConfig, rendererCfg);
  EXPECT_TRUE(__isProfileConfigEqual(defRendererCfg, rendererCfg));
  
  for (int i = ((int)PresetId::defaultConfig) + 1; i < (int)__CONFIG_MAX_PRESET_ID; ++i) {
    loadPreset((PresetId)i, rendererCfg);
    EXPECT_FALSE(__isProfileConfigEqual(defRendererCfg, rendererCfg));
  }

  EXPECT_TRUE(isPresetId((ProfileId)(__CONFIG_PRESET_FLAG | 1)));
  EXPECT_FALSE(isPresetId((ProfileId)1));
}
