#*******************************************************************************
# Author  :     Romain Vinders
# License :     MIT
#*******************************************************************************
cmake_minimum_required(VERSION 3.14)
include("${CMAKE_CURRENT_SOURCE_DIR}/../../../_libs/pandora_toolbox/_cmake/cwork.cmake")
cwork_set_default_solution("gpu_pandora_GS" "${CMAKE_CURRENT_SOURCE_DIR}/../../..")
cwork_read_version_from_file("${CMAKE_CURRENT_SOURCE_DIR}/../../../build_version.txt" OFF)
project("${CWORK_SOLUTION_NAME}.primitive_viewer" VERSION ${CWORK_BUILD_VERSION} LANGUAGES C CXX)

# ┌──────────────────────────────────────────────────────────────────┐
# │  Dependencies                                                    │
# └──────────────────────────────────────────────────────────────────┘
cwork_set_internal_libs(display)

# ┌──────────────────────────────────────────────────────────────────┐
# │  Project settings                                                │
# └──────────────────────────────────────────────────────────────────┘
if(WIN32 OR WIN64 OR _WIN32 OR _WIN64)
    cwork_set_embedded_resource_dir("${CWORK_SOLUTION_PATH}/_img/resources_win32")
else()
    cwork_set_resource_dir("${CWORK_SOLUTION_PATH}/_img/resources_unix")
endif()

cwork_set_subproject_type("tools")
cwork_create_project("executable" "${CWORK_SOLUTION_PATH}/_libs/pandora_toolbox/_cmake"
                     "${CWORK_SOLUTION_PATH}/_libs/pandora_toolbox/_cmake/modules"
                     "include" "src" "test")
