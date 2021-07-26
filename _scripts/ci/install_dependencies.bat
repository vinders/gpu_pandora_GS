@echo off

if not exist CMakeLists.txt cd ..
if not exist CMakeLists.txt cd ..
if not exist CMakeLists.txt (echo CMakeLists.txt not found in current directory nor in parent directory! && exit /B 1)

:: gtest + libs directory
if not exist ./_libs/gtest/CMakeLists.txt call git submodule update --init --recursive --remote
cd _libs

cd ..
exit /B 0
