/******************************************************************************/
/*!
 * @file   binary_stream.hpp
 * @author Shareef Abdoul-Raheem (https://blufedora.github.io/)
 * @date   2021-09-19
 * @brief
 *   Abstract interface for reading and writing binary data.
 *
 *   References:
 *              [AMD GPUOpen Driver](https://github.com/GPUOpen-Drivers/pal/blob/477c8e78bc4f8c7f8b4cd312e708935b0e04b1cc/shared/gpuopen/inc/util/ddByteWriter.h#L40-L65)
 *       [Buffer-centric IO Article](https://fgiesen.wordpress.com/2011/11/21/buffer-centric-io/)
 *     [Small Writeup On Byte Order](https://commandcenter.blogspot.com/2012/04/byte-order-fallacy.html)
 *             [Int C Promotion FAQ](http://c-faq.com/expr/preservingrules.html)
 *
 * @copyright Copyright (c) 2021-2023 Shareef Abdoul-Raheem
 */
/******************************************************************************/
#ifndef BINARY_STREAM_HPP
#define BINARY_STREAM_HPP

#include <climits>  // CHAR_BIT
#include <cstddef>  // size_t
#include <cstdint>  // uint32_t, uint8_t

namespace assetio
{
  // ------------------ //
  //     Interfaces     //
  // ------------------ //

  /*!
   * @brief
   *   Listing of error that could happen from a read or write.
   */
  enum class IOResult : std::uint32_t
  {
    Success,            //!< No error occurred.
    EndOfStream,        //!< No more data in stream.
    AllocationFailure,  //!< Failed to allocate memory for internal stream operations.
    ReadError,          //!< Failed to get more data from stream.
    SeekError,          //!< Invalid seek location.
    InvalidData,        //!< Parse error.
    UnknownError,       //!< Unknown failure.
  };

  inline IOResult& operator+=(IOResult& lhs, const IOResult rhs)
  {
    if (rhs != IOResult::Success)
    {
      lhs = rhs;
    }

    return lhs;
  }

  /*!
   * @brief
   *   Interface for writing bytes.
   */
  struct IByteWriter
  {
    /*!
     * @brief
     *   Writes some bytes to the data stream.
     *
     * @param bytes
     *   The bytes to write to the stream, must be non-null.
     *
     * @param num_bytes
     *   The number of bytes to write to stream.
     *
     * @return
     *   `IOResult::Success` if no errors occurred otherwise an error code is returned.
     */
    virtual IOResult write(const void* const bytes, const size_t num_bytes) = 0;

    /*!
     * @brief
     *   Call when you want the stream flushed and closed.
     *
     * @return
     *   `IOResult::Success` if the stream was able to be closed.
     */
    virtual IOResult end() = 0;
    virtual ~IByteWriter() = default;
  };

  /*!
   * @brief
   *   Non-owning writer adaptor.
   */
  struct ByteWriterView final : public IByteWriter
  {
    /*!
     * @brief
     *   End of Stream Indicated by 'bytes == nullptr && num_bytes == 0u'.
     */
    using WriteFn = IOResult (*)(void* user_data, const void* bytes, size_t num_bytes);

    void*    user_data;
    WriteFn  callback;
    IOResult last_result;

    ByteWriterView(WriteFn callback, void* user_data = nullptr);

    IOResult write(const void* bytes, size_t num_bytes) final override;
    IOResult end() final override;

    ~ByteWriterView() override = default;
  };

  enum class SeekOrigin
  {
    BEGIN,
    CURRENT,
    END,
  };

  /*!
   * @brief
   *   Interface for reading bytes.
   */
  struct IByteReader
  {
    //  Pre-condition : cursor == buffer_end.
    // Post-condition : cursor == buffer_start, cursor < buffer_end.
    //         Return : last_result.
    using RefillFn = IOResult (*)(IByteReader* self);
    using SeekFn   = IOResult (*)(IByteReader* self, const std::size_t offset, const SeekOrigin origin);

