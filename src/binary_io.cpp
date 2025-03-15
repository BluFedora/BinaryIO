/******************************************************************************/
/*!
 * @file   binary_io.cpp
 * @author Shareef Abdoul-Raheem (https://blufedora.github.io/)
 * @date   2022-06-29
 * @brief
 *   Implementation of any code that doesnt belong in the header.
 *
 * @copyright Copyright (c) 2022-2025 Shareef Abdoul-Raheem
 */
/******************************************************************************/
#include "binaryio/binary_chunk.hpp"
#include "binaryio/binary_stream.hpp"
#include "binaryio/binary_stream_ext.hpp"
#include "binaryio/binary_types.hpp"

#include <algorithm>  // rotate
#include <cstdio>     // fprintf, stderr
#include <cstdlib>    // abort
#include <cstring>    // memcpy
#include <utility>    // exchange

// binary_assert.hpp

bool binaryIO::detail::binaryIOAssertImpl(const bool expr, const char* const expr_str, const char* const filename, const int line_number, const char* const assert_msg)
{
  if (!expr)
  {
    std::fprintf(stderr, "BinaryIO[%s:%i] Assertion '%s' failed, %s.\n", filename, line_number, expr_str, assert_msg);
    std::abort();
  }

  return expr;
}

// binary_api.hpp

static binaryIO::BufferedIO SetupMemoryBufferedIO(const void* const bytes, const binaryIO::IOSize num_bytes)
{
  binaryIO::BufferedIO result;
  result.buffer_start = static_cast<const uint8_t*>(bytes);
  result.cursor       = static_cast<const uint8_t*>(bytes);
  result.buffer_end   = static_cast<const uint8_t*>(bytes) + num_bytes;
  result.Refill       = +[](binaryIO::IOStream* const stream) -> binaryIO::IOErrorCode {
    // A memory stream cannot be refilled.
    return BufferedIO_Failure(stream, binaryIO::IOErrorCode::EndOfStream);
  };
  return result;
}

static binaryIO::IOResult MemoryStream_Size(binaryIO::IOStream* const stream)
{
  return stream->user_data.memory_stream.buffer_size;
}

binaryIO::IOResult binaryIO::MemoryStream_CopyBytes(
 void* const             destination,
 const binaryIO::IOSize  num_destination_bytes,
 const void* const       source,
 const binaryIO::IOSize  num_source_bytes,
 const binaryIO::IOSize  desired_number_of_bytes,
 binaryIO::IOSize* const in_out_cursor)
{
  const binaryIO::IOSize num_bytes_to_copy = num_source_bytes < num_destination_bytes ? num_source_bytes : num_destination_bytes;

  std::memcpy(destination, source, num_bytes_to_copy);

  (*in_out_cursor) += num_bytes_to_copy;

  return IOResult(num_bytes_to_copy, num_bytes_to_copy == desired_number_of_bytes ? IOErrorCode::Success : IOErrorCode::EndOfStream);
}

static binaryIO::IOResult MemoryStream_Read(binaryIO::IOStream* const stream, void* const destination, const binaryIO::IOSize num_destination_bytes)
{
  binaryIO::MemoryStreamData& memory_stream = stream->user_data.memory_stream;

  return binaryIO::MemoryStream_CopyBytes(
   destination,
   num_destination_bytes,
   memory_stream.CursorBytes(),
   memory_stream.BytesLeft(),
   num_destination_bytes,
   &memory_stream.cursor);
}

static binaryIO::IOResult MemoryStream_Write(binaryIO::IOStream* const stream, const void* const source, const binaryIO::IOSize num_source_bytes)
{
  binaryIO::MemoryStreamData& memory_stream = stream->user_data.memory_stream;

  return binaryIO::MemoryStream_CopyBytes(
   memory_stream.CursorBytes(),
   memory_stream.BytesLeft(),
   source,
   num_source_bytes,
   num_source_bytes,
   &memory_stream.cursor);
}

static binaryIO::IOResult MemoryStream_Seek(binaryIO::IOStream* const stream, const binaryIO::IOOffset offset, const binaryIO::SeekOrigin seek_origin)
{
  const binaryIO::IOOffset base_offset[] =
   {
    0,
    binaryIO::IOOffset(stream->user_data.memory_stream.cursor),
    binaryIO::IOOffset(stream->user_data.memory_stream.buffer_size),
   };

  const binaryIO::IOOffset absolute_location = base_offset[int(seek_origin)] + offset;

  binaryIO::IOErrorCode err_code;
  if (absolute_location >= 0 && binaryIO::IOSize(absolute_location) < stream->user_data.memory_stream.buffer_size)
  {
    stream->user_data.memory_stream.cursor = absolute_location;
    err_code                               = binaryIO::IOErrorCode::Success;
  }
  else
  {
    err_code = binaryIO::IOErrorCode::SeekError;
  }

  return binaryIO::IOResult(stream->user_data.memory_stream.cursor, err_code);
}

