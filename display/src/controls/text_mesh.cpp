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
#include <cstring>
#include "display/controls/text_mesh.h"

using namespace video_api;
using namespace display;
using namespace display::controls;


TextMesh::TextMesh(Renderer& renderer, Font& font, const char16_t* text,
                   const float pxSizeX, const float pxSizeY, int32_t x, int32_t y, TextAlignment align)
  : x_(x),
    y_(y),
    height_(font.XHeight()) {
  if (text == nullptr || *text == (char16_t)0)
    return;
  double currentX = (double)x;
  const float baseVertexY = ToVertexPositionY(y + (int32_t)height_, pxSizeY);
  uint32_t glyphFirstIndex = 0;
  
  for (; *text; ++text) {
    const auto& glyph = font.getGlyph(renderer, (char32_t)*text);
    if (!glyph.texture.isEmpty()) {
      const float left = ToVertexPositionX((float)currentX + glyph.offsetLeft, pxSizeX);
      const float right = left + glyph.width*pxSizeX;
      const float bottom = baseVertexY - (glyph.height - glyph.bearingTop)*pxSizeY;
      const float top = bottom + glyph.height*pxSizeY;
      vertices.emplace_back(TextVertex{ {left,top},  {0.f,0.f} });
      vertices.emplace_back(TextVertex{ {right,top}, {1.f,0.f} });
      vertices.emplace_back(TextVertex{ {left,bottom}, {0.f,1.f} });
      vertices.emplace_back(TextVertex{ {right,bottom},{1.f,1.f} });
      
      indices.emplace_back(glyphFirstIndex);
      indices.emplace_back(glyphFirstIndex + 1);
      indices.emplace_back(glyphFirstIndex + 2);
      indices.emplace_back(glyphFirstIndex + 2);
      indices.emplace_back(glyphFirstIndex + 1);
      indices.emplace_back(glyphFirstIndex + 3);
      glyphFirstIndex += 4;
    }
    currentX += (double)glyph.advance;
    glyphs.emplace_back(&glyph);
  }
  const auto& lastGlyph = glyphs.back();
  width_ = currentX - (double)x - (double)lastGlyph->advance + (double)lastGlyph->width + (double)lastGlyph->offsetLeft;

  if (align != TextAlignment::left) {
    const uint32_t alignOffsetX = (align == TextAlignment::center) ? (width() >> 1) : width();
    this->x_ -= alignOffsetX;

    const float vertexOffsetX = alignOffsetX * pxSizeX;
    for (auto& vertex : vertices)
      vertex.position[0] -= vertexOffsetX;
  }
  
  if (!vertices.empty()) {
    vertexBuffer = Buffer<ResourceUsage::staticGpu>(renderer, BufferType::vertex,
                                                    vertices.size()*sizeof(TextVertex), vertices.data(), false);
    indexBuffer = Buffer<ResourceUsage::staticGpu>(renderer, BufferType::vertexIndex,
                                                   indices.size()*sizeof(uint32_t), indices.data(), false);
  }
}

void TextMesh::move(Renderer& renderer, const float pxSizeX, const float pxSizeY, int32_t x, int32_t y) {
  // centered/right alignments
  this->x_ = x;
  this->y_ = y;
  if (vertices.empty())
    return;

  double currentX = (double)x;
  const float baseVertexY = ToVertexPositionY(y_ + (int32_t)height_, pxSizeY);
  
  // update coordinates
  auto* vertexIt = &vertices[0];
  const auto* endGlyph = &glyphs[0] + (intptr_t)glyphs.size();
  for (auto* glyph = &glyphs[0]; glyph < endGlyph; ++glyph) {
    const auto* curGlyph = *glyph;
    if (!curGlyph->texture.isEmpty()) {
      const float left = ToVertexPositionX((float)currentX + curGlyph->offsetLeft, pxSizeX);
      const float right = left + curGlyph->width*pxSizeX;
      const float bottom = baseVertexY - (curGlyph->height - curGlyph->bearingTop)*pxSizeY;
      const float top = bottom + curGlyph->height*pxSizeY;
      vertexIt->position[0] = left;      vertexIt->position[1] = top;
      (++vertexIt)->position[0] = right; vertexIt->position[1] = top;
      (++vertexIt)->position[0] = left;  vertexIt->position[1] = bottom;
      (++vertexIt)->position[0] = right; vertexIt->position[1] = bottom;
      ++vertexIt;
    }
    currentX += (double)curGlyph->advance;
  }
  vertexBuffer = Buffer<ResourceUsage::staticGpu>(renderer, BufferType::vertex,
                                                  vertices.size()*sizeof(TextVertex), vertices.data(), false);
}

