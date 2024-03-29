/******************************************************************************/
/*!
 * @file   binary_io.cpp
 * @author Shareef Abdoul-Raheem (https://blufedora.github.io/)
 * @date   2022-06-29
 * @brief
 *   Implementation of any code that doesnt belong in the header.
 *
 * @copyright Copyright (c) 2022-2023 Shareef Abdoul-Raheem
 */
/******************************************************************************/
#include "assetio/binary_assert.hpp"
#include "assetio/binary_chunk.hpp"
#include "assetio/binary_stream.hpp"
#include "assetio/binary_stream_ext.hpp"
#include "assetio/byte_swap.hpp"

#include <algorithm>  // rotate
#include <cstdio>     // fprintf, stderr
#include <cstdlib>    // abort
#include <cstring>    // memcpy
#include <iterator>   // begin, end
#include <utility>    // exchange

namespace assetio
{
  // binary_assert.hpp

  bool detail::binaryIOAssertImpl(const bool expr, const char* const expr_str, const char* const filename, const int line_number, const char* const assert_msg)
  {
    if (!expr)
    {
      std::fprintf(stderr, "BinaryIO[%s:%i] Assertion '%s' failed, %s.\n", filename, line_number, expr_str, assert_msg);
      std::abort();
    }

    return expr;
  }

  // binary_stream.hpp

  //// ByteWriterView

  ByteWriterView::ByteWriterView(WriteFn callback, void* user_data) :
    user_data{user_data},
    callback{callback},
    last_result{IOResult::Success}
  {
  }

  IOResult ByteWriterView::write(const void* bytes, size_t num_bytes)
  {
    if (bytes != nullptr && num_bytes != 0u && last_result == IOResult::Success)
    {
      last_result = callback(user_data, bytes, num_bytes);
    }

    return last_result;
  }

  IOResult ByteWriterView::end()
  {
    if (last_result == IOResult::Success)
    {
      last_result = callback(user_data, nullptr, 0u);
    }

    // Reset the status of last_result.
    return std::exchange(last_result, IOResult::Success);
  }

  //// IByteReader

  IOResult IByteReader::read(void* const dst_bytes, const std::size_t num_bytes, std::size_t* const out_num_bytes_written)
  {
    unsigned char* const       write_bgn    = static_cast<unsigned char*>(dst_bytes);
    const unsigned char* const write_end    = write_bgn + num_bytes;
    unsigned char*             write_cursor = write_bgn;

    while (write_cursor != write_end)
    {
      if (cursor == buffer_end)
      {
        refill();
      }

      if (last_result != IOResult::Success)
      {
        break;
      }

      const std::size_t num_bytes_left_to_read      = write_end - write_cursor;
      const std::size_t num_bytes_available_to_read = numBytesAvailable();
      const std::size_t num_bytes_able_to_read      = std::min(num_bytes_left_to_read, num_bytes_available_to_read);

      std::memcpy(write_cursor, cursor, num_bytes_able_to_read);

      write_cursor += num_bytes_able_to_read;
      cursor += num_bytes_able_to_read;
    }

    if (out_num_bytes_written)
    {
      *out_num_bytes_written = write_cursor - write_bgn;
    }

    return last_result;
  }

  IOResult IByteReader::seek(const std::size_t offset, const SeekOrigin origin)
  {
    if (seek_fn)
    {
      return seek_fn(this, offset, origin);
    }

    const uint8_t* destination;
    switch (origin)
    {
      case SeekOrigin::BEGIN:
      {
        destination = buffer_start + offset;
        break;
      }
      case SeekOrigin::CURRENT:
      {
        destination = cursor + offset;
        break;
      }
      case SeekOrigin::END:
      {
        destination = buffer_end - offset;
        break;
      }
      default:
      {
        binaryIOAssert(false, "Invalid seek origin.\n");
        break;
      }
    }

    if (buffer_start <= destination && destination <= buffer_end)
    {
      cursor = destination;
      return IOResult::Success;
    }

    return IOResult::SeekError;
  }

  IOResult IByteReader::setFailureState(IOResult err)
  {
    last_result = err;
    refill_fn   = [](IByteReader* self) {
      static const uint8_t s_ZeroBuffer[16] = {0};  // If the size is set to 1 then refill will be called more often but that is still valid.

      self->buffer_start = std::begin(s_ZeroBuffer);
      self->cursor       = self->buffer_start;
      self->buffer_end   = std::end(s_ZeroBuffer);

      return self->last_result;
    };

    return refill();
  }

