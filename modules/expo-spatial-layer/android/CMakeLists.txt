cmake_minimum_required(VERSION 3.10)

project(ExpoSpatialLayer)

set(CMAKE_CXX_STANDARD 17)

# Paths
set(CPP_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../cpp)

add_library(
        expo-spatial-layer
        SHARED
        src/main/cpp/expo-spatial-layer.cpp
        ${CPP_DIR}/SpatialLayer.cpp
        ${CPP_DIR}/QuadTree.cpp
)

target_include_directories(
        expo-spatial-layer
        PUBLIC
        ${CPP_DIR}
)

# Find JSI via Prefab
find_package(ReactAndroid REQUIRED CONFIG)

target_link_libraries(
        expo-spatial-layer
        ReactAndroid::jsi
        log
)
