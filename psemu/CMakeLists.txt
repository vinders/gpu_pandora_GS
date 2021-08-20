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
# PSEmu-compatible linked library interface
#*******************************************************************************
cmake_minimum_required(VERSION 3.14)
include("${CMAKE_CURRENT_SOURCE_DIR}/../cmake_options.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/../_libs/pandora_toolbox/_cmake/cwork.cmake")
cwork_set_default_solution("gpu_pandora_GS" "${CMAKE_CURRENT_SOURCE_DIR}/..")

set(_PROJECT_TITLE psemu)
cwork_read_version_from_file("${CMAKE_CURRENT_SOURCE_DIR}/../build_version.txt" OFF)
project("${CWORK_SOLUTION_NAME}.${_PROJECT_TITLE}" VERSION ${CWORK_BUILD_VERSION} LANGUAGES C CXX)

# ┌──────────────────────────────────────────────────────────────────┐
# │  Dependencies                                                    │
# └──────────────────────────────────────────────────────────────────┘
cwork_set_custom_libs("${CWORK_SOLUTION_PATH}/_libs" pandora_toolbox ON OFF
    system
)
cwork_set_internal_libs(config display)

# ┌──────────────────────────────────────────────────────────────────┐
# │  Library file generation                                         │
# └──────────────────────────────────────────────────────────────────┘
set(_LIB_FILE gpuPandoraGS)
set(_LIB_NAME "PandoraGS")
set(_RES_DIR "${CWORK_SOLUTION_PATH}/_img/resources")

if(WIN32 OR WIN64 OR _WIN32 OR _WIN64)
    if(OPTION_VULKAN_RENDERER)
        set(_LIB_FILE ${_LIB_FILE}_vulkan)
        set(_LIB_NAME "${_LIB_NAME} vulkan")
    else()
        set(_LIB_FILE ${_LIB_FILE}_DX11)
        set(_LIB_NAME "${_LIB_NAME} DX11")
    endif()
    if(CWORK_WINDOWS_VERSION AND CWORK_WINDOWS_VERSION STREQUAL "7")
        set(_LIB_FILE ${_LIB_FILE}_win7)
        set(_LIB_NAME "${_LIB_NAME} Win7")
    endif()

    set(__OUTPUT_LIB_DEF_DIR "${CMAKE_CURRENT_SOURCE_DIR}/_generated")
    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/templates/gpuPandoraGS.def.in" "${__OUTPUT_LIB_DEF_DIR}/${_LIB_FILE}.def")
    configure_file("${CWORK_SOLUTION_PATH}/_img/templates/resources.rc.in" "${__OUTPUT_LIB_DEF_DIR}/${_LIB_FILE}.rc")
    file(COPY "${CWORK_SOLUTION_PATH}/_img/templates/resources.h" DESTINATION "${CMAKE_CURRENT_SOURCE_DIR}/include/_generated")
    
    cwork_set_embedded_resource_dir("${CWORK_SOLUTION_PATH}/_img/resources")
    autodetect_source_files("${CMAKE_CURRENT_SOURCE_DIR}/src" "*.[hc]*")
    cwork_set_source_files(${CWORK_AUTODETECTED_FILES} "${__OUTPUT_LIB_DEF_DIR}/${_LIB_FILE}.rc" "${__OUTPUT_LIB_DEF_DIR}/${_LIB_FILE}.def")
    unset(CWORK_AUTODETECTED_FILES)
    unset(__OUTPUT_LIB_DEF_DIR)
endif()

set(__OUTPUT_LIB_INFO "${CMAKE_CURRENT_SOURCE_DIR}/include/_generated/library_info.h")
file(REMOVE "${__OUTPUT_LIB_INFO}")
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/templates/library_info.h.in" "${__OUTPUT_LIB_INFO}")
unset(__OUTPUT_LIB_INFO)
unset(_LIB_NAME)
unset(_RES_DIR)

# ┌──────────────────────────────────────────────────────────────────┐
# │  Project settings                                                │
# └──────────────────────────────────────────────────────────────────┘
if(WIN32 OR WIN64 OR _WIN32 OR _WIN64)
    unset(CWORK_EXPORT_SYMBOLS CACHE)
endif()
cwork_create_project("dynamic" "${CWORK_SOLUTION_PATH}/_libs/pandora_toolbox/_cmake" 
                     "${CWORK_SOLUTION_PATH}/_libs/pandora_toolbox/_cmake/modules"
                     "include" "src" "test")
set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME ${_LIB_FILE})

if(MSVC AND (WIN32 OR WIN64 OR _WIN32 OR _WIN64) AND NOT CMAKE_GENERATOR STREQUAL "Ninja")
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                       COMMAND ${CMAKE_COMMAND} -E copy_if_different
                               ${CMAKE_BINARY_DIR}/${_PROJECT_TITLE}/$<CONFIG>/${_LIB_FILE}.dll
                               ${CMAKE_BINARY_DIR}/$<CONFIG>/${_LIB_FILE}.dll)
endif()
unset(_LIB_FILE)
