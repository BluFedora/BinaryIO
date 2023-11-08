/******************************************************************************/
/*!
 * @file   binary_assert.hpp
 * @author Shareef Raheem (https://blufedora.github.io)
 * @date   2022-06-11
 * @brief
 *   Binary IO Library Assertion functionality.
 *
 * @todo Variadic args for error message to assert.
 *
 * @copyright Copyright (c) 2022 Shareef Abdoul-Raheem
 */
/******************************************************************************/
#ifndef BINARY_ASSERTION_HPP
#define BINARY_ASSERTION_HPP

namespace assetio
{
  namespace detail
  {
    bool binaryIOAssertImpl(const bool expr, const char* const expr_str, const char* const filename, const int line_number, const char* const assert_msg);
  }  // namespace detail
}  // namespace assetio

#define binaryIOAssert(expr, msg) ::assetio::detail::binaryIOAssertImpl((expr), #expr, __FILE__, __LINE__, (msg))

#endif /* BINARY_ASSERTION_HPP */

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
