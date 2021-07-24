#*******************************************************************************
# Author  :     Romain Vinders
# License :     GPLv2
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
