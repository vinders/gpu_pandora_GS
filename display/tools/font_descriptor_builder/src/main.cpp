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
--------------------------------------------------------------------------------
Description : Binary font descriptor builder (from CSV)
              This tool generates binary font descriptors from CSV descriptions.
Note : Should only be run on systems with big-endian or non-standard memory layout.
       After running it, the Cmake project should be regenerated to ensure proper resource copy.
*******************************************************************************/
#include <cstdio>
#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <cstdlib>
#include <io/file_system_io.h>
#include <display/font_map.h>

#define __BUFFER_SIZE 64

// build descriptor file
static void __buildFile(const char* sourcePath, const char* outPath) {
  char buffer[__BUFFER_SIZE];
  std::ifstream srcFile(sourcePath, std::ios_base::in);
  if (!srcFile.is_open()) {
    printf("Error: could not open source file...\n");
    exit(-1);
  }
  std::ofstream outFile(outPath, std::ios_base::out | std::ios_base::binary | std::ios_base::trunc);
  if (!outFile.is_open()) {
    printf("Error: could not create output file...\n");
    exit(-2);
  }

  srcFile.getline(buffer, __BUFFER_SIZE); // skip global prop title row
  srcFile.getline(buffer, __BUFFER_SIZE, ',');
  uint8_t lineHeight = (uint8_t)atoi(buffer);
  srcFile.getline(buffer, __BUFFER_SIZE);
  uint32_t base = (uint32_t)atoi(buffer);

  // characters
  std::map<uint32_t, display::CharDescriptor> descriptors; // ordered
  srcFile.getline(buffer, __BUFFER_SIZE); // skip title row

  srcFile.getline(buffer, __BUFFER_SIZE, ','); // start reading first line
  while (!srcFile.eof() && *buffer > ' ') {    // test if line exists
    uint32_t id = (uint32_t)atoi(buffer);
    if (descriptors.find(id) != descriptors.end()) {
      printf("Repeated ID %u (ignored)\n", id);
      continue;
    }
    auto& descriptor = descriptors[id];

    descriptor.id(id);
    srcFile.getline(buffer, __BUFFER_SIZE, ',');
    descriptor.x((uint8_t)atoi(buffer));
    srcFile.getline(buffer, __BUFFER_SIZE, ',');
    descriptor.y((uint32_t)atoi(buffer));
    srcFile.getline(buffer, __BUFFER_SIZE, ',');
    descriptor.width((uint8_t)atoi(buffer));
    srcFile.getline(buffer, __BUFFER_SIZE, ',');
    descriptor.height((uint8_t)atoi(buffer));
    srcFile.getline(buffer, __BUFFER_SIZE, ',');
    descriptor.offsetX((uint8_t)atoi(buffer));
    srcFile.getline(buffer, __BUFFER_SIZE, ',');
    descriptor.offsetY((uint8_t)atoi(buffer));
    srcFile.getline(buffer, __BUFFER_SIZE);
    descriptor.advanceX((uint8_t)atoi(buffer));
    descriptor.advanceY(lineHeight - descriptor.offsetY());

    srcFile.getline(buffer, __BUFFER_SIZE, ','); // try to start reading next line
  }

  std::vector<display::CharDescriptor> descriptorVec;
  descriptorVec.reserve(descriptors.size());
  if (descriptors.begin()->first <= '0') {
    for (auto it = descriptors.begin(); it != descriptors.end(); ++it)
      descriptorVec.emplace_back(it->second);
  }
  else {
    for (auto it = descriptors.rbegin(); it != descriptors.rend(); ++it)
      descriptorVec.emplace_back(it->second);
  }

  uint32_t length = descriptorVec.size();
  outFile.write((char*)&base, sizeof(uint32_t));
  outFile.write((char*)&length, sizeof(uint32_t));
  outFile.write((char*)&descriptorVec[0], sizeof(display::CharDescriptor)*descriptorVec.size());
  
  outFile.flush();
  outFile.close();
  srcFile.close();
  printf("%u characters successfully written\n", length);
}

// ---

// build binary font descriptor from CSV file
int main() {
  printf("____________________________________________________________\n"
         "\n FONT CSV TO DESCRIPTOR BUILDER\n"
         "____________________________________________________________\n\n");

  const char* fontDescPath = __P_RESOURCE_DIR_PATH "/text_font_map.csv";
  const char* fontOutPath = __P_OUTPUT_DIR_PATH "/text_font_map.desc";
  printf("Source: %s\nOutput: %s\n", fontDescPath, fontOutPath);
  __buildFile(fontDescPath, fontOutPath);
  
  const char* fontDescPath2 = __P_RESOURCE_DIR_PATH "/title_font_map.csv";
  const char* fontOutPath2 = __P_OUTPUT_DIR_PATH "/title_font_map.desc";
  printf("\nSource: %s\nOutput: %s\n", fontDescPath2, fontOutPath2);
  __buildFile(fontDescPath2, fontOutPath2);
  return 0;
}