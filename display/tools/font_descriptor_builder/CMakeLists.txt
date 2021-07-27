#*******************************************************************************
# Pandora GS - PSEmu-compatible GPU driver
# Copyright (C) 2021  Romain Vinders

# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation, version 2 of the License.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details (LICENSE file).
# ------------------------------------------------------------------------------
# Description : Binary font descriptor builder (from CSV)
#               This tool generates binary font descriptors from CSV descriptions.
# Note : Should only be run on systems with big-endian or non-standard memory layout.
#        After running it, the Cmake project should be regenerated to ensure proper resource copy.
#*******************************************************************************
cmake_minimum_required(VERSION 3.14)
include("${CMAKE_CURRENT_SOURCE_DIR}/../../../_libs/pandora_toolbox/_cmake/cwork.cmake")
cwork_set_default_solution("gpu_pandora_GS" "${CMAKE_CURRENT_SOURCE_DIR}/../../..")
cwork_read_version_from_file("${CMAKE_CURRENT_SOURCE_DIR}/../../../build_version.txt" OFF)
project("${CWORK_SOLUTION_NAME}.font_descriptor_builder" VERSION ${CWORK_BUILD_VERSION} LANGUAGES C CXX)

# ┌──────────────────────────────────────────────────────────────────┐
# │  Dependencies                                                    │
# └──────────────────────────────────────────────────────────────────┘
cwork_set_custom_libs("${CWORK_SOLUTION_PATH}/_libs" pandora_toolbox ON OFF
    system
    io
)
cwork_set_internal_libs(display)

# ┌──────────────────────────────────────────────────────────────────┐
# │  Project settings                                                │
# └──────────────────────────────────────────────────────────────────┘
add_definitions(-D__P_RESOURCE_DIR_PATH="${CWORK_SOLUTION_PATH}/_img/resources")
add_definitions(-D__P_OUTPUT_DIR_PATH="${CWORK_SOLUTION_PATH}/_img/resources")

cwork_set_subproject_type("tools")
cwork_create_project("console" "${CWORK_SOLUTION_PATH}/_libs/pandora_toolbox/_cmake"
                     "${CWORK_SOLUTION_PATH}/_libs/pandora_toolbox/_cmake/modules"
                     "include" "src" "test")
