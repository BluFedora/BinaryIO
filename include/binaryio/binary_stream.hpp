/******************************************************************************/
/*!
 * @file   binary_stream.hpp
 * @author Shareef Abdoul-Raheem (https://blufedora.github.io/)
 * @date   2021-09-19
 * @brief
 *   Abstract interface for reading and writing binary data.
 *
 *   References:
 *     [The byte order fallacy](https://commandcenter.blogspot.com/2012/04/byte-order-fallacy.html)
 *        [Int C Promotion FAQ](http://c-faq.com/expr/preservingrules.html)
 *
 * @copyright Copyright (c) 2021-2024 Shareef Abdoul-Raheem
 */
/******************************************************************************/
#ifndef BINARY_STREAM_HPP
#define BINARY_STREAM_HPP

#include "binary_types.hpp"

#include <climits>      // CHAR_BIT
#include <type_traits>  // underlying_type_t, is_enum_v, is_integral_v

namespace binaryIO
{
  /*!
   * @brief
   *   Allows for reduced amount of memory copies by allowing
   *   the stream to expose it's internal buffer.
   *
   *   Check if the stream supports this through `IOSteam_SupportsBufferedRead`.
   *
   *   Based on the ideas in [Buffer-centric IO](https://fgiesen.wordpress.com/2011/11/21/buffer-centric-io/).
   */
  struct BufferedIO
  {
    const uint8_t* buffer_start = nullptr;  //!< Start of buffer.
    const uint8_t* cursor       = nullptr;  //!< Invariant: buffer_start <= cursor <= buffer_end, user code sets this.
    const uint8_t* buffer_end   = nullptr;  //!< End of Buffer + 1, should not be read from.

    /*!
     * @brief
     *    Pre-condition : cursor == buffer_end.
     *   Post-condition : cursor == buffer_start and cursor < buffer_end.
     *           Return : Error code state of the stream.
     */
    IOErrorCode (*Refill)(IOStream* const stream) = nullptr;
  };

  union StreamUserDataValue
  {
    void*  as_handle;
    IOSize as_size;
  };

  struct MemoryStreamData
  {
    void*  buffer_start;
    IOSize cursor;
    IOSize buffer_size;

    IOSize BytesLeft() const { return buffer_size - cursor; }
    void*  CursorBytes() const { return (unsigned char*)buffer_start + cursor; }
  };

  union IOStreamUserData
  {
    MemoryStreamData    memory_stream;
    StreamUserDataValue values[3];
  };

  /*!
   * @brief
   *   Interface for reading and writing bytes to an abstract stream object.
   */
  struct IOStream
  {
    /* Abstract Interface */

    IOResult    (*Size)(IOStream* const stream)                                                              = nullptr;
    IOResult    (*Read)(IOStream* const stream, void* const destination, const IOSize num_destination_bytes) = nullptr;
    IOResult    (*Write)(IOStream* const stream, const void* const source, const IOSize num_source_bytes)    = nullptr;
    IOResult    (*Seek)(IOStream* const stream, const IOOffset offset, const SeekOrigin seek_origin)         = nullptr;
    IOErrorCode (*Close)(IOStream* const stream)                                                             = nullptr;

    /* Data Members */

    IOStreamUserData user_data   = {};
    BufferedIO       buffered_io = {};
    IOErrorCode      error_state = IOErrorCode::Success;
  };

  // IO Stream API

  bool IOSteam_SupportsRead(const IOStream* const stream);
  bool IOSteam_SupportsWrite(const IOStream* const stream);
  bool IOSteam_SupportsBufferedRead(const IOStream* const stream);
  bool IOSteam_SupportsSeek(const IOStream* const stream);

  IOStream IOStream_FromRWMemory(void* const bytes, const IOSize num_bytes);
  IOStream IOStream_FromROMemory(const void* const bytes, const IOSize num_bytes);

