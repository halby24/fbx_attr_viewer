# Copyright 2023 HALBY
# This program is distributed under the terms of the MIT License. See the file LICENSE for details.
cmake_minimum_required(VERSION 3.20)

set(FBX_TARGET_NAME FbxAttrViewer)
set(FBX_TARGET_SOURCE
    src/main.cpp
)

set(CMAKE_CXX_STANDARD 20)
set(FBX_SDK_PATH "" CACHE PATH "Path to the FBX SDK")
set(FBX_LIB_DIR "${FBX_SDK_PATH}/lib/vs2022/x64/debug")

project(${FBX_TARGET_NAME})
add_executable(${FBX_TARGET_NAME} ${FBX_TARGET_SOURCE})
target_include_directories(${FBX_TARGET_NAME} PRIVATE "${FBX_SDK_PATH}/include")
target_link_libraries(${FBX_TARGET_NAME} PRIVATE "${FBX_LIB_DIR}/libfbxsdk.lib")

