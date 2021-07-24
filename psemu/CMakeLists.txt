#*******************************************************************************
# Author  :     Romain Vinders
# License :     GPLv2
# ------------------------------------------------------------------------------
# Display pipeline and rendering
#*******************************************************************************
cmake_minimum_required(VERSION 3.14)
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
set(_LIB_NAME "Pandora-GS GPU driver")

if(WIN32 OR WIN64 OR _WIN32 OR _WIN64)
    if(CWORK_WINDOWS_VERSION AND CWORK_WINDOWS_VERSION STREQUAL "7")
        set(_LIB_FILE ${_LIB_FILE}_win7)
        set(_LIB_NAME "${_LIB_NAME} (Win7)")
    endif()

    set(__OUTPUT_LIB_DEF "${CMAKE_CURRENT_SOURCE_DIR}/_generated/resources/${_LIB_FILE}.def")
    file(REMOVE "${__OUTPUT_LIB_DEF}")
    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/templates/gpuPandoraGS.def.in" "${__OUTPUT_LIB_DEF}")
    cwork_set_embedded_resource_dir("${CMAKE_CURRENT_SOURCE_DIR}/_generated/resources")
    unset(__OUTPUT_LIB_DEF)
endif()

set(__OUTPUT_LIB_INFO "${CMAKE_CURRENT_SOURCE_DIR}/include/_generated/library_info.h")
file(REMOVE "${__OUTPUT_LIB_INFO}")
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/templates/library_info.h.in" "${__OUTPUT_LIB_INFO}")
unset(__OUTPUT_LIB_INFO)
unset(_LIB_NAME)

# ┌──────────────────────────────────────────────────────────────────┐
# │  Project settings                                                │
# └──────────────────────────────────────────────────────────────────┘
if(WIN32 OR WIN64 OR _WIN32 OR _WIN64)
    cwork_set_embedded_resource_dir("${CMAKE_CURRENT_SOURCE_DIR}/_generated/resources")
    unset(CWORK_EXPORT_SYMBOLS CACHE)
endif()

cwork_create_project("dynamic" "${CWORK_SOLUTION_PATH}/_libs/pandora_toolbox/_cmake" 
                     "${CWORK_SOLUTION_PATH}/_libs/pandora_toolbox/_cmake/modules"
                     "include" "src" "test")

if(WIN32 OR WIN64 OR _WIN32 OR _WIN64)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                       COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/${_PROJECT_TITLE}/$<CONFIG>/${PROJECT_NAME}.dll ${CMAKE_BINARY_DIR}/$<CONFIG>/${_LIB_FILE}.dll)
endif()
unset(_LIB_FILE)
