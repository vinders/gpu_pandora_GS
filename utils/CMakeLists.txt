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
# Display pipeline and rendering
#*******************************************************************************
cmake_minimum_required(VERSION 3.14)
include("${CMAKE_CURRENT_SOURCE_DIR}/../cmake_options.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/../_libs/pandora_toolbox/_cmake/cwork.cmake")
cwork_set_default_solution("gpu_pandora_GS" "${CMAKE_CURRENT_SOURCE_DIR}/..")

include("${CMAKE_CURRENT_SOURCE_DIR}/../Version.cmake")
project("${CWORK_SOLUTION_NAME}.utils" VERSION ${CWORK_BUILD_VERSION} LANGUAGES C CXX)

# ┌──────────────────────────────────────────────────────────────────┐
# │  Dependencies                                                    │
# └──────────────────────────────────────────────────────────────────┘
cwork_set_custom_libs("${CWORK_SOLUTION_PATH}/_libs" pandora_toolbox ON OFF
    system
    memory
)

# ┌──────────────────────────────────────────────────────────────────┐
# │  Project settings                                                │
# └──────────────────────────────────────────────────────────────────┘
cwork_create_project("static" "${CWORK_SOLUTION_PATH}/_libs/pandora_toolbox/_cmake" 
                     "${CWORK_SOLUTION_PATH}/_libs/pandora_toolbox/_cmake/modules"
                     "include" "src" "test")
