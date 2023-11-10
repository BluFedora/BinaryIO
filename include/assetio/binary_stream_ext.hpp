/******************************************************************************/
/*!
 * @file   binary_stream_ext.hpp
 * @author Shareef Abdoul-Raheem (https://blufedora.github.io/)
 * @date   2022-03-30
 * @brief
 *   Helpers for standard types for the binary stream interface.
 *
 * @copyright Copyright (c) 2022-2023
 */
/******************************************************************************/
#ifndef BINARY_STREAM_EXT_HPP
#define BINARY_STREAM_EXT_HPP

#include "binary_stream.hpp"  // ByteWriterView, IByteReader

#include <cstdio>  // FILE, fread, feof, BUFSIZ
#include <vector>  // vector<T>

namespace assetio
{
  struct Buffer
  {
    void*       ptr;
    std::size_t written;
    std::size_t capacity;

    Buffer(void* const ptr, std::size_t capacity) :
      ptr{ptr},
      written{0},
      capacity{capacity}
    {
    }
  };

  ByteWriterView byteWriterViewFromVector(std::vector<uint8_t>* const buffer);
  ByteWriterView byteWriterViewFromFile(std::FILE* const file_handle);
  ByteWriterView byteWriterViewFromBuffer(Buffer* const buffer);

  class CFileBufferedByteReader : public IByteReader
  {
   private:
    std::FILE* const m_FileHandle;
    std::uint8_t     m_LocalBuffer[BUFSIZ];

   public:
    CFileBufferedByteReader(FILE* const file_handle);
  };
}  // namespace assetio

#endif /* BINARY_STREAM_EXT_HPP */

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
