#*******************************************************************************
# Pandora GS - PSEmu-compatible GPU driver
# Copyright (C) 2021  Romain Vinders

# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation, version 2
# of the License.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details (LICENSE file).
#*******************************************************************************
cmake_minimum_required(VERSION 3.14)
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/_libs/pandora_toolbox/CMakeLists.txt")
    find_package(Git QUIET)
    if(GIT_FOUND)
        execute_process(COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive --remote
                        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} RESULT_VARIABLE GIT_SUBMOD_RESULT)
    endif()
endif()

# ┌──────────────────────────────────────────────────────────────────┐
# │  Solution settings                                               │
# └──────────────────────────────────────────────────────────────────┘
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake_options.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/_libs/pandora_toolbox/_cmake/cwork.cmake")
cwork_read_version_from_file("${CMAKE_CURRENT_SOURCE_DIR}/build_version.txt" ON)
project("gpu_pandora_GS" VERSION ${CWORK_BUILD_VERSION} LANGUAGES C CXX)

cwork_create_solution("${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/_libs/pandora_toolbox/_cmake")

# ┌──────────────────────────────────────────────────────────────────┐
# │  Toolbox dependencies                                            │
# └──────────────────────────────────────────────────────────────────┘
cwork_set_custom_libs("${CWORK_SOLUTION_PATH}/_libs" pandora_toolbox OFF OFF
    system
    thread
    pattern
    memory
    logic
    time
    io
    hardware
    video
)

# ┌──────────────────────────────────────────────────────────────────┐
# │  Solution projects                                               │
# └──────────────────────────────────────────────────────────────────┘
cwork_set_solution_projects(
    config
    display
    psemu
)
