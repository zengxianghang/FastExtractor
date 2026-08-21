cmake_minimum_required(VERSION 3.15)

project(FastExtractor LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(FastExtractor
    src/main.cpp
    src/RangeParser.cpp
    src/Extractor.cpp
    src/FastExtractByGPST.cpp
    src/GpstLocator.cpp
    src/FastScanner.cpp
    src/FileTimeDetector.cpp
    src/TailScanner.cpp
    src/MetaCache.cpp
)

target_include_directories(FastExtractor PRIVATE include)
