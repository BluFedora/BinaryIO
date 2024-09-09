/******************************************************************************/
/*!
 * @file   binary_types.hpp
 * @author Shareef Abdoul-Raheem (https://blufedora.github.io/)
 * @date   2021-09-19
 * @brief
 *   Lightweight header defining the types maintained by this library.
 *
 * @copyright Copyright (c) 2024 Shareef Abdoul-Raheem
 */
/******************************************************************************/
#ifndef BINARYIO_TYPE_HPP
#define BINARYIO_TYPE_HPP

#include <cstdint>  // uint64_t, int64_t, uint32_t, uint8_t

namespace binaryIO
{
  using IOSize   = std::uint64_t;  //!< Size type used for IO operations.
  using IOOffset = std::int64_t;   //!< Type used for seek operations.

  using VersionType = std::uint16_t;
  using ChunkTypeID = std::uint32_t;

  /*!
   * @brief
   *   Listing of error codes that could happen from an IO operation.
   */
  enum class IOErrorCode : std::uint8_t
  {
    Success,            //!< No error occurred.
    EndOfStream,        //!< No more data in stream.
    AllocationFailure,  //!< Failed to allocate memory for internal stream operations.
    ReadError,          //!< Failed to get more data from stream.
    SeekError,          //!< Invalid seek location.
    InvalidData,        //!< Parse error.
    InvalidOperation,   //!< Invalid Operation
    UnknownError,       //!< Unknown failure.
  };

  /*!
   * @brief
   *   Not all `IByteReader` support seeking, check the return value of IByteReader::seek.
   */
  enum class SeekOrigin : std::uint8_t
  {
    BEGIN,
    CURRENT,
    END,
  };

  struct IOResult
  {
    IOSize value;

    constexpr IOResult(const IOSize valid_value, const IOErrorCode error_code) :
      value{valid_value << 3 | IOSize(error_code)}
    {
    }

    constexpr IOResult(const IOErrorCode error_code) :
      IOResult(IOSize(0), error_code)
    {
    }

    constexpr IOResult(const IOSize valid_value) :
      IOResult(valid_value, IOErrorCode::Success)
    {
    }

    constexpr IOErrorCode ErrorCode() const { return IOErrorCode(value & 0x7); }
    constexpr IOSize      Value() const { return IOSize(value) >> 3; }
  };

  // Stream Types

  struct BufferedIO;
  struct IOStream;

  // Chunk Types

  struct BinaryChunkHeader;
  struct BinaryChunkTypeID;
  enum class BinaryChunkParts : std::uint32_t;

}  // namespace binaryIO

#endif /* BINARYIO_TYPE_HPP */

/******************************************************************************/
/*
  MIT License

  Copyright (c) 2024 Shareef Abdoul-Raheem

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/
/******************************************************************************/
