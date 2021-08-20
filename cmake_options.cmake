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
if(NOT DEFINED _PANDORAGS_OPTIONS_FOUND)
    set(_PANDORAGS_OPTIONS_FOUND ON)

    # ┌──────────────────────────────────────────────────────────────────┐
    # │  Options                                                         │
    # └──────────────────────────────────────────────────────────────────┘
    
    # Windows systems - Direct3D11+Vulkan / Direct3D11+OpenGL4
    if(WIN32 OR WIN64 OR _WIN32 OR _WIN64 OR CMAKE_SYSTEM_NAME STREQUAL "Windows")
        if(NOT DEFINED OPTION_MIN_WINDOWS_VERSION)
            # compiled on Windows 7 / 8 -> force Win7 version
            if("${CMAKE_SYSTEM_VERSION}" EQUAL 6.1 OR "${CMAKE_SYSTEM_VERSION}" EQUAL 6.2 OR MINGW)
                set(OPTION_MIN_WINDOWS_VERSION "7" CACHE STRING "min required Windows version ('7', '10' (RS2))")
            # compiled on Windows 10+ -> option available
            else()
                set(OPTION_MIN_WINDOWS_VERSION "10" CACHE STRING "minimum required Windows version ('7', '10' (RS2))")
                set_property(CACHE OPTION_MIN_WINDOWS_VERSION PROPERTY STRINGS "10" "7") # possible values for GUI
            endif()
        endif()
        
        # Windows 7 / 8
        if(OPTION_MIN_WINDOWS_VERSION STREQUAL "7")
            set(CWORK_WINDOWS_VERSION "7" CACHE INTERNAL "" FORCE)  # use retrocompatible Win32 API -> missing new features + slower API
            set(CWORK_D3D11_VERSION "110" CACHE INTERNAL "" FORCE)  # 11.0 (D3D 11.1 only partially supported on Win7)
            set(OPTION_VULKAN_RENDERER OFF CACHE BOOL "not supported") # disable Vulkan (poor support on older hardware)
        # Windows 10+
        else()
            set(CWORK_WINDOWS_VERSION "10" CACHE INTERNAL "" FORCE) # Windows 10+ required -> support advanced DPI settings + faster API + extra D3D features
            set(CWORK_D3D11_VERSION "114" CACHE INTERNAL "" FORCE)  # 11.4
            
            if("$ENV{VULKAN_SDK}" STREQUAL "") # only allow Vulkan if SDK is installed
                if(NOT DEFINED OPTION_VULKAN_RENDERER)
                    option(OPTION_VULKAN_RENDERER "use Vulkan renderer (instead of Direct3D11)" OFF)
                endif()
            else()
                set(OPTION_VULKAN_RENDERER OFF CACHE BOOL "not supported (env VULKAN_SDK not defined)")
            endif()
        endif()
        
        if(OPTION_VULKAN_RENDERER)
            set(CWORK_VIDEO_D3D11 OFF CACHE BOOL "")
            set(CWORK_VIDEO_VULKAN ON CACHE BOOL "")
        else()
            set(CWORK_VIDEO_D3D11 ON CACHE BOOL "")
            set(CWORK_VIDEO_VULKAN OFF CACHE BOOL "")
        endif()
        set(CWORK_VIDEO_OPENGL4 OFF CACHE BOOL "not supported")
    
    # Linux/MacOS systems - OpenGL4+Vulkan
    else()
        if(NOT APPLE AND "$ENV{VULKAN_SDK}" STREQUAL "")
            message(FATAL_ERROR "Vulkan SDK was not found on current system (env VULKAN_SDK not defined)")
        endif()
        
        set(CWORK_VIDEO_D3D11 OFF CACHE BOOL "not supported")
        set(CWORK_VIDEO_OPENGL4 OFF CACHE BOOL "not supported")
        set(CWORK_VIDEO_VULKAN ON CACHE BOOL "")
    endif()
endif()
