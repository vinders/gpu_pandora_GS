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
#*******************************************************************************
cmake_minimum_required(VERSION 3.14)
include("${CMAKE_CURRENT_SOURCE_DIR}/../../../_libs/pandora_toolbox/_cmake/cwork.cmake")
cwork_set_default_solution("gpu_pandora_GS" "${CMAKE_CURRENT_SOURCE_DIR}/../../..")
include("${CMAKE_CURRENT_SOURCE_DIR}/../../../Version.cmake")
project("${CWORK_SOLUTION_NAME}.primitive_viewer" VERSION ${CWORK_BUILD_VERSION} LANGUAGES C CXX)

# ┌──────────────────────────────────────────────────────────────────┐
# │  Dependencies                                                    │
# └──────────────────────────────────────────────────────────────────┘
cwork_set_custom_libs("${CWORK_SOLUTION_PATH}/_libs" pandora_toolbox ON OFF
    system
    hardware
    video
)
cwork_set_internal_libs(display)

# ┌──────────────────────────────────────────────────────────────────┐
# │  Project settings                                                │
# └──────────────────────────────────────────────────────────────────┘
if(WIN32 OR WIN64 OR _WIN32 OR _WIN64)
    set(_RES_DIR "${CWORK_SOLUTION_PATH}/_img/resources")
    configure_file("${CWORK_SOLUTION_PATH}/_img/templates/resources.rc.in" "${CMAKE_CURRENT_SOURCE_DIR}/_generated/resources.rc")
    file(COPY "${CWORK_SOLUTION_PATH}/_img/templates/resources.h" DESTINATION "${CMAKE_CURRENT_SOURCE_DIR}/include/_generated")

    cwork_set_embedded_resource_dir("${CWORK_SOLUTION_PATH}/_img/resources")
    cwork_set_source_files(
      "${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp"
      "${CMAKE_CURRENT_SOURCE_DIR}/_generated/resources.rc"
    )
    unset(_RES_DIR)
else()
    cwork_set_resource_dir("${CWORK_SOLUTION_PATH}/_img/resources")
endif()

cwork_set_subproject_type("tools")
cwork_create_project("executable" "${CWORK_SOLUTION_PATH}/_libs/pandora_toolbox/_cmake"
                     "${CWORK_SOLUTION_PATH}/_libs/pandora_toolbox/_cmake/modules"
                     "include" "src" "test")