  IOErrorCode IOStream_ResetErrorState(IOStream* const stream);
  IOResult    IOStream_Size(IOStream* const stream);
  IOResult    IOStream_Read(IOStream* const stream, void* const destination, const IOSize num_destination_bytes);
  IOResult    IOStream_Write(IOStream* const stream, const void* const source, const IOSize num_source_bytes);
  IOResult    IOStream_Seek(IOStream* const stream, const IOOffset offset, const SeekOrigin seek_origin);
  IOErrorCode IOStream_Close(IOStream* const stream);

  // Buffered IO API

  IOSize      BufferedIO_NumBytesAvailable(const IOStream* const stream);
  IOErrorCode BufferedIO_Refill(IOStream* const stream);
  IOResult    BufferedIO_Read(IOStream* const stream, void* const destination, const IOSize num_destination_bytes);
  IOErrorCode BufferedIO_Failure(IOStream* const stream, const IOErrorCode error_code);

  // Helpers for Making New IO Streams

  binaryIO::IOResult MemoryStream_CopyBytes(
   void* const             destination,
   const binaryIO::IOSize  num_destination_bytes,
   const void* const       source,
   const binaryIO::IOSize  num_source_bytes,
   const binaryIO::IOSize  desired_number_of_bytes,
   binaryIO::IOSize* const in_out_cursor);

  // Endianess Handling

  namespace detail
  {
    template<typename T, bool is_enum>
    struct UnderlyingTypeImpl;

    template<typename T>
    struct UnderlyingTypeImpl<T, true>
    {
      using type = std::underlying_type_t<T>;
    };

    template<typename T>
    struct UnderlyingTypeImpl<T, false>
    {
      using type = T;
    };

    template<typename T>
    using UnderlyingType = typename UnderlyingTypeImpl<T, std::is_enum_v<T>>::type;

    template<typename T, typename F>
    IOResult writeXEndian(IOStream* const stream, const T value, F&& convertIndex) noexcept
    {
      static_assert(std::is_integral_v<T> || std::is_enum_v<T>, "Byte ordering is for integral types.");

      std::uint8_t bytes[sizeof(T)];
      for (std::size_t i = 0u; i < sizeof(T); ++i)
      {
        bytes[convertIndex(i)] = (static_cast<UnderlyingType<T>>(value) >> (i * CHAR_BIT)) & 0xFF;
      }

      return IOStream_Write(stream, bytes, sizeof(bytes));
    }

    template<typename T, typename F>
    IOResult readXEndian(IOStream* const stream, T* out_value, F&& convertIndex) noexcept
    {
      static_assert(std::is_integral_v<T> || std::is_enum_v<T>, "Byte ordering is for integral types.");

      std::uint8_t      bytes[sizeof(T)];
      const IOErrorCode result = IOStream_Read(stream, bytes, sizeof(bytes)).ErrorCode();

      if (result == IOErrorCode::Success)
      {
        using UT = UnderlyingType<T>;

        UT value = 0x0;

        for (std::size_t i = 0u; i < sizeof(T); ++i)
        {
          value |= UT(UT(bytes[convertIndex(i)]) << (i * CHAR_BIT));
        }

        *out_value = static_cast<T>(value);
      }

      return result;
    }
  }  // namespace detail

  template<typename T>
  IOResult writeLE(IOStream* const stream, const T value) noexcept
  {
    return detail::writeXEndian(stream, value, [](const std::size_t i) { return i; });
  }

  template<typename T>
  IOResult writeBE(IOStream* const stream, const T value) noexcept
  {
    return detail::writeXEndian(stream, value, [](const std::size_t i) { return sizeof(T) - i - 1; });
  }

  template<typename T>
  IOResult readLE(IOStream* const stream, T* const value) noexcept
  {
    return detail::readXEndian(stream, value, [](const std::size_t i) { return i; });
  }

  template<typename T>
  IOResult readBE(IOStream* const stream, T* const value) noexcept
  {
    return detail::readXEndian(stream, value, [](const std::size_t i) { return sizeof(T) - i - 1; });
  }

}  // namespace binaryIO

#endif /* BINARY_STREAM_HPP */

/******************************************************************************/
/*
  MIT License

  Copyright (c) 2021-2024 Shareef Abdoul-Raheem

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
