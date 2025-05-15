cmake_minimum_required(VERSION 3.12)
project(onnx_inference_project)

set(CMAKE_CXX_STANDARD 17)
option(BUILD_SHARED_LIBS "Whether to build shared libraries" OFF)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

# 查找ONNX Runtime
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
include(onnxruntime)
include(cppjieba)
set(ONNXRUNTIME_DIR ${onnxruntime_SOURCE_DIR})
message(STATUS "ONNXRUNTIME_DIR: ${ONNXRUNTIME_DIR}")
message(STATUS "ONNXRUNTIME_INCLUDE_DIRS: ${ONNXRUNTIME_INCLUDE_DIRS}")
include_directories(${CMAKE_SOURCE_DIR})

# 可执行文件
add_executable(infer
    main.cc
    kokoro.cpp
    wave-writer.cc
)

# 链接库
target_link_libraries(infer
   cppjieba
   ${onnxruntime_lib_files} 
)