  IByteReader IByteReader::fromBuffer(const uint8_t* buffer, const size_t buffer_size)
  {
    IByteReader result = {};

    result.buffer_start = buffer;
    result.cursor       = buffer;
    result.buffer_end   = buffer + buffer_size;
    result.last_result  = IOResult::Success;
    result.refill_fn    = [](IByteReader* const self) -> IOResult {
      // A memory stream cannot be refilled.
      return self->setFailureState(IOResult::EndOfStream);
    };

    return result;
  }

  // binary_stream_ext.hpp

  ByteWriterView byteWriterViewFromFile(std::FILE* const file_handle)
  {
    binaryIOAssert(file_handle != nullptr, "Invalid file handle.");

    return ByteWriterView{
     [](void* user_data, const void* bytes, size_t num_bytes) -> IOResult {
       std::FILE* const file = static_cast<std::FILE*>(user_data);

       if (bytes != nullptr && num_bytes != 0u)
       {
         fwrite(bytes, sizeof(uint8_t), num_bytes, file);
       }
       else
       {
         fflush(file);
       }

       return IOResult::Success;
     },
     file_handle};
  }

  ByteWriterView byteWriterViewFromBuffer(Buffer* const buffer)
  {
    return ByteWriterView{
     [](void* user_data, const void* bytes, size_t num_bytes) -> IOResult {
       Buffer* const buffer = static_cast<Buffer*>(user_data);

       if ((buffer->written + num_bytes) <= buffer->capacity)
       {
         std::memcpy(static_cast<unsigned char*>(buffer->ptr) + buffer->written, bytes, num_bytes);
         buffer->written += num_bytes;
         return IOResult::Success;
       }

       return IOResult::EndOfStream;
     },
     buffer};
  }

  //// CFileBufferedByteReader

  CFileBufferedByteReader::CFileBufferedByteReader(FILE* const file_handle) :
    IByteReader(),
    m_FileHandle{file_handle},
    m_LocalBuffer{}
  {
    binaryIOAssert(file_handle != nullptr, "Invalid file handle.");

    buffer_start = m_LocalBuffer;
    cursor       = m_LocalBuffer;
    buffer_end   = m_LocalBuffer;
    last_result  = IOResult::Success;
    refill_fn    = [](IByteReader* self_) -> IOResult {
      CFileBufferedByteReader* const self = static_cast<CFileBufferedByteReader*>(self_);

      if (std::feof(self->m_FileHandle))
      {
        return self->setFailureState(IOResult::EndOfStream);
      }

      std::uint8_t* const data_bgn            = const_cast<std::uint8_t*>(self->buffer_start);
      std::uint8_t* const read_end            = const_cast<std::uint8_t*>(self->cursor);
      std::uint8_t* const data_end            = const_cast<std::uint8_t*>(self->buffer_end);
      std::uint8_t* const write_start         = std::rotate(data_bgn, read_end, data_end);
      const std::size_t   num_bytes_in_buffer = std::end(self->m_LocalBuffer) - write_start;
      const std::size_t   num_bytes_read      = std::fread(write_start, sizeof(write_start[0]), num_bytes_in_buffer, self->m_FileHandle);

      if (num_bytes_read != 0 || num_bytes_in_buffer == 0u)
      {
        self->buffer_start = data_bgn;
        self->cursor       = self->buffer_start;
        self->buffer_end   = write_start + num_bytes_read;

        return IOResult::Success;
      }
      else if (std::feof(self->m_FileHandle))
      {
        return self->setFailureState(IOResult::EndOfStream);
      }
      else if (std::ferror(self->m_FileHandle))
      {
        return self->setFailureState(IOResult::ReadError);
      }
      else
      {
        return self->setFailureState(IOResult::UnknownError);
      }
    };
    seek_fn = [](IByteReader* self_, const std::size_t offset, const SeekOrigin origin) -> IOResult {
      CFileBufferedByteReader* const self = static_cast<CFileBufferedByteReader*>(self_);

      const int file_origin = [origin]() -> int {
        switch (origin)
        {
          case SeekOrigin::BEGIN: return SEEK_SET;
          case SeekOrigin::CURRENT: return SEEK_CUR;
          case SeekOrigin::END: return SEEK_END;
          default:
          {
            binaryIOAssert(false, "Invalid seek origin.\n");
            break;
          }
        }

        return -1;
      }();

#if _WIN32
      if (_fseeki64(self->m_FileHandle, offset, file_origin) == 0)
#else
      if (fseeko(self->m_FileHandle, offset, file_origin) == 0)
#endif
      {
        self->buffer_start = self->m_LocalBuffer;
        self->cursor       = self->m_LocalBuffer;
        self->buffer_end   = self->m_LocalBuffer;
        return self->refill_fn(self);
      }

      return IOResult::SeekError;
    };
  }
}  // namespace assetio

/******************************************************************************/
/*
  MIT License

  Copyright (c) 2022-2023 Shareef Abdoul-Raheem

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
