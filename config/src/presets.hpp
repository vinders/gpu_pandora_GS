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
// this implementation is included in config.cpp -> single object file (less overhead + smaller lib size)
#include <system/force_inline.h>
#include "config/presets.h"

using namespace config;


// -- presets --

static __forceinline void __loadDefaultConfig(RendererProfile& outRendererCfg, WindowProfile& outWindowCfg,
                                              EffectsProfile& outEffectsCfg) noexcept {
  outRendererCfg.internalResFactorX = 4;
  outRendererCfg.internalResFactorY = 4;
  outRendererCfg.colorMode = ColorOutput::rgb32;
  outRendererCfg.fillMode = FillMode::normal;
  outRendererCfg.antiAliasing = AntiAliasing::none;

  outRendererCfg.textureUpscaling = UpscalingFilter::lanczos;
  outRendererCfg.textureUpscalingFactor = 4;
  outRendererCfg.useTextureBilinear = true;
  outRendererCfg.spriteUpscaling = UpscalingFilter::lanczos;
  outRendererCfg.spriteUpscalingFactor = 4;
  outRendererCfg.useSpriteBilinear = false;
  outRendererCfg.screenUpscaling = UpscalingFilter::none;
  outRendererCfg.screenUpscalingFactor = 1;
  outRendererCfg.mdecUpscaling = MdecFilter::bilinear;

  outWindowCfg.screenStretching = 0;
  outWindowCfg.screenCropping = 0;
  outWindowCfg.screenCurvature = 0;
  outWindowCfg.isMirrored = false;
  outWindowCfg.isPalRecentered = true;
  outWindowCfg.isOverscanVisible = false;
  memset(outWindowCfg.blackBorderSizes, 0, 4*sizeof(*outWindowCfg.blackBorderSizes));

  outEffectsCfg.textureGrain = NoiseFilter::none;
  outEffectsCfg.screenGrain = NoiseFilter::none;
  outEffectsCfg.dithering = ColorDithering::none;
  outEffectsCfg.useTextureDithering = false;
  outEffectsCfg.useSpriteDithering = false;
}

// ---

static __forceinline void __loadPsxAccurate(RendererProfile& outRendererCfg, WindowProfile& outWindowCfg,
                                            EffectsProfile& outEffectsCfg) noexcept {
  outRendererCfg.internalResFactorX = 1;
  outRendererCfg.internalResFactorY = 1;
  outRendererCfg.colorMode = ColorOutput::rgb16;
  outRendererCfg.fillMode = FillMode::normal;
  outRendererCfg.antiAliasing = AntiAliasing::none;

  outRendererCfg.textureUpscaling = UpscalingFilter::none;
  outRendererCfg.textureUpscalingFactor = 1;
  outRendererCfg.useTextureBilinear = false;
  outRendererCfg.spriteUpscaling = UpscalingFilter::none;
  outRendererCfg.spriteUpscalingFactor = 1;
  outRendererCfg.useSpriteBilinear = false;
  outRendererCfg.screenUpscaling = UpscalingFilter::none;
  outRendererCfg.screenUpscalingFactor = 1;
  outRendererCfg.mdecUpscaling = MdecFilter::none;

  outWindowCfg.screenStretching = 0;
  outWindowCfg.screenCropping = 0;
  outWindowCfg.screenCurvature = 0;
  outWindowCfg.isMirrored = false;
  outWindowCfg.isPalRecentered = false;
  outWindowCfg.isOverscanVisible = false;
  memset(outWindowCfg.blackBorderSizes, 0, 4*sizeof(*outWindowCfg.blackBorderSizes));

  outEffectsCfg.textureGrain = NoiseFilter::none;
  outEffectsCfg.screenGrain = NoiseFilter::none;
  outEffectsCfg.dithering = ColorDithering::none;
  outEffectsCfg.useTextureDithering = false;
  outEffectsCfg.useSpriteDithering = false;
}

// ---

static __forceinline void __loadRealistic2D(RendererProfile& outRendererCfg, WindowProfile& outWindowCfg,
                                            EffectsProfile& outEffectsCfg) noexcept {
  outRendererCfg.internalResFactorX = 2;
  outRendererCfg.internalResFactorY = 2;
  outRendererCfg.colorMode = ColorOutput::rgb32;
  outRendererCfg.fillMode = FillMode::normal;
  outRendererCfg.antiAliasing = AntiAliasing::nfaa;

  outRendererCfg.textureUpscaling = UpscalingFilter::lanczos;
  outRendererCfg.textureUpscalingFactor = 2;
  outRendererCfg.useTextureBilinear = false;
  outRendererCfg.spriteUpscaling = UpscalingFilter::jinc2;
  outRendererCfg.spriteUpscalingFactor = 2;
  outRendererCfg.useSpriteBilinear = false;
  outRendererCfg.screenUpscaling = UpscalingFilter::lanczos;
  outRendererCfg.screenUpscalingFactor = 4;
  outRendererCfg.mdecUpscaling = MdecFilter::jinc2;

  outWindowCfg.screenStretching = 0;
  outWindowCfg.screenCropping = 2;
  outWindowCfg.screenCurvature = 0;
  outWindowCfg.isMirrored = false;
  outWindowCfg.isPalRecentered = true;
  outWindowCfg.isOverscanVisible = false;
  memset(outWindowCfg.blackBorderSizes, 0, 4*sizeof(*outWindowCfg.blackBorderSizes));

  outEffectsCfg.textureGrain = NoiseFilter::none;
  outEffectsCfg.screenGrain = NoiseFilter::none;
  outEffectsCfg.dithering = ColorDithering::none;
  outEffectsCfg.useTextureDithering = false;
  outEffectsCfg.useSpriteDithering = false;
}