static binaryIO::IOErrorCode MemoryStream_Close(binaryIO::IOStream* const stream)
{
  (void)stream;
  return binaryIO::IOErrorCode::Success;
}

bool binaryIO::IOSteam_SupportsRead(const IOStream* const stream)
{
  return stream->Read != nullptr;
}

bool binaryIO::IOSteam_SupportsWrite(const IOStream* const stream)
{
  return stream->Write != nullptr;
}

bool binaryIO::IOSteam_SupportsBufferedRead(const IOStream* const stream)
{
  return stream->buffered_io.Refill != nullptr;
}

bool binaryIO::IOSteam_SupportsSeek(const IOStream* const stream)
{
  return stream->Seek != nullptr;
}

binaryIO::IOStream binaryIO::IOStream_FromRWMemory(void* const bytes, const IOSize num_bytes)
{
  binaryIO::IOStream result      = {};
  result.Size                    = &MemoryStream_Size;
  result.Read                    = &MemoryStream_Read;
  result.Write                   = &MemoryStream_Write;
  result.Seek                    = &MemoryStream_Seek;
  result.Close                   = &MemoryStream_Close;
  result.user_data.memory_stream = MemoryStreamData{bytes, 0, num_bytes};
  result.buffered_io             = SetupMemoryBufferedIO(bytes, num_bytes);

  return result;
}

binaryIO::IOStream binaryIO::IOStream_FromROMemory(const void* const bytes, const IOSize num_bytes)
{
  binaryIO::IOStream result      = {};
  result.Size                    = &MemoryStream_Size;
  result.Read                    = &MemoryStream_Read;
  result.Write                   = nullptr;
  result.Seek                    = &MemoryStream_Seek;
  result.Close                   = &MemoryStream_Close;
  result.user_data.memory_stream = MemoryStreamData{const_cast<void*>(bytes), 0, num_bytes};
  result.buffered_io             = SetupMemoryBufferedIO(bytes, num_bytes);

  return result;
}

static void AccumulateError(binaryIO::IOStream* const stream, const binaryIO::IOErrorCode error_code)
{
  if (stream->error_state == binaryIO::IOErrorCode::Success)
  {
    stream->error_state = error_code;
  }
}

binaryIO::IOErrorCode binaryIO::IOStream_ResetErrorState(IOStream* const stream)
{
  return std::exchange(stream->error_state, IOErrorCode::Success);
}

binaryIO::IOResult binaryIO::IOStream_Size(IOStream* const stream)
{
  if (stream->Size)
  {
    return stream->Size(stream);
  }

  AccumulateError(stream, binaryIO::IOErrorCode::InvalidOperation);
  return binaryIO::IOErrorCode::InvalidOperation;
}

binaryIO::IOResult binaryIO::IOStream_Read(IOStream* const stream, void* const destination, const IOSize num_destination_bytes)
{
  if (num_destination_bytes == 0)
  {
    return binaryIO::IOErrorCode::Success;
  }

  if (stream->Read)
  {
    const binaryIO::IOResult result = stream->Read(stream, destination, num_destination_bytes);

    AccumulateError(stream, result.ErrorCode());
    return result;
  }

  AccumulateError(stream, binaryIO::IOErrorCode::InvalidOperation);
  return binaryIO::IOErrorCode::InvalidOperation;
}

binaryIO::IOResult binaryIO::IOStream_Write(IOStream* const stream, const void* const source, const IOSize num_source_bytes)
{
  if (num_source_bytes == 0)
  {
    return binaryIO::IOErrorCode::Success;
  }

  if (stream->Write)
  {
    const binaryIO::IOResult result = stream->Write(stream, source, num_source_bytes);

    AccumulateError(stream, result.ErrorCode());
    return result;
  }

  AccumulateError(stream, binaryIO::IOErrorCode::InvalidOperation);
  return binaryIO::IOErrorCode::InvalidOperation;
}

