/*******************************************************************************
Pandora GS - PSEmu-compatible GPU driver
Copyright (C) 2023  Romain Vinders

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

namespace menu {
  enum class InterpolationType : uint32_t { ///< Minification/magnification interpolation types
    nearest = 0, ///< Nearest pixel, no filtering
    bilinear,    ///< Smooth, diamond artifacts, fast
    gaussian,    ///< Smoothest, blurry, slower
    bessel,      ///< Smooth/rounder, slower
    mitchell,    ///< Smooth/rounder, less blurry, slower
    lanczos,     ///< Sharper/rounder, keeps color vibrance, ring artifacts, slower
    COUNT
  };
  enum class UpscalingType : int { ///< Upscaling filters for textures/sprites/screen
    none = 0, ///< Nearest pixel: fastest (no upscaling)
    jinc,     ///< Jinc: smooth filter, great for 2D games and cinematics (average aliasing)
    SaI,      ///< SaI: fast and sharp, but high aliasing
    SABR,     ///< SABR: smoother than xBR/SaI, great for 2D games, causes color banding
    xBRZ,     ///< xBRZ: very sharp, excellent edge detection, low aliasing, causes color banding (improved curves/details compared to xBR)
    scaleFX,  ///< scaleFX: sharp, excellent edge and curve detection, lowest aliasing, causes color banding
    super_xBR,///< super-xBR-fast-bilateral: xBR sharp edges combined with smoother gradients (low aliasing)
    COUNT
  };
  enum class MdecFilter : int { ///< Upscaling filters for MDEC videos
    nearest = 0,
    bilinear,  ///< Smooth/blurry standard filter
    lanczos,   ///< Lanczos: smooth filter with improved/rounded gradients (average aliasing, ringing effect)
    nnedi3,    ///< NNEDI3: sharp upscaling filter for realistic cinematics (not too sharp, low aliasing, no ringing, slower)
    sabr,      ///< SABR: smoother than xBR/SaI, great for cartoon or semi-realistic cinematics
    super_xBR, ///< Super-xBR: sharp edge upscaling combined with linear gradient for cartoon cinematics
    COUNT
  };
  enum class AntiAliasing : int { ///< Anti-aliasing methods
    none = 0,
    fxaa, ///< Fast-approximate: blurry, very fast
    nfaa, ///< Normal filter: very good with 2D, fast
    smaa2, ///< Subpixel morph 2x: faster than MSAA, great results (if supported)
    smaa4, ///< Subpixel morph 4x: faster than MSAA, great results (if supported)
    smaa8, ///< Subpixel morph 8x: faster than MSAA, great results (if supported)
    msaa2, ///< Multi-sample 2x: great results, slow
    msaa4, ///< Multi-sample 2x: great results, slow
    msaa8, ///< Multi-sample 2x: great results, very slow
    COUNT
  };
}
