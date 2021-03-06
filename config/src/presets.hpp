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

static __forceinline void __loadDefaultConfig(RendererProfile& outRendererCfg) noexcept {
  outRendererCfg.screenStretching = 0;
  outRendererCfg.screenCropping = 0;
  outRendererCfg.isCenterX = false;
  outRendererCfg.isCenterY = true;
  outRendererCfg.isOverscanVisible = false;
  outRendererCfg.isMirrored = false;
  outRendererCfg.screenCurvature = 0;
  memset(outRendererCfg.blackBorderSizes, 0, 4*sizeof(*outRendererCfg.blackBorderSizes));
  
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

  outRendererCfg.textureGrain = NoiseFilter::none;
  outRendererCfg.screenGrain = NoiseFilter::none;
  outRendererCfg.dithering = ColorDithering::none;
  outRendererCfg.useTextureDithering = false;
  outRendererCfg.useSpriteDithering = false;
}

// ---

static __forceinline void __loadPsxAccurate(RendererProfile& outRendererCfg) noexcept {
  outRendererCfg.screenStretching = 0;
  outRendererCfg.screenCropping = 0;
  outRendererCfg.isCenterX = false;
  outRendererCfg.isCenterY = false;
  outRendererCfg.isOverscanVisible = false;
  outRendererCfg.isMirrored = false;
  outRendererCfg.screenCurvature = 0;
  memset(outRendererCfg.blackBorderSizes, 0, 4*sizeof(*outRendererCfg.blackBorderSizes));
  
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

  outRendererCfg.textureGrain = NoiseFilter::none;
  outRendererCfg.screenGrain = NoiseFilter::none;
  outRendererCfg.dithering = ColorDithering::none;
  outRendererCfg.useTextureDithering = false;
  outRendererCfg.useSpriteDithering = false;
}

// ---

static __forceinline void __loadRealistic2D(RendererProfile& outRendererCfg) noexcept {
  outRendererCfg.screenStretching = 0;
  outRendererCfg.screenCropping = 2;
  outRendererCfg.isCenterX = false;
  outRendererCfg.isCenterY = true;
  outRendererCfg.isOverscanVisible = false;
  outRendererCfg.isMirrored = false;
  outRendererCfg.screenCurvature = 0;
  memset(outRendererCfg.blackBorderSizes, 0, 4*sizeof(*outRendererCfg.blackBorderSizes));
  
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

  outRendererCfg.textureGrain = NoiseFilter::none;
  outRendererCfg.screenGrain = NoiseFilter::none;
  outRendererCfg.dithering = ColorDithering::none;
  outRendererCfg.useTextureDithering = false;
  outRendererCfg.useSpriteDithering = false;
}

// ---

static __forceinline void __loadRealistic3D(RendererProfile& outRendererCfg) noexcept {
  outRendererCfg.screenStretching = 4;
  outRendererCfg.screenCropping = 4;
  outRendererCfg.isCenterX = true;
  outRendererCfg.isCenterY = true;
  outRendererCfg.isOverscanVisible = false;
  outRendererCfg.isMirrored = false;
  outRendererCfg.screenCurvature = 0;
  memset(outRendererCfg.blackBorderSizes, 0, 4*sizeof(*outRendererCfg.blackBorderSizes));

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

  outRendererCfg.textureGrain = NoiseFilter::grain;
  outRendererCfg.screenGrain = NoiseFilter::none;
  outRendererCfg.dithering = ColorDithering::none;
  outRendererCfg.useTextureDithering = false;
  outRendererCfg.useSpriteDithering = false;
}

// ---

static __forceinline void __loadCartoon(RendererProfile& outRendererCfg) noexcept {
  outRendererCfg.screenStretching = 0;
  outRendererCfg.screenCropping = 2;
  outRendererCfg.isCenterX = false;
  outRendererCfg.isCenterY = true;
  outRendererCfg.isOverscanVisible = false;
  outRendererCfg.isMirrored = false;
  outRendererCfg.screenCurvature = 0;
  memset(outRendererCfg.blackBorderSizes, 0, 4*sizeof(*outRendererCfg.blackBorderSizes));

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

  outRendererCfg.textureGrain = NoiseFilter::none;
  outRendererCfg.screenGrain = NoiseFilter::none;
  outRendererCfg.dithering = ColorDithering::none;
  outRendererCfg.useTextureDithering = false;
  outRendererCfg.useSpriteDithering = true;
}


// -- preset selection --

void config::loadPreset(PresetId id, RendererProfile& outRendererCfg) noexcept {
  switch (id) {
    case PresetId::psxAccurate:   __loadPsxAccurate(outRendererCfg); break;
    case PresetId::realistic2D:   __loadRealistic2D(outRendererCfg); break;
    case PresetId::realistic3D:   __loadRealistic3D(outRendererCfg); break;
    case PresetId::cartoon:       __loadCartoon(outRendererCfg); break;
    case PresetId::defaultConfig:
    default: __loadDefaultConfig(outRendererCfg); break;
  }
}
