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


TextMesh::TextMesh(Renderer& renderer, Font& font, const char32_t* text,
                   const float pxSizeX, const float pxSizeY, int32_t x, int32_t y, TextAlignment align)
  : x_(x),
    y_(y),
    height_(font.XHeight()) {
  if (text == nullptr || *text == (char32_t)0)
    return;
  int32_t currentX = x;
  const float baseVertexY = ToVertexPositionY(y + height_, pxSizeY);
  uint32_t glyphFirstIndex = 0;
  
  for (; *text; ++text) {
    const auto& glyph = font.getGlyph(renderer, *text);
    if (!glyph->texture.isEmpty()) {
      const float left = ToVertexPositionX(currentX + glyph->offsetLeft, pxSizeX);
      const float right = left + glyph->width*pxSizeX;
      const float bottom = baseVertexY - (glyph->height - glyph->bearingTop)*pxSizeY;
      const float top = bottom + glyph->height*pxSizeY;
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
    currentX += (int32_t)(glyph->advance >> 6);
    glyphs.emplace_back(glyph);
  }
  const auto& lastGlyph = glyphs.back();
  width_ = currentX - x - (int32_t)(lastGlyph->advance >> 6) + (int32_t)lastGlyph->width + lastGlyph->offsetLeft;

  if (align != TextAlignment::left) {
    const uint32_t alignOffsetX = (align == TextAlignment::center) ? (width_ >> 1) : width_;
    this->x_ -= alignOffsetX;

    const float vertexOffsetX = alignOffsetX * pxSizeX;
    for (auto& vertex : vertices)
      vertex.position[0] -= vertexOffsetX;
  }
  
  if (!vertices.empty()) {
    vertexBuffer = Buffer<ResourceUsage::staticGpu>(renderer, BufferType::vertex,
                                                    vertices.size()*sizeof(TextVertex), vertices.data());
    indexBuffer = Buffer<ResourceUsage::staticGpu>(renderer, BufferType::vertexIndex,
                                                   indices.size()*sizeof(uint32_t), indices.data());
  }
}

void TextMesh::move(Renderer& renderer, const float pxSizeX, const float pxSizeY, int32_t x, int32_t y) {
  // centered/right alignments
  this->x_ = x;
  this->y_ = y;
  if (vertices.empty())
    return;

  int32_t currentX = x;
  const float baseVertexY = ToVertexPositionY(y_ + height_, pxSizeY);
  
  // update coordinates
  auto* vertexIt = &vertices[0];
  const auto* endGlyph = &glyphs[0] + (intptr_t)glyphs.size();
  for (auto* glyph = &glyphs[0]; glyph < endGlyph; ++glyph) {
    const auto* curGlyph = glyph->get();
    if (!curGlyph->texture.isEmpty()) {
      const float left = ToVertexPositionX(currentX + curGlyph->offsetLeft, pxSizeX);
      const float right = left + curGlyph->width*pxSizeX;
      const float bottom = baseVertexY - (curGlyph->height - curGlyph->bearingTop)*pxSizeY;
      const float top = bottom + curGlyph->height*pxSizeY;
      vertexIt->position[0] = left;      vertexIt->position[1] = top;
      (++vertexIt)->position[0] = right; vertexIt->position[1] = top;
      (++vertexIt)->position[0] = left;  vertexIt->position[1] = bottom;
      (++vertexIt)->position[0] = right; vertexIt->position[1] = bottom;
      ++vertexIt;
    }
    currentX += (int32_t)(curGlyph->advance >> 6);
  }
  vertexBuffer = Buffer<ResourceUsage::staticGpu>(renderer, BufferType::vertex,
                                                  vertices.size()*sizeof(TextVertex), vertices.data());
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
                                                          outClone.indices.data());
  outClone.move(renderer, pxSizeX, pxSizeY, x, y);
}


// -- add characters -- --------------------------------------------------------

bool TextMesh::push(video_api::Renderer& renderer, Font& font, const float pxSizeX, const float pxSizeY, char32_t code) {
  const auto& glyph = font.getGlyph(renderer, code);
  if (glyph->advance == 0 && glyph->texture.isEmpty())
    return false;

  int32_t advancePrevX = 0;
  if (!glyphs.empty()) {
    const auto& lastGlyph = glyphs.back();
    advancePrevX = (int32_t)(lastGlyph->advance >> 6) - (int32_t)lastGlyph->width;
  }
  width_ += advancePrevX;
  
  if (!glyph->texture.isEmpty()) {
    const float left = ToVertexPositionX(this->x_ + (int32_t)width_ + glyph->offsetLeft, pxSizeX);
    const float right = left + glyph->width*pxSizeX;
    const float bottom = ToVertexPositionY(this->y_ + (int32_t)height_ - (glyph->height - glyph->bearingTop), pxSizeY);
    const float top = bottom + glyph->height*pxSizeY;
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
                                                    vertices.size()*sizeof(TextVertex), vertices.data());
    indexBuffer = Buffer<ResourceUsage::staticGpu>(renderer, BufferType::vertexIndex,
                                                   indices.size()*sizeof(uint32_t), indices.data());
  }
  width_ += static_cast<uint32_t>((int32_t)glyph->width + glyph->offsetLeft);
  glyphs.emplace_back(glyph);
  return true;
}

