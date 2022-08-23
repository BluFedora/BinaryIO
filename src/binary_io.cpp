/******************************************************************************/
/*!
 * @file   binary_io.cpp
 * @author Shareef Abdoul-Raheem (https://blufedora.github.io/)
 * @date   2022-06-29
 * @brief
 *   Implementation of any code that doesnt belong in the header.
 *
 * @copyright Copyright (c) 2022
 */
/******************************************************************************/
#include "assetio/binary_assert.hpp"
#include "assetio/binary_chunk.hpp"
#include "assetio/binary_stream.hpp"
#include "assetio/binary_stream_ext.hpp"

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
      std::fprintf(stderr, "RelPtr [%s:%i] Assertion '%s' failed, %s.\n", filename, line_number, expr_str, assert_msg);
      std::abort();
    }

    return expr;
  }

  // binary_chunk

  void ChunkUtils::writeChecksum(BinaryChunkHeader* chunk_header)
  {
    const char* const        data   = static_cast<const char*>(chunk_header->data());
    BinaryChunkFooter* const footer = chunk_header->footer();

    footer->crc32_checksum = crc32(data, chunk_header->data_size, k_crc32InitialSeed);
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
    binaryIOAssert(bytes != nullptr && num_bytes > 0, "Write must be called with a valid set of bytes.");

    if (last_result == IOResult::Success)
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

  IOResult IByteReader::read(void* const dst_bytes, const std::size_t num_bytes)
  {
    char*             write_cursor = static_cast<char*>(dst_bytes);
    const char* const write_end    = write_cursor + num_bytes;

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

    return last_result;
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

  ByteWriterView byteWriterViewFromVector(std::vector<uint8_t>* const buffer)
  {
    return ByteWriterView(
     [](void* user_data, const void* bytes, size_t num_bytes) -> IOResult {
       try
       {
         std::vector<uint8_t>* const buffer           = static_cast<std::vector<uint8_t>*>(user_data);
         const bool                  is_end_of_stream = bytes == nullptr && num_bytes == 0u;

         if (!is_end_of_stream)
         {
           const uint8_t* const typed_bytes = static_cast<const uint8_t*>(bytes);

           buffer->insert(buffer->end(), typed_bytes, typed_bytes + num_bytes);
         }
         else
         {
           // End of Stream, No work needed for Vector.
         }

         return IOResult::Success;
       }
       catch (const std::bad_alloc&)
       {
         return IOResult::AllocationFailure;
       }
       catch (...)
       {
         return IOResult::UnknownError;
       }
     },
     buffer);
  }

  //// CFileBufferedByteReader

  CFileBufferedByteReader::CFileBufferedByteReader(FILE* const file) :
    IByteReader(),
    m_FileHandle{file},
    m_LocalBuffer{}
  {
    binaryIOAssert(file != nullptr, "Invalid file handle.");

    buffer_start = nullptr;
    cursor       = nullptr;
    buffer_end   = nullptr;
    last_result  = IOResult::Success;
    refill_fn    = [](IByteReader* self_) -> IOResult {
      CFileBufferedByteReader* const self = static_cast<CFileBufferedByteReader*>(self_);

      if (std::feof(self->m_FileHandle))
      {
        return self->setFailureState(IOResult::EndOfStream);
      }

      std::uint8_t* const local_buffer_bgn    = std::begin(self->m_LocalBuffer);
      std::uint8_t* const local_buffer_end    = std::end(self->m_LocalBuffer);
      std::uint8_t* const local_buffer_cursor = const_cast<std::uint8_t*>(self->cursor);
      std::uint8_t* const write_start         = std::rotate(local_buffer_bgn, local_buffer_cursor, local_buffer_end);
      const std::size_t   num_bytes_in_buffer = local_buffer_end - write_start;
      const std::size_t   num_bytes_read      = std::fread(write_start, sizeof(write_start[0]), num_bytes_in_buffer, self->m_FileHandle);

      if (num_bytes_read != 0)
      {
        self->buffer_start = write_start;
        self->cursor       = write_start;
        self->buffer_end   = write_start + num_bytes_read;
      }
      else
      {
        if (std::feof(self->m_FileHandle))
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
      }

      return IOResult::Success;
    };
  }
}  // namespace assetio

#if 0
// [https://lxp32.github.io/docs/a-simple-example-crc32-calculation/]
void crc32_GenerateHeader(std::FILE* const stream)
{
  std::fprintf(stream, "#ifndef BF_CRC32_HPP\n");
  std::fprintf(stream, "#define BF_CRC32_HPP\n\n");

  std::uint32_t crc32_table[256];

  for (uint32_t i = 0; i < 256; ++i)
  {
    std::uint32_t ch  = i;
    std::uint32_t crc = 0;

    for (std::size_t j = 0; j < 8; ++j)
    {
      const std::uint32_t b = (ch ^ crc) & 1;

      crc >>= 1;

      if (b)
      {
        crc = crc ^ 0xEDB88320 /* Polynomial */;
      }

      ch >>= 1;
    }

    crc32_table[i] = crc;
  }

  std::fprintf(stream, "#include <cstdint>\n#include <cstdio>\n\n// clang-format off\ninline static constexpr std::uint32_t k_Crc32Table[256] =\n");
  std::fprintf(stream, "{");

  for (uint32_t i = 0; i < 256; ++i)
  {
    if (!(i % 8))
    {
      std::fprintf(stream, "\n  ");
    }

    std::fprintf(stream, "0x%08X, ", crc32_table[i]);
  }

  std::fprintf(stream, "\n};\n// clang-format on\n\n");

  const char* k_Crc32Src = R"(// crc-32b
inline constexpr std::uint32_t crc32(const char* bytes, std::size_t num_bytes, std::uint32_t crc = 0xFFFFFFFF) 
{
  for (std::size_t i = 0; i < num_bytes; ++i) 
  {
    crc = (crc >> 8) ^ k_Crc32Table[(bytes[i] ^ crc) & 0xFF];
  }
  
  return ~crc;
}

inline constexpr std::uint32_t crc32(const char* bytes, std::uint32_t crc = 0xFFFFFFFF) 
{
  while (*bytes)
  {
    crc = (crc >> 8) ^ k_Crc32Table[(*bytes++ ^ crc) & 0xFF];
  }

  return ~crc;
}

inline constexpr std::uint32_t operator""_hash(const char* s, std::size_t size) 
{
  return crc32(s, size);
}
)";

  std::fprintf(stream, "%s\n", k_Crc32Src);
  std::fprintf(stream, "void crc32_GenerateHeader(FILE* stream);\n\n");

  std::fprintf(stream, "#endif /*  BF_CRC32_HPP */\n\n");
}

#endif

/******************************************************************************/
/*
  MIT License

  Copyright (c) 2022 Shareef Abdoul-Raheem

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
