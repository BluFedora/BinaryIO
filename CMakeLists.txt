# 
# BinaryIO by Shareef Raheem (@blufedora)
#
cmake_minimum_required(VERSION 3.23)

project(AssetIO_BinaryIO VERSION 1.0.0 DESCRIPTION "Helper Binary IO Operations For C++17 and above." LANGUAGES CXX)

add_library(
  AssetIO_BinaryIO STATIC

    # Headers
      "include/binaryio/binary_chunk.hpp"
      "include/binaryio/binary_stream.hpp"
      "include/binaryio/binary_stream_ext.hpp"
      "include/binaryio/binary_types.hpp"
      "include/binaryio/rel_ptr.hpp"

    # Sources
      "src/binary_io.cpp"
)

set_property(TARGET AssetIO_BinaryIO PROPERTY CXX_STANDARD 17)

target_include_directories(
  AssetIO_BinaryIO

  PUBLIC
    "${PROJECT_SOURCE_DIR}/include"
)

set_property(TARGET AssetIO_BinaryIO PROPERTY FOLDER "BluFedora/AssetIO")
