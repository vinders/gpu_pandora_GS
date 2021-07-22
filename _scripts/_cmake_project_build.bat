@echo off

:: Identify CMake target
if not exist CMakeLists.txt cd ..
if not exist CMakeLists.txt (echo CMakeLists.txt not found in current directory nor in parent directory! && exit /B 1)
if "%1" == "" (echo Missing argument to identify platform (name of platform) && exit /B 1)

:: Create build directory
if not exist ./_build mkdir _build

:: Generate per platform
:loop
if not "%1" == "" (
    cd _build
    if exist "./%1" rmdir /S /Q "%1"
    mkdir "%1"
    cd ..

    if "%1" == "vs2017-win7" cmake -G "Visual Studio 15 2017" -A Win32 -S . -B "./_build/%1" -DCWORK_CPP_REVISION="14" -DCWORK_WINDOWS_VERSION="7" -DCWORK_OPENGL4_VERSION="45" -DCWORK_D3D11_VERSION="110"
    if "%1" == "vs2019-T2017-win7" cmake -G "Visual Studio 16 2019" -A Win32 -T v141 -S . -B "./_build/%1" -DCWORK_WINDOWS_VERSION="7" -DCWORK_OPENGL4_VERSION="45" -DCWORK_D3D11_VERSION="110"
    if "%1" == "vs2019-win10" cmake -G "Visual Studio 16 2019" -A Win32 -S . -B "./_build/%1" -DCWORK_WINDOWS_VERSION="10" -DCWORK_OPENGL4_VERSION="46" -DCWORK_D3D11_VERSION="114"
    if "%1" == "vs2019-win10-clang" cmake -G "Visual Studio 16 2019" -A Win32 -T ClangCL -S . -B "./_build/%1" -DCMAKE_C_COMPILER=clang-cl -DCMAKE_CXX_COMPILER=clang-cl -DCMAKE_FLAGS="-m32" -DCWORK_WINDOWS_VERSION="10" -DCWORK_OPENGL4_VERSION="46" -DCWORK_D3D11_VERSION="114"
    shift
    goto loop
)

exit /B %errorlevel%
