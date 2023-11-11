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
#include "display/controls/icon_mesh.h"

using namespace video_api;
using namespace display;
using namespace display::controls;


void IconMesh::initFullImage(Renderer& renderer, const float pxSizeX, const float pxSizeY) {
  const float left = ToVertexPositionX(x_, pxSizeX);
  const float top = ToVertexPositionY(y_, pxSizeY);
  const float right = left + (float)width_*pxSizeX;
  const float bottom = top - (float)height_*pxSizeY;
  
  IconVertex iconVertices[4] {
    IconVertex{ {left,top},  {0.f,0.f} },
    IconVertex{ {right,top}, {1.f,0.f} },
    IconVertex{ {left,bottom},  {0.f,1.f} },
    IconVertex{ {right,bottom}, {1.f,1.f} }
  };
  memcpy(this->vertices, iconVertices, 4*sizeof(IconVertex));
  vertexBuffer = Buffer<ResourceUsage::staticGpu>(renderer, BufferType::vertex,
                                                  4*sizeof(IconVertex), iconVertices);
  
  uint32_t indices[6] { 0,1,2, 2,1,3 };
  indexBuffer = Buffer<ResourceUsage::staticGpu>(renderer, BufferType::vertexIndex,
                                                 6*sizeof(uint32_t), indices);
}

IconMesh::IconMesh(Renderer& renderer, std::shared_ptr<Texture2D> texture,
                   const float pxSizeX, const float pxSizeY, int32_t x, int32_t y,
                   uint32_t txOffsetX, uint32_t txOffsetY, uint32_t width, uint32_t height)
  : texture(std::move(texture)),
    x_(x),
    y_(y),
    width_(width),
    height_(height) {
  if (this->texture == nullptr)
    return;
  const float left = ToVertexPositionX(x, pxSizeX);
  const float top = ToVertexPositionY(y, pxSizeY);
  const float right = left + (float)width*pxSizeX;
  const float bottom = top - (float)height*pxSizeY;
  
  const uint32_t textureWidth = this->texture->rowBytes() >> 2;
  const float texLeft = ToTextureCoord(txOffsetX, textureWidth);
  const float texTop = ToTextureCoord(txOffsetY, this->texture->height());
  const float texRight = ToTextureCoord(txOffsetX + width, textureWidth);
  const float texBottom = ToTextureCoord(txOffsetY + height, this->texture->height());
  
  IconVertex iconVertices[4] {
    IconVertex{ {left,top},  {texLeft,texTop} },
    IconVertex{ {right,top}, {texRight,texTop} },
    IconVertex{ {left,bottom},  {texLeft,texBottom} },
    IconVertex{ {right,bottom}, {texRight,texBottom} }
  };
  memcpy(this->vertices, iconVertices, 4*sizeof(IconVertex));
  vertexBuffer = Buffer<ResourceUsage::staticGpu>(renderer, BufferType::vertex,
                                                  4*sizeof(IconVertex), iconVertices);
  
  uint32_t indices[6] { 0,1,2, 2,1,3 };
  indexBuffer = Buffer<ResourceUsage::staticGpu>(renderer, BufferType::vertexIndex,
                                                 6*sizeof(uint32_t), indices);
}

// ---

void IconMesh::regenerate(Renderer& renderer, const float pxSizeX, const float pxSizeY) {
  if (texture == nullptr)
    return;
  const float left = ToVertexPositionX(x_, pxSizeX);
  const float top = ToVertexPositionY(y_, pxSizeY);
  const float right = left + (float)width_*pxSizeX;
  const float bottom = top - (float)height_*pxSizeY;
  
  // move vertices at requested position
  IconVertex* vertexIt = vertices;
  vertexIt->position[0] = left;      vertexIt->position[1] = top;
  (++vertexIt)->position[0] = right; vertexIt->position[1] = top;
  (++vertexIt)->position[0] = left; vertexIt->position[1] = bottom;
  (++vertexIt)->position[0] = right; vertexIt->position[1] = bottom;
  
  vertexBuffer = Buffer<ResourceUsage::staticGpu>(renderer, BufferType::vertex,
                                                  4*sizeof(IconVertex), vertices);
}

void IconMesh::invertX(video_api::Renderer& renderer, const float pxSizeX, const float pxSizeY) {
  for (auto& vertex : vertices)
    vertex.coords[0] = -vertex.coords[0];
  vertexBuffer = Buffer<ResourceUsage::staticGpu>(renderer, BufferType::vertex,
                                                  4*sizeof(IconVertex), vertices);
}

// ---

void IconMesh::draw(Renderer& renderer) {
  if (texture == nullptr)
    return;
  renderer.bindFragmentTextures(0, texture->resourceViewPtr(), 1);
  renderer.bindVertexArrayBuffer(0, vertexBuffer.handle(), sizeof(IconVertex), 0);
  renderer.bindVertexIndexBuffer(indexBuffer.handle(), VertexIndexFormat::r32_ui, 0);
  renderer.drawIndexed(6, 0);
}
