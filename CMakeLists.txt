# 
# BinaryIO by Shareef Raheem (@blufedora)
#
cmake_minimum_required(VERSION 3.23)

project(AssetIO_BinaryIO VERSION 1.0.0 DESCRIPTION "Helper Binary IO Operations For C++17 and above." LANGUAGES CXX)

add_library(
  AssetIO_BinaryIO STATIC

    # Headers
      "include/assetio/binary_assert.hpp"
      "include/assetio/binary_stream.hpp"
      "include/assetio/binary_stream_ext.hpp"
      "include/assetio/byte_swap.hpp"
      "include/assetio/rel_ptr.hpp"

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