// ---

bool TextMesh::insertBefore(video_api::Renderer& renderer, Font& font, const float pxSizeX,
                            const float pxSizeY, char32_t code, uint32_t index) {
  if (index >= (uint32_t)glyphs.size())
    return false;
  const auto& glyph = font.getGlyph(renderer, code);
  if (glyph->advance == 0 && glyph->texture.isEmpty())
    return false;

  const auto glyphAfter = glyphs.begin() + index;
  uint32_t vertexIndex = 0;
  int32_t insertedCharX = this->x_;
  for (auto glyphIt = glyphs.begin(); glyphIt != glyphAfter; ++glyphIt) {
    insertedCharX += ((*glyphIt)->advance >> 6);
    if (!(*glyphIt)->texture.isEmpty())
      vertexIndex += 4;
  }
  const int32_t advanceX = (int32_t)(glyph->advance >> 6);
  width_ += advanceX;

  const float vertexOffsetX = (float)advanceX*pxSizeX; // move vertices located after inserted glyph
  for (auto vertexIt = vertices.begin() + vertexIndex; vertexIt != vertices.end(); ++vertexIt)
    vertexIt->position[0] += vertexOffsetX;

  if (!glyph->texture.isEmpty()) {
    const float left = ToVertexPositionX(insertedCharX + glyph->offsetLeft, pxSizeX);
    const float right = left + glyph->width*pxSizeX;
    const float bottom = ToVertexPositionY(this->y_ + (int32_t)height_ - (glyph->height - glyph->bearingTop), pxSizeY);
    const float top = bottom + glyph->height*pxSizeY;
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

    vertexBuffer = Buffer<ResourceUsage::staticGpu>(renderer, BufferType::vertex,
                                                    vertices.size()*sizeof(TextVertex), vertices.data());
    indexBuffer = Buffer<ResourceUsage::staticGpu>(renderer, BufferType::vertexIndex,
                                                   indices.size()*sizeof(uint32_t), indices.data());
  }
  glyphs.insert(glyphAfter, glyph);
  return true;
}


// -- remove characters -- -----------------------------------------------------

void TextMesh::pop(video_api::Renderer& renderer) {
  if (glyphs.empty())
    return;

  const auto& lastGlyph = glyphs.back();
  int32_t retreatX = lastGlyph->offsetLeft + (int32_t)lastGlyph->width;
  if (glyphs.size() >= (size_t)2) {
    const auto& previousGlyph = glyphs[glyphs.size() - (size_t)2];
    retreatX += (int32_t)(previousGlyph->advance >> 6) - (int32_t)previousGlyph->width;
  }
  width_ -= retreatX;

  if (!lastGlyph->texture.isEmpty()) {
    vertices.resize(vertices.size() - (size_t)4);
    indices.resize(indices.size() - (size_t)6);

    if (!vertices.empty()) {
      vertexBuffer = Buffer<ResourceUsage::staticGpu>(renderer, BufferType::vertex,
                                                      vertices.size()*sizeof(TextVertex), vertices.data());
      indexBuffer = Buffer<ResourceUsage::staticGpu>(renderer, BufferType::vertexIndex,
                                                     indices.size()*sizeof(uint32_t), indices.data());
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
  if (index >= (uint32_t)glyphs.size() - 1u)
    return pop(renderer); // last char -> different advance/width management -> use pop()

  const auto glyph = glyphs.begin() + index;
  uint32_t vertexIndex = 0;
  for (auto glyphIt = glyphs.begin(); glyphIt != glyph; ++glyphIt) {
    if (!(*glyphIt)->texture.isEmpty())
      vertexIndex += 4;
  }
  const int32_t retreatX = (int32_t)((*glyph)->advance >> 6);
  width_ -= retreatX;
  
  const float vertexOffsetX = (float)retreatX*pxSizeX; // move vertices located after removed glyph
  for (auto vertexIt = vertices.begin() + (vertexIndex + 4); vertexIt != vertices.end(); ++vertexIt)
    vertexIt->position[0] -= vertexOffsetX;

  if (!(*glyph)->texture.isEmpty()) {
    vertices.erase(vertices.begin() + vertexIndex, vertices.begin() + vertexIndex + 4);
    indices.resize(indices.size() - (size_t)6);

    if (!indices.empty())
      indexBuffer = Buffer<ResourceUsage::staticGpu>(renderer, BufferType::vertexIndex,
                                                     indices.size()*sizeof(uint32_t), indices.data());
  }
  if (!vertices.empty()) {
    vertexBuffer = Buffer<ResourceUsage::staticGpu>(renderer, BufferType::vertex,
                                                    vertices.size()*sizeof(TextVertex), vertices.data());
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

std::unique_ptr<char32_t[]> TextMesh::toString(const char32_t* text) {
  size_t length = 0;
  if (text != nullptr) {
    for (const char32_t* it = text; *it; ++it)
      ++length;
  }
  if (length > 0) {
    ++length; // include ending zero
    std::unique_ptr<char32_t[]> storage(new char32_t[length]);
    memcpy(storage.get(), text, length*sizeof(char32_t));
    return storage;
  }
  return nullptr;
}