void TextMesh::cloneAtLocation(video_api::Renderer& renderer, const float pxSizeX, const float pxSizeY,
                               int32_t x, int32_t y, TextMesh& outClone) {
  outClone.glyphs = this->glyphs;
  outClone.vertices = this->vertices;
  outClone.indices = this->indices;
  outClone.width_ = this->width_;
  outClone.height_ = this->height_;
  outClone.indexBuffer = Buffer<ResourceUsage::staticGpu>(renderer, BufferType::vertexIndex,
                                                          outClone.indices.size()*sizeof(uint32_t),
                                                          outClone.indices.data(), false);
  outClone.move(renderer, pxSizeX, pxSizeY, x, y);
}


// -- add characters -- --------------------------------------------------------

bool TextMesh::push(video_api::Renderer& renderer, Font& font, const float pxSizeX, const float pxSizeY, char32_t code) {
  const auto& glyph = font.getGlyph(renderer, code);
  if (glyph.advance == 0.f && glyph.texture.isEmpty())
    return false;

  if (!glyphs.empty()) {
    const auto& lastGlyph = glyphs.back();
    width_ = (glyphs.size() > (size_t)1) ? (width_ + (double)lastGlyph->advance - (double)lastGlyph->width) : (double)lastGlyph->advance;
  }
  
  if (!glyph.texture.isEmpty()) {
    const float left = ToVertexPositionX((float)x_ + (float)width_ + glyph.offsetLeft, pxSizeX);
    const float right = left + glyph.width*pxSizeX;
    const float bottom = ToVertexPositionY((float)(y_ + (int32_t)height_) + (glyph.height - glyph.bearingTop), pxSizeY);
    const float top = bottom + glyph.height*pxSizeY;
    uint32_t vertexCount = (uint32_t)vertices.size();

    vertices.emplace_back(TextVertex{ {left,top},  {0.f,0.f} });
    vertices.emplace_back(TextVertex{ {right,top}, {1.f,0.f} });
    vertices.emplace_back(TextVertex{ {left,bottom}, {0.f,1.f} });
    vertices.emplace_back(TextVertex{ {right,bottom},{1.f,1.f} });
    indices.emplace_back(vertexCount);
    indices.emplace_back(vertexCount + 1);
    indices.emplace_back(vertexCount + 2);
    indices.emplace_back(vertexCount + 2);
    indices.emplace_back(vertexCount + 1);
    indices.emplace_back(vertexCount + 3);

    vertexBuffer = Buffer<ResourceUsage::staticGpu>(renderer, BufferType::vertex,
                                                    vertices.size()*sizeof(TextVertex), vertices.data(), false);
    indexBuffer = Buffer<ResourceUsage::staticGpu>(renderer, BufferType::vertexIndex,
                                                   indices.size()*sizeof(uint32_t), indices.data(), false);
  }
  width_ += glyph.width + glyph.offsetLeft;
  glyphs.emplace_back(&glyph);
  return true;
}

// ---

bool TextMesh::insertBefore(video_api::Renderer& renderer, Font& font, const float pxSizeX,
                            const float pxSizeY, char32_t code, uint32_t index) {
  if (index >= (uint32_t)glyphs.size())
    return false;
  const auto& glyph = font.getGlyph(renderer, code);
  if (glyph.advance == 0.f && glyph.texture.isEmpty())
    return false;

  const auto glyphAfter = glyphs.begin() + index;
  uint32_t vertexIndex = 0;
  double insertedCharX = (double)x_;
  for (auto glyphIt = glyphs.begin(); glyphIt != glyphAfter; ++glyphIt) {
    insertedCharX += (double)(*glyphIt)->advance;
    if (!(*glyphIt)->texture.isEmpty())
      vertexIndex += 4;
  }
  width_ += (double)glyph.advance;

  const float vertexOffsetX = glyph.advance*pxSizeX; // move vertices located after inserted glyph
  for (auto vertexIt = vertices.begin() + vertexIndex; vertexIt != vertices.end(); ++vertexIt)
    vertexIt->position[0] += vertexOffsetX;

  if (!glyph.texture.isEmpty()) {
    const float left = ToVertexPositionX((float)insertedCharX + glyph.offsetLeft, pxSizeX);
    const float right = left + glyph.width*pxSizeX;
    const float bottom = ToVertexPositionY((float)(y_ + (int32_t)height_) + (glyph.height - glyph.bearingTop), pxSizeY);
    const float top = bottom + glyph.height*pxSizeY;
    uint32_t vertexCount = (uint32_t)vertices.size();

    vertices.insert(vertices.begin() + vertexIndex, {
      TextVertex{ {left,top},  {0.f,0.f} },
      TextVertex{ {right,top}, {1.f,0.f} },
      TextVertex{ {left,bottom}, {0.f,1.f} },
      TextVertex{ {right,bottom},{1.f,1.f} }
    });
    indices.emplace_back(vertexCount);
    indices.emplace_back(vertexCount + 1);
    indices.emplace_back(vertexCount + 2);
    indices.emplace_back(vertexCount + 2);
    indices.emplace_back(vertexCount + 1);
    indices.emplace_back(vertexCount + 3);

    indexBuffer = Buffer<ResourceUsage::staticGpu>(renderer, BufferType::vertexIndex,
                                                   indices.size()*sizeof(uint32_t), indices.data(), false);
  }
  vertexBuffer = Buffer<ResourceUsage::staticGpu>(renderer, BufferType::vertex,
                                                  vertices.size()*sizeof(TextVertex), vertices.data(), false);
  glyphs.insert(glyphAfter, &glyph);
  return true;
}


