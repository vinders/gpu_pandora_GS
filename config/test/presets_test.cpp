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


static bool __isProfileConfigEqual(const RendererProfile& r1, const RendererProfile& r2,
                                   const WindowProfile& w1, const WindowProfile& w2,
                                   const EffectsProfile& e1, const EffectsProfile& e2) {
  return  (r1.internalResFactorX == r2.internalResFactorX
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

        && w1.screenStretching == w2.screenStretching
        && w1.screenCropping == w2.screenCropping
        && w1.screenCurvature == w2.screenCurvature
        && w1.isMirrored == w2.isMirrored
        && w1.isPalRecentered == w2.isPalRecentered
        && w1.isOverscanVisible == w2.isOverscanVisible

        && e1.screenGrain == e2.screenGrain
        && e1.textureGrain == e2.textureGrain
        && e1.dithering == e2.dithering
        && e1.useTextureDithering == e2.useTextureDithering
        && e1.useSpriteDithering == e2.useSpriteDithering);
}

// ---

TEST_F(PresetsTest, applyPresets) {
  RendererProfile defRendererCfg, rendererCfg;
  WindowProfile defWindowCfg, windowCfg;
  EffectsProfile defEffectsCfg, effectsCfg;
  
  loadPreset(PresetId::defaultConfig, rendererCfg, windowCfg, effectsCfg);
  EXPECT_TRUE(__isProfileConfigEqual(defRendererCfg, rendererCfg, defWindowCfg, windowCfg, defEffectsCfg, effectsCfg));
  
  for (int i = ((int)PresetId::defaultConfig) + 1; i < (int)__CONFIG_LAST_PRESET_ID; ++i) {
    loadPreset((PresetId)i, rendererCfg, windowCfg, effectsCfg);
    EXPECT_FALSE(__isProfileConfigEqual(defRendererCfg, rendererCfg, defWindowCfg, windowCfg, defEffectsCfg, effectsCfg));
  }
}
