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
#include "display/controls/control_mesh.h"

using namespace video_api;
using namespace display;
using namespace display::controls;


ControlMesh::ControlMesh(Renderer& renderer, std::vector<ControlVertex>&& verticesRelPos,
                         const std::vector<uint32_t>& indices, const float pxSizeX, const float pxSizeY,
                         int32_t x, int32_t y, uint32_t width, uint32_t height)
  : verticesRelPos(std::move(verticesRelPos)),
    indexCount((uint32_t)indices.size()),
    x_(x),
    y_(y),
    width_(width),
    height_(height) {
  const float baseVertexX = ToVertexPositionX(x, pxSizeX);
  const float baseVertexY = ToVertexPositionY(y, pxSizeY);
  
  // move vertices at requested position
  std::vector<ControlVertex> vertices;
  vertices.resize(this->verticesRelPos.size());
  memcpy(vertices.data(), this->verticesRelPos.data(), this->verticesRelPos.size()*sizeof(ControlVertex));

  for (auto& vertex : vertices) {
    vertex.position[0] *= pxSizeX;
    vertex.position[0] += baseVertexX;
    vertex.position[1] *= pxSizeY;
    vertex.position[1] += baseVertexY;
  }
  
  // GPU buffer storage
  vertexBuffer = Buffer<ResourceUsage::staticGpu>(renderer, BufferType::vertex,
                                                  vertices.size()*sizeof(ControlVertex),
                                                  vertices.data());
  indexBuffer = Buffer<ResourceUsage::staticGpu>(renderer, BufferType::vertexIndex,
                                                 indices.size()*sizeof(uint32_t),
                                                 indices.data());
}

void ControlMesh::move(Renderer& renderer, const float pxSizeX, const float pxSizeY, int32_t x, int32_t y) {
  if (verticesRelPos.empty())
    return;
  this->x_ = x;
  this->y_ = y;
  // set aligned origins
  const float baseVertexX = ToVertexPositionX(x, pxSizeX);
  const float baseVertexY = ToVertexPositionY(y, pxSizeY);
  
  // move vertices at requested position
  std::vector<ControlVertex> vertices;
  vertices.resize(this->verticesRelPos.size());
  memcpy(vertices.data(), this->verticesRelPos.data(), this->verticesRelPos.size()*sizeof(ControlVertex));

  for (auto& vertex : vertices) {
    vertex.position[0] *= pxSizeX;
    vertex.position[0] += baseVertexX;
    vertex.position[1] *= pxSizeY;
    vertex.position[1] += baseVertexY;
  }
  
  // GPU buffer storage
  vertexBuffer = Buffer<ResourceUsage::staticGpu>(renderer, BufferType::vertex,
                                                  vertices.size()*sizeof(ControlVertex),
                                                  vertices.data());
}

// ---

void ControlMesh::draw(Renderer& renderer) {
  renderer.bindVertexArrayBuffer(0, vertexBuffer.handle(), sizeof(ControlVertex), 0);
  renderer.bindVertexIndexBuffer(indexBuffer.handle(), VertexIndexFormat::r32_ui, 0);
  renderer.drawIndexed(indexCount, 0);
}