binaryIO::IOResult binaryIO::IOStream_Seek(IOStream* const stream, const IOOffset offset, const SeekOrigin seek_origin)
{
  if (stream->Seek)
  {
    const binaryIO::IOResult result = stream->Seek(stream, offset, seek_origin);

    AccumulateError(stream, result.ErrorCode());
    return result;
  }

  AccumulateError(stream, binaryIO::IOErrorCode::InvalidOperation);
  return binaryIO::IOErrorCode::InvalidOperation;
}

binaryIO::IOErrorCode binaryIO::IOStream_Close(IOStream* const stream)
{
  if (stream->Close)
  {
    const IOErrorCode result = stream->Close(stream);

    AccumulateError(stream, result);
    return result;
  }

  return binaryIO::IOErrorCode::Success;
}

binaryIO::IOSize binaryIO::BufferedIO_NumBytesAvailable(const IOStream* const stream)
{
  return stream->buffered_io.buffer_end - stream->buffered_io.cursor;
}

binaryIO::IOErrorCode binaryIO::BufferedIO_Refill(IOStream* const stream)
{
  const binaryIO::BufferedIO* const buffered_io = &stream->buffered_io;

  if (buffered_io->Refill)
  {
    binaryIOAssert(buffered_io->cursor == buffered_io->buffer_end, "Expected to have read all of the buffered data.");

    const binaryIO::IOErrorCode result = buffered_io->Refill(stream);

    binaryIOAssert(buffered_io->cursor == buffered_io->buffer_start && buffered_io->cursor < buffered_io->buffer_end,
                   "Invalid refill function, cursor must be in buffer range.");

    return result;
  }

  return binaryIO::IOErrorCode::InvalidOperation;
}

binaryIO::IOResult binaryIO::BufferedIO_Read(IOStream* const stream, void* const destination, const IOSize num_destination_bytes)
{
  unsigned char* const       write_bgn    = static_cast<unsigned char*>(destination);
  const unsigned char* const write_end    = write_bgn + num_destination_bytes;
  unsigned char*             write_cursor = write_bgn;

  binaryIO::BufferedIO* const buffered_io = &stream->buffered_io;

  while (write_cursor != write_end)
  {
    binaryIO::IOErrorCode last_error;

    if (buffered_io->cursor == buffered_io->buffer_end)
    {
      last_error = BufferedIO_Refill(stream);
    }
    else
    {
      last_error = stream->error_state;
    }

    if (last_error != binaryIO::IOErrorCode::Success)
    {
      break;
    }

    const binaryIO::IOSize num_bytes_left_to_read      = write_end - write_cursor;
    const binaryIO::IOSize num_bytes_available_to_read = BufferedIO_NumBytesAvailable(stream);
    const binaryIO::IOSize num_bytes_able_to_read      = std::min(num_bytes_left_to_read, num_bytes_available_to_read);

    std::memcpy(write_cursor, buffered_io->cursor, num_bytes_able_to_read);

    write_cursor += num_bytes_able_to_read;
    buffered_io->cursor += num_bytes_able_to_read;
  }

  return binaryIO::IOResult(write_cursor - write_bgn, stream->error_state);
}

binaryIO::IOErrorCode binaryIO::BufferedIO_Failure(IOStream* const stream, const IOErrorCode error_code)
{
  stream->error_state        = error_code;
  stream->buffered_io.Refill = [](IOStream* const stream) -> IOErrorCode {
    static constexpr const uint8_t s_ZeroBuffer[16] = {0};  // If the size is set to 1 then refill will be called more often but that is still valid.

    stream->buffered_io.buffer_start = s_ZeroBuffer;
    stream->buffered_io.cursor       = s_ZeroBuffer;
    stream->buffered_io.buffer_end   = s_ZeroBuffer + sizeof(s_ZeroBuffer);

    return stream->error_state;
  };

  return error_code;
}

// binary_api_ext.hpp

static binaryIO::IOResult CFile_Read(binaryIO::IOStream* const stream, void* const destination, const binaryIO::IOSize num_destination_bytes)
{
  std::FILE* const file_handle = static_cast<std::FILE*>(stream->user_data.values[0].as_handle);

  if (std::feof(file_handle))
  {
    return BufferedIO_Failure(stream, binaryIO::IOErrorCode::EndOfStream);
  }

  const std::size_t           num_bytes_read = std::fread(destination, sizeof(unsigned char), num_destination_bytes, file_handle);
  const binaryIO::IOErrorCode error_code     = (num_bytes_read == num_destination_bytes) ? binaryIO::IOErrorCode::Success : binaryIO::IOErrorCode::ReadError;

  return binaryIO::IOResult(num_bytes_read, error_code);
}