    const uint8_t* buffer_start = nullptr;            //!< Start of buffer.
    const uint8_t* cursor       = nullptr;            //!< Invariant: buffer_start <= cursor <= buffer_end, user code sets this.
    const uint8_t* buffer_end   = nullptr;            //!< End of Buffer + 1, should not be read from.
    IOResult       last_result  = IOResult::Success;  //!< Initialized to `IOResult::Success`.
    RefillFn       refill_fn    = nullptr;            //!< Called when more data from the stream is needed.
    SeekFn         seek_fn      = nullptr;            //!< Called when `IByteReader::seek` is called, if null then seek will work on the local buffer.

    inline IOResult refill() { return refill_fn(this); }
    inline size_t   bufferSize() const { return buffer_end - buffer_start; }
    inline size_t   numBytesAvailable() const { return buffer_end - cursor; }
    IOResult        read(void* const dst_bytes, const std::size_t num_bytes, std::size_t* const out_num_bytes_written = nullptr);
    IOResult        seek(const std::size_t offset, const SeekOrigin origin = SeekOrigin::CURRENT);
    IOResult        setFailureState(IOResult err);

    static IByteReader fromBuffer(const uint8_t* buffer, const size_t buffer_size);
  };

  // ------------------ //
  // Endianess Handling //
  // ------------------ //

  namespace detail
  {
    template<typename T, typename F>
    IOResult writeXEndian(IByteWriter& writer, const T value, F&& convertIndex) noexcept
    {
      static_assert(std::is_integral_v<T>, "Byte ordering is for integral types.");

      std::uint8_t bytes[sizeof(T)];
      for (std::size_t i = 0u; i < sizeof(T); ++i)
      {
        bytes[convertIndex(i)] = (value >> (i * CHAR_BIT)) & 0xFF;
      }

      return writer.write(bytes, sizeof(bytes));
    }

    template<typename T, typename F>
    IOResult readXEndian(IByteReader& reader, T* value, F&& convertIndex) noexcept
    {
      static_assert(std::is_integral_v<T>, "Byte ordering is for integral types.");

      std::uint8_t   bytes[sizeof(T)];
      const IOResult result = reader.read(bytes, sizeof(bytes));

      if (result == IOResult::Success)
      {
        *value = 0x0;

        for (std::size_t i = 0u; i < sizeof(T); ++i)
        {
          (*value) |= T(T(bytes[convertIndex(i)]) << (i * CHAR_BIT));
        }
      }

      return result;
    }
  }  // namespace detail

  template<typename T>
  T swapEndian(const T value) noexcept
  {
    static_assert(std::is_integral_v<T>, "Byte ordering is for integral types.");

    T                         result;
    const std::uint8_t* const src = reinterpret_cast<const std::uint8_t*>(&value);
    std::uint8_t* const       dst = reinterpret_cast<std::uint8_t*>(&result);

    for (std::size_t i = 0; i < sizeof(T); ++i)
    {
      dst[i] = src[sizeof(T) - i - 1];
    }

    return result;
  }

  template<typename T>
  IOResult writeLE(IByteWriter& writer, const T value) noexcept
  {
    return detail::writeXEndian(writer, value, [](const std::size_t i) { return i; });
  }

  template<typename T>
  IOResult writeBE(IByteWriter& writer, const T value) noexcept
  {
    return detail::writeXEndian(writer, value, [](const std::size_t i) { return sizeof(T) - i - 1; });
  }

  template<typename T>
  IOResult readLE(IByteReader& reader, T* const value) noexcept
  {
    return detail::readXEndian(reader, value, [](const std::size_t i) { return i; });
  }

  template<typename T>
  IOResult readBE(IByteReader& reader, T* const value) noexcept
  {
    return detail::readXEndian(reader, value, [](const std::size_t i) { return sizeof(T) - i - 1; });
  }
}  // namespace assetio

#endif /* BINARY_STREAM_HPP */

/******************************************************************************/
/*
  MIT License

  Copyright (c) 2021-2023 Shareef Abdoul-Raheem

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
