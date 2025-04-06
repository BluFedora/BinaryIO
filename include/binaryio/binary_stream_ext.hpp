/******************************************************************************/
/*!
 * @file   binary_stream_ext.hpp
 * @author Shareef Abdoul-Raheem (https://blufedora.github.io/)
 * @date   2022-03-30
 * @brief
 *   Helpers for standard types for the binary stream interface.
 *
 * @copyright Copyright (c) 2022-2024 Shareef Abdoul-Raheem
 */
/******************************************************************************/
#ifndef BINARY_STREAM_EXT_HPP
#define BINARY_STREAM_EXT_HPP

#include "binary_stream.hpp"  // ByteWriterView, IByteReader

#include <cstdio>  // FILE, fread, feof
#include <vector>  // vector<T>

namespace binaryIO
{
  template<typename Allocator>
  struct VectorStream : public IOStream
  {
    std::vector<uint8_t, Allocator>* buffer   = nullptr;
    IOSize                           position = 0u;
  };

  IOStream IOStream_FromCFile(std::FILE* const file_handle);

  template<typename Allocator>
  IOStream IOStream_FromVector(std::vector<uint8_t, Allocator>* const buffer)
  {
    IOStream result = {};
    result.Size     = +[](IOStream* const stream) -> IOResult {
      const std::vector<uint8_t, Allocator>* const buffer = static_cast<std::vector<uint8_t, Allocator>*>(stream->user_data.values[0].as_handle);

      return buffer->size();
    };
    result.Read = +[](IOStream* const stream, void* const destination, const IOSize num_destination_bytes) -> IOResult {
      const std::vector<uint8_t, Allocator>* const buffer = static_cast<std::vector<uint8_t, Allocator>*>(stream->user_data.values[0].as_handle);
      IOSize&                                      cursor = stream->user_data.values[1].as_size;

      return MemoryStream_CopyBytes(destination, num_destination_bytes, buffer->data() + cursor, buffer->size() - cursor, num_destination_bytes, &cursor);
    };
    result.Write = +[](IOStream* const stream, const void* const source, const IOSize num_source_bytes) -> IOResult {
      std::vector<uint8_t, Allocator>* const buffer = static_cast<std::vector<uint8_t, Allocator>*>(stream->user_data.values[0].as_handle);
      IOSize&                                cursor = stream->user_data.values[1].as_size;

      const IOSize needed_size = cursor + num_source_bytes;

      if (needed_size > buffer->size())
      {
        try
        {
          buffer->resize(needed_size);
        }
        catch (const std::bad_alloc&)
        {
          return IOErrorCode::AllocationFailure;
        }
        catch (...)
        {
          return IOErrorCode::UnknownError;
        }
      }

      return MemoryStream_CopyBytes(buffer->data() + cursor, buffer->size() - cursor, source, num_source_bytes, num_source_bytes, &cursor);
    };
    result.Seek = +[](IOStream* const stream, const IOOffset offset, const SeekOrigin seek_origin) -> IOResult {
      std::vector<uint8_t, Allocator>* const buffer = static_cast<std::vector<uint8_t, Allocator>*>(stream->user_data.values[0].as_handle);
      IOSize&                                cursor = stream->user_data.values[1].as_size;

      IOOffset final_seek_pos = offset;

      if (seek_origin == SeekOrigin::CURRENT)
      {
        final_seek_pos += cursor;
      }
      else if (seek_origin == SeekOrigin::END)
      {
        final_seek_pos += buffer->size();
      }

      if (final_seek_pos >= 0)
      {
        if (IOSize(final_seek_pos) > buffer->size())
        {
          try
          {
            buffer->resize(final_seek_pos);
            cursor = final_seek_pos;
          }
          catch (const std::bad_alloc&)
          {
            return IOErrorCode::AllocationFailure;
          }
          catch (...)
          {
            return IOErrorCode::UnknownError;
          }
        }
        else
        {
          cursor = final_seek_pos;
        }

        return IOErrorCode::Success;
      }

      return IOErrorCode::SeekError;
    };
    result.Close                         = nullptr;
    result.user_data.values[0].as_handle = buffer;
    result.user_data.values[1].as_size   = 0;

    return result;
  }

}  // namespace binaryIO

#endif /* BINARY_STREAM_EXT_HPP */

/******************************************************************************/
/*
  MIT License

  Copyright (c) 2022-2024 Shareef Abdoul-Raheem

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