static binaryIO::IOResult CFile_Write(binaryIO::IOStream* const stream, const void* const source, const binaryIO::IOSize num_source_bytes)
{
  std::FILE* const            file_handle       = static_cast<std::FILE*>(stream->user_data.values[0].as_handle);
  const std::size_t           num_bytes_written = std::fwrite(source, sizeof(unsigned char), num_source_bytes, file_handle);
  const binaryIO::IOErrorCode error_code        = (num_bytes_written == num_source_bytes) ? binaryIO::IOErrorCode::Success : binaryIO::IOErrorCode::UnknownError;

  return binaryIO::IOResult(num_bytes_written, error_code);
}

static binaryIO::IOResult CFile_Seek(binaryIO::IOStream* const stream, const binaryIO::IOOffset offset, const binaryIO::SeekOrigin seek_origin)
{
  std::FILE* const file_handle = static_cast<std::FILE*>(stream->user_data.values[0].as_handle);

  const int file_origin = [](const binaryIO::SeekOrigin seek_origin) -> int {
    switch (seek_origin)
    {
      case binaryIO::SeekOrigin::BEGIN: return SEEK_SET;
      case binaryIO::SeekOrigin::CURRENT: return SEEK_CUR;
      case binaryIO::SeekOrigin::END: return SEEK_END;
      default:
      {
        binaryIOAssert(false, "Invalid seek origin.\n");
        break;
      }
    }

    return -1;
  }(seek_origin);

  const bool is_null_seek = offset == 0 && file_origin == SEEK_CUR;

#if _WIN32
  if (is_null_seek || _fseeki64(file_handle, offset, file_origin) == 0)
#else
  if (is_null_seek || fseeko(file_handle, offset, file_origin) == 0)
#endif
  {
#if _WIN32
    const binaryIO::IOOffset file_position = _ftelli64(file_handle);
#else
    const binaryIO::IOOffset file_position = ftello(file_handle);
#endif

    if (file_position != -1L)
    {
      return binaryIO::IOResult(file_position, binaryIO::IOErrorCode::Success);
    }
  }

  return binaryIO::IOErrorCode::SeekError;
}

static binaryIO::IOErrorCode CFile_Close(binaryIO::IOStream* const stream)
{
  std::FILE* const file_handle = static_cast<std::FILE*>(stream->user_data.values[0].as_handle);

  const int err = std::fclose(file_handle);

  return err == 0 ? binaryIO::IOErrorCode::Success : binaryIO::IOErrorCode::UnknownError;
}

binaryIO::IOStream binaryIO::IOStream_FromCFile(std::FILE* const file_handle)
{
  binaryIO::IOStream result            = {};
  result.Size                          = nullptr;
  result.Read                          = &CFile_Read;
  result.Write                         = &CFile_Write;
  result.Seek                          = &CFile_Seek;
  result.Close                         = &CFile_Close;
  result.user_data.values[0].as_handle = file_handle;

  return result;
}

#if 0
static binaryIO::IOErrorCode Generic_Refill(binaryIO::IOStream* const stream)
{
  if (stream->error_state != binaryIO::IOErrorCode::Success)
  {
    return stream->error_state;
  }

  binaryIO::BufferedIO* const buffered_io = &stream->buffered_io;

  std::uint8_t* const         data_bgn            = const_cast<std::uint8_t*>(buffered_io->buffer_start);
  std::uint8_t* const         read_end            = const_cast<std::uint8_t*>(buffered_io->cursor);
  std::uint8_t* const         data_end            = const_cast<std::uint8_t*>(buffered_io->buffer_end);
  std::uint8_t* const         write_start         = std::rotate(data_bgn, read_end, data_end);
  const std::size_t           num_bytes_in_buffer = static_cast<std::uint8_t*>(stream->user_datas[0]) - write_start;
  const binaryIO::IOResult    read_result         = IOStream_Read(stream, write_start, num_bytes_in_buffer);
  const binaryIO::IOErrorCode read_error          = read_result.ErrorCode();

  if (read_error == binaryIO::IOErrorCode::Success)
  {
    buffered_io->buffer_start = data_bgn;
    buffered_io->cursor       = buffered_io->buffer_start;
    buffered_io->buffer_end   = write_start + read_result.Value();

    return binaryIO::IOErrorCode::Success;
  }

  return BufferedIO_Failure(stream, read_error);
}
#endif

/******************************************************************************/
/*
  MIT License

  Copyright (c) 2022-2025 Shareef Abdoul-Raheem

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
