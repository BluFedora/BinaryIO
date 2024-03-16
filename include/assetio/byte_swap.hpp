/******************************************************************************/
/*!
 * @file   byte_swap.hpp
 * @author Shareef Raheem (https://blufedora.github.io)
 * @brief
 *   Intinsics for fast byte swapping routines, should just compile down to a
 *   `bswap` instuction on x86, or a `rev` / `rev16` instuction on arm.
 *
 * @copyright Copyright (c) 2023-2024 Shareef Abdoul-Raheem
 */
/******************************************************************************/
#ifndef LIB_ASSETIO_BINARYIO_BYTE_SWAP_HPP
#define LIB_ASSETIO_BINARYIO_BYTE_SWAP_HPP

#include <cstdint>  // uint16_t, uint32_t, uint64_t

#if defined(_MSC_VER)
#include <stdlib.h>  // _byteswap_ushort, _byteswap_ulong, _byteswap_uint64
#endif

namespace binaryio
{
  // Portable implementations of byte swap

  namespace detail
  {
    constexpr std::uint16_t byteSwap16_portable(const std::uint16_t value)
    {
      return ((value & 0x00FF) << 8) |
             ((value & 0xFF00) >> 8);
    }

    constexpr std::uint32_t byteSwap32_portable(const std::uint32_t value)
    {
      return ((value & 0x000000FF) << 24) |
             ((value & 0x0000FF00) << 8) |
             ((value & 0x00FF0000) >> 8) |
             ((value & 0xFF000000) >> 24);
    }

    constexpr std::uint64_t byteSwap64_portable(const std::uint64_t value)
    {
      return ((value & 0x00000000000000FF) << 56) |
             ((value & 0x000000000000FF00) << 40) |
             ((value & 0x0000000000FF0000) << 24) |
             ((value & 0x00000000FF000000) << 8) |
             ((value & 0x000000FF00000000) >> 8) |
             ((value & 0x0000FF0000000000) >> 24) |
             ((value & 0x00FF000000000000) >> 40) |
             ((value & 0xFF00000000000000) >> 56);
    }
  }  // namespace detail

  // Intrinsic verions of the byte swap functions

#if defined(__GNUC__) || defined(__clang__) || defined(__INTEL_LLVM_COMPILER)  // gcc or clang or intel
  constexpr std::uint16_t byteSwap16(const std::uint16_t value)
  {
    return __builtin_bswap16(value);
  }

  constexpr std::uint32_t byteSwap32(const std::uint32_t value)
  {
    return __builtin_bswap32(value);
  }

  constexpr std::uint64_t byteSwap64(const std::uint64_t value)
  {
    return __builtin_bswap64(value);
  }
#elif defined(_MSC_VER)  // msvc
  inline std::uint16_t byteSwap16(const std::uint16_t value)
  {
    if constexpr (sizeof(unsigned short) == sizeof(value))
    {
      return _byteswap_ushort(value);
    }
    else
    {
      return detail::byteSwap16_portable(value);
    }
  }

  inline std::uint32_t byteSwap32(const std::uint32_t value)
  {
    if constexpr (sizeof(unsigned long) == sizeof(value))
    {
      return _byteswap_ulong(value);
    }
    else
    {
      return detail::byteSwap16_portable(value);
    }
  }

  inline std::uint64_t byteSwap64(const std::uint64_t value)
  {
    if constexpr (sizeof(unsigned __int64) == sizeof(value))
    {
      return _byteswap_uint64(value);
    }
    else
    {
      return detail::byteSwap16_portable(value);
    }
  }
#else
  constexpr std::uint16_t byteSwap16(const std::uint16_t value)
  {
    return detail::byteSwap16_portable(value);
  }

  constexpr std::uint32_t byteSwap32(const std::uint32_t value)
  {
    return detail::byteSwap32_portable(value);
  }

  constexpr std::uint64_t byteSwap64(const std::uint64_t value)
  {
    return detail::byteSwap64_portable(value);
  }
#endif

  // Constexpr versions (only needed on msvc since their intrinsics are not marked constexpr.)

  template<std::uint16_t value>
  constexpr std::uint16_t byteSwap16()
  {
    return detail::byteSwap16_portable(value);
  }

  template<std::uint32_t value>
  constexpr std::uint32_t byteSwap32()
  {
    return detail::byteSwap32_portable(value);
  }

  template<std::uint64_t value>
  constexpr std::uint64_t byteSwap64()
  {
    return detail::byteSwap64_portable(value);
  }
}  // namespace binaryio

#endif  // LIB_ASSETIO_BINARYIO_BYTE_SWAP_HPP

/******************************************************************************/
/*
  MIT License

  Copyright (c) 2023-2024 Shareef Abdoul-Raheem

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
