/******************************************************************************/
/*!
 * @file   rel_ptr.hpp
 * @author Shareef Raheem (https://blufedora.github.io)
 * @date   2021-09-19
 * @brief
 *   A pointer type that uses a relative signed offset from it's own address.
 *
 *   References:
 *     [https://steamcdn-a.akamaihd.net/apps/valve/2015/Migdalskiy_Sergiy_Physics_Optimization_Strategies.pdf]
 *
 * @copyright Copyright (c) 2021-2022 Shareef Abdoul-Raheem
 */
/******************************************************************************/
#ifndef REL_PTR_HPP
#define REL_PTR_HPP

#include "binary_assert.hpp"  // bfBinaryAssert

#include <climits>      // CHAR_BIT
#include <cstdint>      // int8_t, int16_t, int32_t int64_t, uint8_t, uint16_t, uint32_t uint64_t
#include <limits>       // numeric_limits
#include <type_traits>  // is_integral_v, is_unsigned_v

namespace assetio
{
  /*!
   * @brief
   *   A pointer type that uses a relative signed offset from it's own address.
   *
   * @tparam offset_type
   *   Controls the size of this pointer type.
   *
   * @tparam T
   *   Datatype that this pointer points to.
   *
   * @tparam alignment_type
   *   If you know the min alignment of all uses of the type pointer to by offset
   *   you can increase the range by using larger strides.
   */
  template<typename offset_type_t, typename T, typename alignment_type_t = std::uint8_t>
  struct rel_ptr
  {
    using value_type     = T;
    using offset_type    = offset_type_t;
    using alignment_type = alignment_type_t;

    static_assert(std::is_integral_v<offset_type> && !std::is_unsigned_v<offset_type>, "offset_type must be a signed integer type.");
    static_assert(std::is_integral_v<alignment_type> && std::is_unsigned_v<alignment_type>, "alignment_type must be an unsigned integer type.");

    static constexpr offset_type    k_OffsetMax          = std::numeric_limits<offset_type>::max();
    static constexpr offset_type    k_OffsetMin          = std::numeric_limits<offset_type>::min();
    static constexpr alignment_type k_AlignmentByteCount = std::numeric_limits<alignment_type>::digits / CHAR_BIT;

    offset_type offset = 0;  //!< The stored offset from the address of `this`.

    rel_ptr() = default;

    rel_ptr(const std::nullptr_t) :
      offset{0}
    {
    }

    rel_ptr(T* const rhs) :
      offset{calculateOffset(rhs, base())}
    {
    }

    // This datatype should not be copied or moved automatically.

    rel_ptr(const rel_ptr& rhs) = delete;
    rel_ptr(rel_ptr&& rhs)      = delete;
    rel_ptr& operator=(const rel_ptr& rhs) = delete;
    rel_ptr& operator=(rel_ptr&& rhs) = delete;

    // Assignment Operators //

    rel_ptr& operator=(T* const rhs) { return assign(rhs), *this; }
    rel_ptr& operator=(const std::nullptr_t n) { return assign(n), *this; }

    // Pointer Operators //

    T* operator->() const { return get(); }
    T& operator*() const { return *get(); }
    T& operator[](const std::size_t idx) const { return get()[idx]; }

    // Conversion Operators //

    operator T*() const { return get(); }
    operator bool() const { return !isNull(); }

    // Misc //

    void            assign(T* const rhs) { offset = calculateOffset(rhs, base()); }
    void            assign(std::nullptr_t) { offset = 0; }
    bool            isNull() const { return offset == 0; }
    T*              get() const { return isNull() ? nullptr : reinterpret_cast<T*>(base() + offset); }
    alignment_type* base() const { return reinterpret_cast<alignment_type*>(const_cast<offset_type*>(&offset)); }

    /*!
     * @brief
     *   Calculates the offset from the pointer to the base address using the `alignment_type` as the stride.
     *
     * @param rhs
     *   The pointer whose offset you want.
     *
     * @param base
     *   Base address from which the offset is from.
     *
     * @return
     *   The offset from the pointer to the base address.
     */
    static offset_type calculateOffset(T* const rhs, const alignment_type* base)
    {
      if (!rhs) { return 0; }

      const std::ptrdiff_t off = reinterpret_cast<const alignment_type*>(rhs) - base;

      binaryIOAssert(reinterpret_cast<std::uintptr_t>(rhs) % k_AlignmentByteCount == 0, "Invalid pointer alignment, decrease alignment_type.");
      binaryIOAssert(off >= k_OffsetMin && off <= k_OffsetMax, "Pointer out of range, increase offset_type.");

      return static_cast<offset_type>(off);
    }

    friend inline bool operator==(const rel_ptr& lhs, const rel_ptr& rhs)
    {
      return lhs.get() == rhs.get();
    }

    friend inline bool operator!=(const rel_ptr& lhs, const rel_ptr& rhs)
    {
      return lhs.get() != rhs.get();
    }
  };

  template<typename TCount, typename TPtr>
  struct rel_array
  {
    using alignment_type = typename TPtr::alignment_type;
    using offset_type    = typename TPtr::offset_type;
    using value_type     = typename TPtr::value_type;

    TPtr   elements     = nullptr;
    TCount num_elements = 0u;

    auto& operator[](const std::size_t idx) const { return elements.get()[idx]; }

    auto begin() { return elements.get(); }
    auto end() { return begin() + num_elements; }
    auto begin() const { return elements.get(); }
    auto end() const { return begin() + num_elements; }

    bool isEmpty() const { return num_elements == 0u; }
  };

  template<typename T, typename alignment_type = std::uint8_t>
  using rel_ptr8 = rel_ptr<std::int8_t, T, alignment_type>;

  template<typename T, typename alignment_type = std::uint8_t>
  using rel_ptr16 = rel_ptr<std::int16_t, T, alignment_type>;

  template<typename T, typename alignment_type = std::uint8_t>
  using rel_ptr32 = rel_ptr<std::int32_t, T, alignment_type>;

  template<typename T, typename alignment_type = std::uint8_t>
  using rel_ptr64 = rel_ptr<std::int64_t, T, alignment_type>;

  template<typename T>
  using rel_array8 = rel_array<std::uint8_t, rel_ptr8<T>>;

  template<typename T>
  using rel_array16 = rel_array<std::uint16_t, rel_ptr16<T>>;

  template<typename T>
  using rel_array32 = rel_array<std::uint32_t, rel_ptr32<T>>;

  template<typename T>
  using rel_array64 = rel_array<std::uint64_t, rel_ptr64<T>>;

}  // namespace assetio

#endif /* REL_PTR_HPP */

/******************************************************************************/
/*
  MIT License

  Copyright (c) 2021-2022 Shareef Abdoul-Raheem

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