// ---

static __forceinline void __loadRealistic3D(RendererProfile& outRendererCfg, WindowProfile& outWindowCfg,
                                            EffectsProfile& outEffectsCfg) noexcept {
  outRendererCfg.internalResFactorX = 4;
  outRendererCfg.internalResFactorY = 4;
  outRendererCfg.colorMode = ColorOutput::rgb32;
  outRendererCfg.fillMode = FillMode::normal;
  outRendererCfg.antiAliasing = AntiAliasing::fxaa;

  outRendererCfg.textureUpscaling = UpscalingFilter::lanczos;
  outRendererCfg.textureUpscalingFactor = 4;
  outRendererCfg.useTextureBilinear = true;
  outRendererCfg.spriteUpscaling = UpscalingFilter::super_xBR;
  outRendererCfg.spriteUpscalingFactor = 4;
  outRendererCfg.useSpriteBilinear = true;
  outRendererCfg.screenUpscaling = UpscalingFilter::jinc2;
  outRendererCfg.screenUpscalingFactor = 2;
  outRendererCfg.mdecUpscaling = MdecFilter::jinc2;

  outWindowCfg.screenStretching = 4;
  outWindowCfg.screenCropping = 4;
  outWindowCfg.screenCurvature = 0;
  outWindowCfg.isMirrored = false;
  outWindowCfg.isPalRecentered = true;
  outWindowCfg.isOverscanVisible = false;
  memset(outWindowCfg.blackBorderSizes, 0, 4*sizeof(*outWindowCfg.blackBorderSizes));

  outEffectsCfg.textureGrain = NoiseFilter::grain;
  outEffectsCfg.screenGrain = NoiseFilter::none;
  outEffectsCfg.dithering = ColorDithering::none;
  outEffectsCfg.useTextureDithering = false;
  outEffectsCfg.useSpriteDithering = false;
}

// ---

static __forceinline void __loadCartoon(RendererProfile& outRendererCfg, WindowProfile& outWindowCfg,
                                          EffectsProfile& outEffectsCfg) noexcept {
  outRendererCfg.internalResFactorX = 4;
  outRendererCfg.internalResFactorY = 4;
  outRendererCfg.colorMode = ColorOutput::rgb32;
  outRendererCfg.fillMode = FillMode::normal;
  outRendererCfg.antiAliasing = AntiAliasing::nfaa;

  outRendererCfg.textureUpscaling = UpscalingFilter::xBR;
  outRendererCfg.textureUpscalingFactor = 4;
  outRendererCfg.useTextureBilinear = false;
  outRendererCfg.spriteUpscaling = UpscalingFilter::xBRZ;
  outRendererCfg.spriteUpscalingFactor = 4;
  outRendererCfg.useSpriteBilinear = false;
  outRendererCfg.screenUpscaling = UpscalingFilter::none;
  outRendererCfg.screenUpscalingFactor = 1;
  outRendererCfg.mdecUpscaling = MdecFilter::super_xBR;

  outWindowCfg.screenStretching = 0;
  outWindowCfg.screenCropping = 2;
  outWindowCfg.screenCurvature = 0;
  outWindowCfg.isMirrored = false;
  outWindowCfg.isPalRecentered = true;
  outWindowCfg.isOverscanVisible = false;
  memset(outWindowCfg.blackBorderSizes, 0, 4*sizeof(*outWindowCfg.blackBorderSizes));

  outEffectsCfg.textureGrain = NoiseFilter::none;
  outEffectsCfg.screenGrain = NoiseFilter::none;
  outEffectsCfg.dithering = ColorDithering::none;
  outEffectsCfg.useTextureDithering = false;
  outEffectsCfg.useSpriteDithering = true;
}


// -- preset selection --

void config::loadPreset(PresetId id, RendererProfile& outRendererCfg,
                        WindowProfile& outWindowCfg, EffectsProfile& outEffectsCfg) noexcept {
  switch (id) {
    case PresetId::psxAccurate:   __loadPsxAccurate(outRendererCfg, outWindowCfg, outEffectsCfg); break;
    case PresetId::realistic2D:   __loadRealistic2D(outRendererCfg, outWindowCfg, outEffectsCfg); break;
    case PresetId::realistic3D:   __loadRealistic3D(outRendererCfg, outWindowCfg, outEffectsCfg); break;
    case PresetId::cartoon:       __loadCartoon(outRendererCfg, outWindowCfg, outEffectsCfg); break;
    case PresetId::defaultConfig:
    default: __loadDefaultConfig(outRendererCfg, outWindowCfg, outEffectsCfg); break;
  }
}
