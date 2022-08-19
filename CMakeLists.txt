################################################################################
#                                                                              #
#                            BF BINARY IO PROJECT                              #
#                                                                              #
################################################################################
###     Abstract interface for Binary IO Operations For C++11 and above.     ###
################################################################################

cmake_minimum_required(VERSION 3.23)

project(AssetIO_BinaryIO)

add_library(
  AssetIO_BinaryIO STATIC

    # Headers
      "include/assetio/binary_assert.hpp"
      "include/assetio/binary_stream.hpp"
      "include/assetio/binary_stream_ext.hpp"
      "include/assetio/rel_ptr.hpp"

    # Sources
      "src/binary_io.cpp"
)

target_include_directories(
  AssetIO_BinaryIO

  PUBLIC
    "${PROJECT_SOURCE_DIR}/include"
)

set_property(TARGET AssetIO_BinaryIO PROPERTY FOLDER "BluFedora/AssetIO")