// -- remove characters -- -----------------------------------------------------

void TextMesh::pop(video_api::Renderer& renderer) {
  if (glyphs.empty())
    return;

  const auto& lastGlyph = glyphs.back();
  if (glyphs.size() >= (size_t)2) {
    const auto& previousGlyph = glyphs[glyphs.size() - (size_t)2];
    width_ -= (double)lastGlyph->offsetLeft + (double)lastGlyph->width + (double)previousGlyph->advance - (double)previousGlyph->width;
    if (width_ < 0.0)
      width_ = 0.0;
  }
  else width_ = 0.0;

  if (!lastGlyph->texture.isEmpty()) {
    vertices.resize(vertices.size() - (size_t)4);
    indices.resize(indices.size() - (size_t)6);

    if (!vertices.empty()) {
      vertexBuffer = Buffer<ResourceUsage::staticGpu>(renderer, BufferType::vertex,
                                                      vertices.size()*sizeof(TextVertex), vertices.data(), false);
      indexBuffer = Buffer<ResourceUsage::staticGpu>(renderer, BufferType::vertexIndex,
                                                     indices.size()*sizeof(uint32_t), indices.data(), false);
    }
    else {
      vertexBuffer.release();
      indexBuffer.release();
    }
  }
  glyphs.pop_back();
}

// ---

void TextMesh::removeAt(video_api::Renderer& renderer, const float pxSizeX, uint32_t index) {
  if (glyphs.size() < (size_t)2 || index >= (uint32_t)glyphs.size() - 1u) {
    pop(renderer); // last char -> different advance/width management -> use pop()
    return;
  }

  const auto glyph = glyphs.begin() + index;
  uint32_t vertexIndex = 0;
  for (auto glyphIt = glyphs.begin(); glyphIt != glyph; ++glyphIt) {
    if (!(*glyphIt)->texture.isEmpty())
      vertexIndex += 4;
  }
  width_ -= (double)(*glyph)->advance;
  
  const float vertexOffsetX = (*glyph)->advance*pxSizeX; // move vertices located after removed glyph
  for (auto vertexIt = vertices.begin() + ((intptr_t)vertexIndex + 4); vertexIt != vertices.end(); ++vertexIt)
    vertexIt->position[0] -= vertexOffsetX;

  if (!(*glyph)->texture.isEmpty()) {
    vertices.erase(vertices.begin() + vertexIndex, vertices.begin() + vertexIndex + 4);
    indices.resize(indices.size() - (size_t)6);

    if (!indices.empty())
      indexBuffer = Buffer<ResourceUsage::staticGpu>(renderer, BufferType::vertexIndex,
                                                     indices.size()*sizeof(uint32_t), indices.data(), false);
  }
  if (!vertices.empty()) {
    vertexBuffer = Buffer<ResourceUsage::staticGpu>(renderer, BufferType::vertex,
                                                    vertices.size()*sizeof(TextVertex), vertices.data(), false);
  }
  else {
    vertexBuffer.release();
    indexBuffer.release();
  }
  glyphs.erase(glyph);
}


// -- rendering -- -------------------------------------------------------------

void TextMesh::draw(Renderer& renderer) {
  if (vertices.empty())
    return;
  renderer.bindVertexArrayBuffer(0, vertexBuffer.handle(), sizeof(TextVertex), 0);
  renderer.bindVertexIndexBuffer(indexBuffer.handle(), VertexIndexFormat::r32_ui, 0);
  
  uint32_t indexOffset = 0;
  const auto* endGlyph = &glyphs[0] + (intptr_t)glyphs.size();
  for (auto* glyph = &glyphs[0]; glyph < endGlyph; ++glyph) {
    const auto& texture = (*glyph)->texture;
    if (!texture.isEmpty()) {
      renderer.bindFragmentTextures(0, texture.resourceViewPtr(), 1);
      renderer.drawIndexed(6u, indexOffset);
      indexOffset += 6u;
    }
  }
}


// -- utils -- -----------------------------------------------------------------

size_t TextMesh::getStringLength(const char16_t* value) {
  if (value == nullptr)
    return 0;
  const char16_t* it = value;
  while (*it)
  ++it;
  return static_cast<size_t>(it - value);
}
