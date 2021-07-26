#!/bin/bash
if ! [ -e "./CMakeLists.txt" ]; then
  cd ..
  if ! [ -e "./CMakeLists.txt" ]; then
    cd ..
  fi
fi

if [ -z "$1" ]; then
  PLATFORM="linux"
else
  PLATFORM="$1"
fi

# -- project dependencies --

# gtest + libs directory
if ! [ -e "./_libs/gtest/CMakeLists.txt" ]; then
  git submodule update --init --recursive --remote
fi
cd "./_libs"

exit 0
